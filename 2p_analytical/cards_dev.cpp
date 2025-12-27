#include "cards_dev.hpp"
#include "cards.hpp"
#include <iostream>
#include <string>
#include <vector>

using namespace std;

// for debugging
const string RANKS = "AKQJT98765432";
const string SUITS = "cdhs";

string get_hand_string(uint64_t hand) {
  string hand_str = "";
  uint16_t string_suits[4] = {
    hand & 0xFFFFU,
    (hand >> 16) & 0xFFFFU,
    (hand >> 32) & 0xFFFFU,
    (hand >> 48) & 0xFFFFU
  };
  for (int j = 12; j >= 0; j--) {
    for (int i = 0; i < 4; i++) {
      uint8_t bit = (string_suits[i] >> j) & 1;
      if (bit) {
        hand_str += RANKS[12 - j];
        hand_str += SUITS[3 - i];
      }
    }
  }
  return hand_str;
}

uint64_t get_hand_num(string hand_str) {
  uint64_t hand = 0;
  for (int i = 0; i < hand_str.length(); i += 2) {
    int rank = RANKS.find(hand_str[i]);
    int suit = SUITS.find(hand_str[i + 1]);
    uint64_t card = (((uint64_t) 1) << (12 - rank)) << ((3 - suit) * 16);
    hand |= card;
  }
  return hand;
}

vector<int> get_list_of_bits(uint16_t num) {
  vector<int> bits;
  // skip the 3 MSBs
  for (int i = 0; i < 13; i++) {
    if (num & 1) {
      bits.push_back(12 - i);
    }
    num >>= 1;
  }
  return bits;
}

uint64_t get_hand_result_print(string hand_str) {
  uint64_t hand = get_hand_num(hand_str);

  // print every 16 bits in binary
  uint64_t hand_copy = hand;
  cout << "Hand: " << endl;
  for (int i = 0; i < 4; i++) {
    uint16_t card = hand_copy & 0xFFFFU;
    hand_copy >>= 16;
    printf("%016b\n", card);
  }

  uint32_t result = get_hand_value(hand);

  uint32_t result_copy = result;
  cout << "Result: " << endl;
  for (int i = 0; i < 2; i++) {
    uint16_t card = result_copy & 0xFFFFU;
    result_copy >>= 16;
    printf("%016b\n", card);
  }

  // get prefix
  uint32_t prefix = (result >> 29) << 1;
  printf("Prefix: %04b\n", prefix);

  // hand ranking:
  // Straight Flush   1110 (0xE000)
  // Quads            1100 (0xC000)
  // Full House       1010 (0xA000)
  // Flush            1000 (0x8000)
  // Straight         0110 (0x6000)
  // Trips            0100 (0x4000)
  // Two Pairs        0010 (0x2000)
  // Pairs/High Card  0000 (0x0000)
  if (prefix == 0) {
    uint16_t second_half = result & 0xFFFF;
    vector<int> kicker_bits = get_list_of_bits(second_half);
    uint16_t first_half = result >> 16;
    vector<int> pair_bits = get_list_of_bits(first_half);
    if (pair_bits.size() > 1) {
      cout << "[!] Error: Pairs/High Card should have at most one pair!" << endl;
    } else if (pair_bits.size() * 2 + kicker_bits.size() != 5) {
      cout << "[!] Error: " << pair_bits.size() << " pairs and " << kicker_bits.size() << " kickers!" << endl;
    } else if (pair_bits.size() == 1) {
      cout << "Pair: " << RANKS[pair_bits[0]] << " with kicker " << RANKS[kicker_bits[2]] << ", " << RANKS[kicker_bits[1]] << " and " << RANKS[kicker_bits[0]] << endl;
    } else {
      cout << "High Card: " << RANKS[kicker_bits[4]] << ", " << RANKS[kicker_bits[3]] << ", " << RANKS[kicker_bits[2]] << ", " << RANKS[kicker_bits[1]] << " and " << RANKS[kicker_bits[0]] << endl;
    }
  } else if (prefix == 0x2) {
    uint16_t second_half = result & 0xFFFF;
    vector<int> kicker_bits = get_list_of_bits(second_half);
    if (kicker_bits.size() != 1) {
      cout << "[!] Error: 2 pairs and " << kicker_bits.size() << " kickers!" << endl;
    } else {
      uint16_t top_half = result >> 16;
      vector<int> pair_bits = get_list_of_bits(top_half);
      if (pair_bits.size() != 2) {
        cout << "[!] Error: 2 pairs code but " << pair_bits.size() << " pairs!" << endl;
      } else {
        cout << "Two Pairs: " << RANKS[pair_bits[1]] << " and " << RANKS[pair_bits[0]] << " with kicker " << RANKS[kicker_bits[0]] << endl;
      }
    }
  } else if (prefix == 0x4) {
    uint16_t second_half = result & 0xFFFF;
    vector<int> kicker_bits = get_list_of_bits(second_half);
    if (kicker_bits.size() != 2) {
      cout << "[!] Error: Trips should have exactly two kickers!" << endl;
    } else {
      uint16_t top_half = result >> 16;
      vector<int> bits = get_list_of_bits(top_half);
      if (bits.size() != 1) {
        cout << "[!] Error: Trips should have only one top card!"
             << endl;
      } else {
        cout << "Trips: " << RANKS[bits[0]] << " with kicker " << RANKS[kicker_bits[1]] << " and " << RANKS[kicker_bits[0]] << endl;
      }
    }
  } else if (prefix == 0x6) {
    uint16_t second_half = result & 0xFFFF;
    if (second_half != 0) {
      cout << "[!] Error: Straight should have no kicker!" << endl;
    } else {
      uint16_t top_half = result >> 16;
      vector<int> bits = get_list_of_bits(top_half);
      if (bits.size() != 1) {
        cout << "[!] Error: Straight should have only one top card!"
             << endl;
      } else {
        cout << "Straight: " << RANKS[bits[0]] << endl;
      }
    }
  } else if (prefix == 0x8) {
    uint16_t second_half = result & 0xFFFF;
    if (second_half != 0) {
      cout << "[!] Error: Flush should have no kicker!" << endl;
    } else {
      uint16_t top_half = result >> 16;
      vector<int> bits = get_list_of_bits(top_half);
      if (bits.size() != 5) {
        cout << "[!] Error: Flush should have exactly 5 cards!"
             << endl;
      } else {
        cout << "Flush: " << RANKS[bits[4]] << ", " << RANKS[bits[3]] << ", " << RANKS[bits[2]] << ", " << RANKS[bits[1]] << ", " << RANKS[bits[0]] << endl;
      }
    }
  } else if (prefix == 0xA) {
    uint16_t second_half = result & 0xFFFF;
    vector<int> pairs_bits = get_list_of_bits(second_half);
    if (pairs_bits.size() != 1) {
      cout << "[!] Error: Full house should have only one pair!" << endl;
    } else {
      uint16_t top_half = result >> 16;
      vector<int> trips_bits = get_list_of_bits(top_half);
      if (trips_bits.size() != 1) {
        cout << "[!] Error: Full house should have only one trips!"
             << endl;
      } else {
        cout << "Full House: " << RANKS[trips_bits[0]] << " over " << RANKS[pairs_bits[0]]
             << endl;
      }
    }
  } else if (prefix == 0xC) {
    uint16_t second_half = result & 0xFFFF;
    vector<int> kicker_bits = get_list_of_bits(second_half);
    if (kicker_bits.size() != 1) {
      cout << "[!] Error: Quads should have only one kicker!" << endl;
    } else {
      uint16_t top_half = result >> 16;
      vector<int> bits = get_list_of_bits(top_half);
      if (bits.size() != 1) {
        cout << "[!] Error: Quads should have only one top card!" << endl;
      } else {
        cout << "Quads: " << RANKS[bits[0]] << " with kicker " << RANKS[kicker_bits[0]] << endl;
      }
    }
  } else if (prefix == 0xE) {
    uint16_t second_half = result & 0xFFFF;
    if (second_half != 0) {
      cout << "[!] Error: Straight flush should have no kicker!" << endl;
    } else {
      uint16_t top_half = result >> 16;
      vector<int> bits = get_list_of_bits(top_half);
      if (bits.size() != 1) {
        cout << "[!] Error: Straight flush should have only one top card!"
             << endl;
      } else {
        cout << "Straight Flush: " << RANKS[bits[0]] << endl;
      }
    }
  } else {
    cout << "[!] Error: Invalid result!" << endl;
  }
  return result;
}

// returns 0 -> 169, left to right up to down like a matrix
// AA  ... A3s A2s
// AKo ... K3s K2s
// ... ... ... ...
// A2o ... 32o 22
// have to assume that there are exactly 2 cards in the hand
// idk if assertion ruins performance
int get_canonical_hand_from_int(int c1, int c2) {
  int r1 = 12 - c1 / 4;
  int s1 = c1 % 4;
  int r2 = 12 - c2 / 4;
  int s2 = c2 % 4;

  if (r1 > r2) {
    r1 = r1 + r2;
    r2 = r1 - r2;
    r1 = r1 - r2;
  }

  if (s1 != s2) {
    // off suit
    return r2 * 13 + r1;
  }
  else {
    return r1 * 13 + r2;
  }
}

string get_canonical_from_idx(int idx) {
  string base = "";
  int row = idx / 13;
  int col = idx % 13;
  if (row < col) {
    return base + RANKS[row] + RANKS[col] + 's';
  }
  if (row > col) {
    return base + RANKS[col] + RANKS[row] + 'o';
  }
  return base + RANKS[col] + RANKS[row];
}