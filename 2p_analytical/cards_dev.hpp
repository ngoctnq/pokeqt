#ifndef CARDS_DEV_HPP
#define CARDS_DEV_HPP

#include <string>
#include <vector>
#include <iostream>
#include "cards.hpp"

using namespace std;

// for debugging
extern const std::string RANKS;
extern const std::string SUITS;

std::string get_hand_string(std::uint64_t hand);
std::uint64_t get_hand_num(std::string hand_str);
std::uint64_t get_hand_result_print(std::string hand);
int get_canonical_hand_from_int(int c1, int c2);
string get_canonical_from_idx(int idx);

#endif // CARDS_DEV_HPP