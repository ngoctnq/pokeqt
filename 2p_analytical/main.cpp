#include <iostream>
#include <cstdint>
#include <vector>
#include <algorithm>
#include <cassert>
#include <filesystem>
#include <string>
#include <fstream>
#include <thread>
#include <atomic>

#include "cards.hpp"
#include "cards_dev.hpp"

namespace fs = std::filesystem;
using namespace std;

const unsigned int MAX_JOBS = std::thread::hardware_concurrency();

uint64_t int_to_hand(int i) {
    // convert a integer between [0, 52) to a 64-bit representation
    int rank = i / 4;
    int suit = i % 4;
    return ((uint64_t) 1) << (rank + suit * 16);
}

int correct_num(int num, vector<int> cards) {
    // we can assume that the cards are sorted
    for (int i : cards) {
        if (num >= i) {
            num += 1;
        }
    }
    return num;
}

void single_thread(vector<int> cards) {
    // also can assume it's already sorted
    assert(cards.size() == 4);
    assert(cards[0] < cards[1] && cards[1] < cards[2] && cards[2] < cards[3]);
    // 3 game to be made
    uint64_t hand1_game1 = int_to_hand(cards[0]) | int_to_hand(cards[1]);
    uint64_t hand2_game1 = int_to_hand(cards[2]) | int_to_hand(cards[3]);
    uint64_t hand1_game2 = int_to_hand(cards[0]) | int_to_hand(cards[2]);
    uint64_t hand2_game2 = int_to_hand(cards[1]) | int_to_hand(cards[3]);
    uint64_t hand1_game3 = int_to_hand(cards[0]) | int_to_hand(cards[3]);
    uint64_t hand2_game3 = int_to_hand(cards[1]) | int_to_hand(cards[2]);

    // theoretically, you can deduce the tie from the win/loss,
    // but I'm putting it here for later sanity check
    int hand1_game1_wins = 0;
    int hand2_game1_wins = 0;
    int game1_tie = 0;
    int hand1_game2_wins = 0;
    int hand2_game2_wins = 0;
    int game2_tie = 0;
    int hand1_game3_wins = 0;
    int hand2_game3_wins = 0;
    int game3_tie = 0;

    // now iterate through all games, i.e. all 5-card combinations out of 48
    for (int i = 0; i < 48; i++) {
        int i_ = correct_num(i, cards);
        for (int j = i + 1; j < 48; j++) {
            int j_ = correct_num(j, cards);
            for (int k = j + 1; k < 48; k++) {
                int k_ = correct_num(k, cards);
                for (int l = k + 1; l < 48; l++) {
                    int l_ = correct_num(l, cards);
                    for (int m = l + 1; m < 48; m++) {
                        int m_ = correct_num(m, cards);
                        assert(i_ != j_ && j_ != k_ && k_ != l_ && l_ != m_ && i_ != k_ && i_ != l_ && i_ != m_ && j_ != l_ && j_ != m_ && k_ != m_);

                        uint64_t hand_game = int_to_hand(i_) | int_to_hand(j_) | int_to_hand(k_) | int_to_hand(l_) | int_to_hand(m_);
                        
                        uint32_t hand1_game1_value = get_hand_value(hand1_game1 | hand_game);
                        uint32_t hand2_game1_value = get_hand_value(hand2_game1 | hand_game);
                        if (hand1_game1_value > hand2_game1_value) {
                            hand1_game1_wins++;
                        } else if (hand1_game1_value < hand2_game1_value) {
                            hand2_game1_wins++;
                        } else {
                            game1_tie++;
                        }
                        
                        uint32_t hand1_game2_value = get_hand_value(hand1_game2 | hand_game);
                        uint32_t hand2_game2_value = get_hand_value(hand2_game2 | hand_game);
                        if (hand1_game2_value > hand2_game2_value) {
                            hand1_game2_wins++;
                        } else if (hand1_game2_value < hand2_game2_value) {
                            hand2_game2_wins++;
                        } else {
                            game2_tie++;
                        }
                        
                        uint32_t hand1_game3_value = get_hand_value(hand1_game3 | hand_game);
                        uint32_t hand2_game3_value = get_hand_value(hand2_game3 | hand_game);
                        if (hand1_game3_value > hand2_game3_value) {
                            hand1_game3_wins++;
                        } else if (hand1_game3_value < hand2_game3_value) {
                            hand2_game3_wins++;
                        } else {
                            game3_tie++;
                        }
                    }
                }
            }
        }
    }

    int total = 48 * 47 * 46 * 45 * 44 / (5 * 4 * 3 * 2 * 1);
    assert(hand1_game1_wins + hand2_game1_wins + game1_tie == total);
    assert(hand1_game2_wins + hand2_game2_wins + game2_tie == total);
    assert(hand1_game3_wins + hand2_game3_wins + game3_tie == total);

    string hand1, hand2;
    ofstream file;
    fs::create_directories("results");

    hand1 = get_hand_string(hand1_game1);
    hand2 = get_hand_string(hand2_game1);
    fs::create_directories("results/" + hand1);
    fs::create_directories("results/" + hand2);
    // write to file
    file.open("results/" + hand1 + "/" + hand2 + ".txt");
    file << std::fixed << std::setprecision(4);
    file << hand1 << " vs " << hand2 << endl;
    file << "Win:  " << 100. * hand1_game1_wins / total << "% (" << hand1_game1_wins << "/" << total << ")" << endl;
    file << "Loss: " << 100. * hand2_game1_wins / total << "% (" << hand2_game1_wins << "/" << total << ")" << endl;
    file << "Tie:  " << 100. * game1_tie / total << "% (" << game1_tie << "/" << total << ")" << endl;
    file.close();

    file.open("results/" + hand2 + "/" + hand1 + ".txt");
    file << std::fixed << std::setprecision(4);
    file << hand2 << " vs " << hand1 << endl;
    file << "Win:  " << 100. * hand2_game1_wins / total << "% (" << hand2_game1_wins << "/" << total << ")" << endl;
    file << "Loss: " << 100. * hand1_game1_wins / total << "% (" << hand1_game1_wins << "/" << total << ")" << endl;
    file << "Tie:  " << 100. * game1_tie / total << "% (" << game1_tie << "/" << total << ")" << endl;
    file.close();
    
    hand1 = get_hand_string(hand1_game2);
    hand2 = get_hand_string(hand2_game2);
    fs::create_directories("results/" + hand1);
    fs::create_directories("results/" + hand2);
    // write to file
    file.open("results/" + hand1 + "/" + hand2 + ".txt");
    file << std::fixed << std::setprecision(4);
    file << hand1 << " vs " << hand2 << endl;
    file << "Win:  " << 100. * hand1_game2_wins / total << "% (" << hand1_game2_wins << "/" << total << ")" << endl;
    file << "Loss: " << 100. * hand2_game2_wins / total << "% (" << hand2_game2_wins << "/" << total << ")" << endl;
    file << "Tie:  " << 100. * game2_tie / total << "% (" << game2_tie << "/" << total << ")" << endl;
    file.close();

    file.open("results/" + hand2 + "/" + hand1 + ".txt");
    file << std::fixed << std::setprecision(4);
    file << hand2 << " vs " << hand1 << endl;
    file << "Win:  " << 100. * hand2_game2_wins / total << "% (" << hand2_game2_wins << "/" << total << ")" << endl;
    file << "Loss: " << 100. * hand1_game2_wins / total << "% (" << hand1_game2_wins << "/" << total << ")" << endl;
    file << "Tie:  " << 100. * game2_tie / total << "% (" << game2_tie << "/" << total << ")" << endl;
    file.close();
    
    hand1 = get_hand_string(hand1_game3);
    hand2 = get_hand_string(hand2_game3);
    fs::create_directories("results/" + hand1);
    fs::create_directories("results/" + hand2);
    // write to file
    file.open("results/" + hand1 + "/" + hand2 + ".txt");
    file << std::fixed << std::setprecision(4);
    file << hand1 << " vs " << hand2 << endl;
    file << "Win:  " << 100. * hand1_game3_wins / total << "% (" << hand1_game3_wins << "/" << total << ")" << endl;
    file << "Loss: " << 100. * hand2_game3_wins / total << "% (" << hand2_game3_wins << "/" << total << ")" << endl;
    file << "Tie:  " << 100. * game3_tie / total << "% (" << game3_tie << "/" << total << ")" << endl;
    file.close();

    file.open("results/" + hand2 + "/" + hand1 + ".txt");
    file << std::fixed << std::setprecision(4);
    file << hand2 << " vs " << hand1 << endl;
    file << "Win:  " << 100. * hand2_game3_wins / total << "% (" << hand2_game3_wins << "/" << total << ")" << endl;
    file << "Loss: " << 100. * hand1_game3_wins / total << "% (" << hand1_game3_wins << "/" << total << ")" << endl;
    file << "Tie:  " << 100. * game3_tie / total << "% (" << game3_tie << "/" << total << ")" << endl;
    file.close();
    
}

int main() {
    std::vector<std::jthread> threads;
    
    for (int i = 0; i < 52; i++) {
        cout << "Running i=" << i << endl;
        for (int j = i + 1; j < 52; j++) {
            for (int k = j + 1; k < 52; k++) {
                for (int l = k + 1; l < 52; l++) {
                    if (threads.size() >= MAX_JOBS) {
                        threads.erase(threads.begin());
                    }
                    threads.emplace_back([i, j, k, l]() {
                        single_thread(std::vector<int>{i, j, k, l});
                    });
                }
            }
        }
    }
    threads.clear();
    
    return 0;
}
