#include "cards.hpp"
#include <cstdint>
#include <iostream>

using namespace std;

// each hand is a 64-bit number, every 16 bits are the ranks, 3 MSBs are ignored
// shift left by 1 to make space, then copy the 13th bit to the 0th bit
uint16_t correct_ace(uint16_t num) {
  return (num << 1) | ((num & 0x1000) >> 12);
}

// keep only the most significant nonzero bit
uint16_t keep_top_bit(uint16_t num) {
  num |= num >> 1;
  num |= num >> 2;
  num |= num >> 4;
  num |= num >> 8;
  return num - (num >> 1);
}

uint32_t get_max(uint32_t a, uint32_t b) {
  return a ^ ((a ^ b) & -(a < b));
}

uint32_t get_min(uint32_t a, uint32_t b) {
  return a ^ ((a ^ b) & -(a > b));
}

// uint32_t is_all_zeros(uint16_t x) {
//   uint32_t y = -x;  // Zero: MSB=0, non-zeros: MSB=1
//   y = (~y) >> 31;   // Zero: MSB=1, non-zeros: MSB=0
//   return y;         // Zero: 1, non-zeros: 0
// }

// from here on, after processing the cards, there will be only 13 bits for ranks
// the LSB for Ace will be removed as the 13->12th bit already represents Ace

// hand ranking:
// Straight Flush   1110 (0xE000)
// Quads            1100 (0xC000)
// Full House       1010 (0xA000)
// Flush            1000 (0x8000)
// Straight         0110 (0x6000)
// Trips            0100 (0x4000)
// Two Pairs        0010 (0x2000)
// Pairs/High Card  0000 (0x0000)

// get the straight for any hand, suit-neutral
uint16_t get_straight_helper(uint16_t hand) {
  uint16_t straight_hand = correct_ace(hand);
  uint16_t straight = 0;
  for (int i = 0; i < 14 - 4; i++) {
    straight |= (((straight_hand & 0x1F) == 0x1F) << (i + 4));
    straight_hand >>= 1;
  }
  // remove the Ace bit
  straight >>= 1;
  return straight;
}

// get the flush for any hand, suit-neutral
// basically make sure that there are at least 5 cards of the same suit
// keep only the top 5 bits even if there are more
uint16_t get_flush_helper(uint16_t hand) {
  uint16_t flush = 0;
  int count = 0;
  for (int i = 12; i >= 0; i--) {
    uint16_t bit = (hand >> i) & 1;
    count += bit;
    flush |= (bit * (count <= 5)) << i;
  }
  // only keep flush track if there is at least/exactly 5 cards of the same suit
  return flush * (count >= 5);
}

// straight/flush: first 3 bit denotes straight/flush/both, next 13 bits are top rank
uint32_t get_straight_flush(uint64_t hand) {
  uint16_t straight_merge = 0;
  uint16_t straight_flush = 0;
  uint16_t flush = 0;
  for (int i = 0; i < 4; i++) {
    uint16_t cards_per_suit = hand & 0xFFFFU;
    straight_merge |= cards_per_suit;
    straight_flush |= get_straight_helper(cards_per_suit);
    flush |= get_flush_helper(cards_per_suit);
    hand >>= 16;
  }
  uint16_t straight = get_straight_helper(straight_merge);
  straight_flush = keep_top_bit(straight_flush);
  straight = keep_top_bit(straight);

  // return the first non-zero value
  straight_flush = (0xE000 | straight_flush) * (straight_flush != 0) + (straight_flush == 0) * 
        ((0x8000 | flush) * (flush != 0) + (flush == 0) * (0x6000 | straight) * (straight != 0));

  // no kicker
  return ((uint32_t) straight_flush) << 16;
}

// quads: first 16 bits are quads, next 16 bits are kicker
// prefix: 100
uint32_t get_quads(uint64_t hand) {
  // 13 bits for all cards
  uint16_t quads = 0x1FFFU;
  for (int i = 0; i < 4; i++) {
    quads &= (hand >> (i * 16)) & 0xFFFFU;
  }
  uint16_t kicker = 0;
  for (int i = 0; i < 4; i++) {
    kicker |= (hand >> (i * 16)) & 0xFFFFU;
  }
  // remove quads from kicker
  kicker &= ~quads;
  // there should only be at most 1 quads -> no need to keep top bit
  // keep only the max kicker by keeping the most significant bit
  kicker = keep_top_bit(kicker);
  return ((((uint32_t) (0xC000 | quads)) << 16) | kicker) * (quads != 0);
}

// trips/pairs: first 16 bits are trips/pairs, next 16 bits are kicker
// 011 prefix if both trips and pairs (full house), 010 if trips, 000 if pairs or none
// if 000 and no pairs, the pair's top-16 bits are zero
uint32_t get_trips_pairs(uint64_t hand) {
  uint16_t suit1 = hand & 0xFFFFU;
  uint16_t suit2 = (hand >> 16) & 0xFFFFU;
  uint16_t suit3 = (hand >> 32) & 0xFFFFU;
  uint16_t suit4 = (hand >> 48) & 0xFFFFU;
  uint16_t trips = (
    (suit1 & suit2 & suit3) |
    (suit1 & suit2 & suit4) |
    (suit1 & suit3 & suit4) |
    (suit2 & suit3 & suit4)
  );
  uint16_t pairs = (
    (suit1 & suit2) |
    (suit1 & suit3) |
    (suit1 & suit4) |
    (suit2 & suit3) |
    (suit2 & suit4) |
    (suit3 & suit4)
  );
  uint16_t kicker = suit1 | suit2 | suit3 | suit4;
  // if you have 2 trips, one is counted as pairs
  trips = keep_top_bit(trips);
  // remove trips from pairs
  pairs &= ~trips;

  // if both trips and pairs are not zero, it's a full house
  uint32_t full_house = (trips != 0) * (pairs != 0) * ((((uint32_t) (0xA000 | trips)) << 16) | keep_top_bit(pairs));
  
  // if only trips are not zero, it's trips
  uint16_t trips_kicker = kicker & ~trips;
  // keep 2 bits for kicker
  uint16_t trips_kicker_top = keep_top_bit(trips_kicker);
  trips_kicker_top = keep_top_bit(trips_kicker & (~trips_kicker_top)) | trips_kicker_top;
  uint32_t trips_final = (trips != 0) * ((((uint32_t) (0x4000 | trips)) << 16) | trips_kicker_top);

  // consider the pair case: keep at most 2 pairs
  uint16_t pairs_top = keep_top_bit(pairs);
  uint16_t pairs_bottom = keep_top_bit(pairs & (~pairs_top));
  pairs = pairs_top | pairs_bottom;
  // get the kicker
  uint16_t pairs_kicker = kicker & ~pairs;
  uint16_t pairs_kicker_top = keep_top_bit(pairs_kicker);
  pairs_kicker = pairs_kicker & ~pairs_kicker_top;
    
  // to keep the current top kicker and add to _top if there's still slots
  // adding 0 kickers if two pairs, 2 if one pair, 4 if no pairs
  uint16_t kicker_count = ((pairs_top == 0) + (pairs_bottom == 0)) * 2;
  for (int i = 0; i < 4; i++) {
    uint16_t pairs_kicker_temp = keep_top_bit(pairs_kicker);
    pairs_kicker = pairs_kicker & ~pairs_kicker_temp;
    pairs_kicker_top |= pairs_kicker_temp * (i < kicker_count);
  }
  uint32_t pairs_final = (((uint32_t) ((0x2000 * (pairs_bottom != 0)) | pairs)) << 16) | pairs_kicker_top;
  return full_house + (full_house == 0) * (trips_final + (trips_final == 0) * pairs_final);
}

uint32_t get_hand_value(uint64_t hand) {
  uint32_t straight_flush = get_straight_flush(hand);
  uint32_t quads = get_quads(hand);
  uint32_t trips_pairs = get_trips_pairs(hand);
  return get_max(straight_flush, get_max(quads, trips_pairs));
}

uint8_t get_canonical_hand(uint64_t hand) {
  // returns 0 -> 169, left to right up to down like a matrix
  // AA  ... A3s A2s
  // AKo ... K3s K2s
  // ... ... ... ...
  // A2o ... 32o 22
  // have to assume that there are exactly 2 cards in the hand
  // idk if assertion ruins performance

  uint8_t ret = 0;
  uint8_t found = 0;

  for (int i = 0; i < 4; i++) {
    uint8_t suit_acc = 0;
    uint8_t suit_count = 0;
    for (int j = 12; j >= 0; j--) {
      uint16_t bit = (hand >> (12 - j)) & 1;
      suit_acc += j * bit * (1 + suit_count * 12);
      suit_count += bit;
    }
    ret += suit_acc + (found > 0) * (suit_count > 0) * get_max(ret, suit_acc) * 12;
    found += suit_count;
    hand >>= 16;
  }

  return ret;
}
  