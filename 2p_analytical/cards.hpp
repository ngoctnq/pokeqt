#ifndef CARDS_HPP
#define CARDS_HPP

#include <cstdint>

std::uint32_t get_hand_value(std::uint64_t hand);
std::uint64_t correct_ace(std::uint64_t num);
std::uint16_t keep_top_bit(std::uint16_t num);
std::uint32_t get_straight_flush(std::uint64_t hand);
std::uint32_t get_quads(std::uint64_t hand);
std::uint32_t get_trips_pairs(std::uint64_t hand);
std::uint32_t get_max(std::uint32_t a, std::uint32_t b);
std::uint32_t get_min(std::uint32_t a, std::uint32_t b);
std::uint8_t get_canonical_hand(std::uint64_t hand);

#endif // CARDS_HPP
