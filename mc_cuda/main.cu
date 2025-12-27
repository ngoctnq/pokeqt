#include <iostream>
#include <vector>
#include <filesystem>
#include <string>
#include <fstream>
#include <ctime>
#include <chrono>

#include <cuda_runtime.h>
#include <curand_kernel.h>

#include "cards.hpp"

namespace fs = std::filesystem;
using namespace std;

// simulation parameters
#define MAX_NUM_PLAYERS 9
#define MIN_NUM_PLAYERS 2

// kernel parameters
#define THREADS_PER_BLOCK 256
// tune this so that it maximizes utilization
#define BLOCKS_PER_GRID 256
// tune this so that it takes however long you want to run
#define SIMS_PER_THREAD 10000

// 169 hands x (sum of player counts)
const int TOTAL_VECTOR_SIZE = 169 * (MIN_NUM_PLAYERS + MAX_NUM_PLAYERS + 2) * (MAX_NUM_PLAYERS - MIN_NUM_PLAYERS + 1) / 2;

#define CHECK_CUDA(call) { \
    cudaError_t err = call; \
    if (err != cudaSuccess) { \
        fprintf(stderr, "CUDA Error: %s at %s:%d\n", cudaGetErrorString(err), __FILE__, __LINE__); \
        exit(1); \
    } \
}

__device__ uint64_t int_to_hand(int i) {
    // convert a integer between [0, 52) to a 64-bit representation
    int rank = i / 4;
    int suit = i % 4;
    return ((uint64_t) 1) << (rank + suit * 16);
}

__global__ void setup_kernel(curandState *state, unsigned long seed) {
    int id = threadIdx.x + blockIdx.x * blockDim.x;
    curand_init(seed, id, 0, &state[id]);
}

__global__ void mc_kernel(curandState *state, uint64_t *results, int num_sims) {
    int id = threadIdx.x + blockIdx.x * blockDim.x;
    curandState localState = state[id];
    
    // Fixed size arrays instead of vector
    int cards[52];
    uint64_t hands[MAX_NUM_PLAYERS];
    uint32_t values[MAX_NUM_PLAYERS];
    uint32_t tracker_max[MAX_NUM_PLAYERS][MAX_NUM_PLAYERS];
    int tracker_count[MAX_NUM_PLAYERS][MAX_NUM_PLAYERS];

    for (int i = 0; i < 52; ++i) cards[i] = i;

    for (int sim = 0; sim < num_sims; ++sim) {
        // Fisher-Yates shuffle
        for (int i = 51; i > 0; --i) {
            int j = curand(&localState) % (i + 1);
            int temp = cards[i];
            cards[i] = cards[j];
            cards[j] = temp;
        }

        uint64_t board = 0;
        int counter = 0;

        // it would be funny if we deal in real world order with cuts
        for (int player_idx = 0; player_idx < MAX_NUM_PLAYERS; player_idx++) {
            hands[player_idx] = int_to_hand(cards[counter++]);
            hands[player_idx] |= int_to_hand(cards[counter++]);
        }
        for (int board_idx = 0; board_idx < 5; board_idx++) {
            board |= int_to_hand(cards[counter++]);
        }

        for (int player_idx = 0; player_idx < MAX_NUM_PLAYERS; player_idx++) {
            values[player_idx] = get_hand_value(hands[player_idx] | board);
        }

        // Reset trackers
        memset(tracker_max, 0, sizeof(tracker_max));
        memset(tracker_count, 0, sizeof(tracker_count));

        for (int player_idx = 0; player_idx < MAX_NUM_PLAYERS; player_idx++) {
            tracker_max[player_idx][player_idx] = values[player_idx];
            tracker_count[player_idx][player_idx] = 1;

            for (int p_idx_shift = 1; p_idx_shift < MAX_NUM_PLAYERS; p_idx_shift++) {
                int player_idx_to = (player_idx + p_idx_shift) % MAX_NUM_PLAYERS;
                int prev_idx = (player_idx_to + MAX_NUM_PLAYERS - 1) % MAX_NUM_PLAYERS;
                
                uint32_t current_val = values[player_idx_to];
                uint32_t prev_max = tracker_max[player_idx][prev_idx];
                
                tracker_max[player_idx][player_idx_to] = (prev_max > current_val) ? prev_max : current_val;
                
                tracker_count[player_idx][player_idx_to] = (
                    current_val >= prev_max
                ) + tracker_count[player_idx][prev_idx] * (
                    current_val <= prev_max
                );
            }
        }

        // update results
        int starting_idx = 0;
        for (int player_count = MIN_NUM_PLAYERS; player_count <= MAX_NUM_PLAYERS; player_count++) {
            for (int player_idx = 0; player_idx < MAX_NUM_PLAYERS; player_idx++) {
                uint32_t hand_canon = get_canonical_hand(hands[player_idx]);
                int idx_to_check = (player_idx + player_count - 1) % MAX_NUM_PLAYERS;
                uint32_t max_result = tracker_max[player_idx][idx_to_check];
                int tie_count = tracker_count[player_idx][idx_to_check];
                
                uint32_t offset = (values[player_idx] == max_result) * (tie_count - 1) + \
                    (values[player_idx] != max_result) * player_count;
                
                // record win/loss/tie
                size_t update_idx = starting_idx + hand_canon * (player_count + 1) + offset;
                atomicAdd((unsigned long long*) &results[update_idx], 1ULL);
            }
            starting_idx += 169 * (player_count + 1);
        }
    }
    state[id] = localState;
}

void write_out(const vector<uint64_t>& results) {
    int current = 0;
    // write out results
    for (int player_count = MIN_NUM_PLAYERS; player_count <= MAX_NUM_PLAYERS; player_count++) {
        ofstream file("results/" + to_string(player_count) + "p_mc.csv");
        for (int i = 0; i < 169; i++) {
            for (int j = 0; j <= player_count; j++) {
                file << results[current++];
                if (j < player_count) file << ",";
            }
            file << endl;
        }
        file.close();
    }
}

int main(int argc, char** argv) {
    size_t result_size = TOTAL_VECTOR_SIZE * sizeof(uint64_t);

    uint64_t *device_results;
    CHECK_CUDA(cudaMalloc(&device_results, result_size));
    CHECK_CUDA(cudaMemset(device_results, 0, result_size));
    
    // RNG Setup
    curandState *device_state;
    CHECK_CUDA(cudaMalloc(&device_state, THREADS_PER_BLOCK * BLOCKS_PER_GRID * sizeof(curandState)));
    setup_kernel<<<BLOCKS_PER_GRID, THREADS_PER_BLOCK>>>(device_state, time(NULL));
    CHECK_CUDA(cudaGetLastError());
    
    vector<uint64_t> host_results(TOTAL_VECTOR_SIZE);
    cout << "Running simulation on GPU..." << endl;

    // std::chrono::high_resolution_clock::time_point start, end;
    while (true) {
        // start = std::chrono::high_resolution_clock::now();
        mc_kernel<<<BLOCKS_PER_GRID, THREADS_PER_BLOCK>>>(device_state, device_results, SIMS_PER_THREAD);
        CHECK_CUDA(cudaGetLastError());
        CHECK_CUDA(cudaDeviceSynchronize());
        // end = std::chrono::high_resolution_clock::now();
        // cout << "Time elapsed: " << (end - start).count() / 1e9 << "s" << endl;

        CHECK_CUDA(cudaMemcpy(host_results.data(), device_results, result_size, cudaMemcpyDeviceToHost));
        write_out(host_results);
   }
    
    CHECK_CUDA(cudaFree(device_results));
    CHECK_CUDA(cudaFree(device_state));

    return 0;
}
