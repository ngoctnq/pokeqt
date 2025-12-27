#ifndef CARDS_HPP
#define CARDS_HPP

#include <stdint.h>

// each hand is a 64-bit number, every 16 bits are the ranks, 3 MSBs are ignored
__device__ uint32_t get_hand_value(uint64_t hand);
__device__ uint32_t get_canonical_hand(uint64_t hand);

#endif // CARDS_HPP
