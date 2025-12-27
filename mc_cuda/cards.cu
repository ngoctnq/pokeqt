#include "cards.hpp"

// keep only the most significant nonzero bit
__device__ uint32_t keep_top_bit(uint32_t num) {
  return 1ULL << (31 - __clz(num));
}

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
__device__ uint32_t get_straight_helper(uint32_t hand) {
  // shift left by 1 to make space, then copy the 13th bit to the 0th bit
  uint32_t straight_hand = (hand << 1) | ((hand & 0x1000) >> 12);
  uint32_t straight = 0;
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
__device__ uint32_t get_flush_helper(uint32_t hand) {
  uint32_t idx5 = __fns(hand, 31, -5);
  return (idx5 > 31) ? 0 : (hand >> idx5) << idx5;
}

// straight/flush: first 3 bit denotes straight/flush/both, next 13 bits are top rank
__device__ uint32_t get_straight_flush(uint64_t hand) {
  uint32_t straight_merge = 0;
  uint32_t straight_flush = 0;
  uint32_t flush = 0;
  for (int i = 0; i < 4; i++) {
    uint32_t cards_per_suit = hand & 0xFFFFU;
    straight_merge |= cards_per_suit;
    straight_flush |= get_straight_helper(cards_per_suit);
    flush |= get_flush_helper(cards_per_suit);
    hand >>= 16;
  }
  uint32_t straight = get_straight_helper(straight_merge);
  straight_flush = keep_top_bit(straight_flush);
  straight = keep_top_bit(straight);

  // return the first non-zero value
  straight_flush = straight_flush ? (0xE000 | straight_flush) :
        (flush ? (0x8000 | flush) : (straight ? (0x6000 | straight) : 0));

  // no kicker
  return straight_flush << 16;
}

// quads: first 16 bits are quads, next 16 bits are kicker
// prefix: 100
__device__ uint32_t get_quads(uint64_t hand) {
  // 13 bits for all cards
  uint32_t quads = 0x1FFF;
  for (int i = 0; i < 4; i++) {
    quads &= (hand >> (i * 16)) & 0xFFFFU;
  }
  uint32_t kicker = 0;
  for (int i = 0; i < 4; i++) {
    kicker |= (hand >> (i * 16)) & 0xFFFFU;
  }
  // remove quads from kicker
  kicker &= ~quads;
  // there should only be at most 1 quads -> no need to keep top bit
  // keep only the max kicker by keeping the most significant bit
  kicker = keep_top_bit(kicker);
  return quads ? ((0xC000 | quads) << 16) | kicker : 0;
}

// trips/pairs: first 16 bits are trips/pairs, next 16 bits are kicker
// 011 prefix if both trips and pairs (full house), 010 if trips, 000 if pairs or none
// if 000 and no pairs, the pair's top-16 bits are zero
__device__ uint32_t get_trips_pairs(uint64_t hand) {
  uint32_t suit1 = hand & 0xFFFFU;
  uint32_t suit2 = (hand >> 16) & 0xFFFFU;
  uint32_t suit3 = (hand >> 32) & 0xFFFFU;
  uint32_t suit4 = (hand >> 48) & 0xFFFFU;
  uint32_t trips = (
    (suit1 & suit2 & suit3) |
    (suit1 & suit2 & suit4) |
    (suit1 & suit3 & suit4) |
    (suit2 & suit3 & suit4)
  );
  uint32_t pairs = (
    (suit1 & suit2) |
    (suit1 & suit3) |
    (suit1 & suit4) |
    (suit2 & suit3) |
    (suit2 & suit4) |
    (suit3 & suit4)
  );
  uint32_t kicker = suit1 | suit2 | suit3 | suit4;
  // if you have 2 trips, one is counted as pairs
  trips = keep_top_bit(trips);
  // remove trips from pairs
  pairs &= ~trips;

  // if both trips and pairs are not zero, it's a full house
  uint32_t full_house = (trips && pairs) ? ((0xA000 | trips) << 16) | keep_top_bit(pairs) : 0;
  
  // if only trips are not zero, it's trips
  uint32_t trips_kicker = kicker & ~trips;
  // keep 2 bits for kicker
  uint32_t trips_kicker_top = keep_top_bit(trips_kicker);
  trips_kicker_top = keep_top_bit(trips_kicker & (~trips_kicker_top)) | trips_kicker_top;
  uint32_t trips_final = trips ? ((0x4000 | trips) << 16) | trips_kicker_top : 0;

  // consider the pair case: keep at most 2 pairs
  uint32_t pairs_top = keep_top_bit(pairs);
  uint32_t pairs_bottom = keep_top_bit(pairs & (~pairs_top));
  pairs = pairs_top | pairs_bottom;
  // get the kicker
  uint32_t pairs_kicker = kicker & ~pairs;
  uint32_t pairs_kicker_top = keep_top_bit(pairs_kicker);
  pairs_kicker = pairs_kicker & ~pairs_kicker_top;
    
  // to keep the current top kicker and add to _top if there's still slots
  // adding 0 kickers if two pairs, 2 if one pair, 4 if no pairs
  uint32_t kicker_count = ((pairs_top == 0) + (pairs_bottom == 0)) * 2;
  for (int i = 0; i < 4; i++) {
    uint32_t pairs_kicker_temp = keep_top_bit(pairs_kicker);
    pairs_kicker = pairs_kicker & ~pairs_kicker_temp;
    pairs_kicker_top |= pairs_kicker_temp * (i < kicker_count);
  }
  uint32_t pairs_final = (((pairs_bottom ? 0x2000 : 0) | pairs) << 16) | pairs_kicker_top;
  return full_house ? full_house : (trips_final ? trips_final : pairs_final);
}

__device__ uint32_t get_hand_value(uint64_t hand) {
  uint32_t straight_flush = get_straight_flush(hand);
  uint32_t quads = get_quads(hand);
  uint32_t trips_pairs = get_trips_pairs(hand);
  return (straight_flush > quads) ? ((straight_flush > trips_pairs) ? straight_flush : trips_pairs)
                                  : ((quads > trips_pairs) ? quads : trips_pairs);
}

__device__ uint32_t get_canonical_hand(uint64_t hand) {
  // returns 0 -> 169, left to right up to down like a matrix
  // AA  ... A3s A2s
  // AKo ... K3s K2s
  // ... ... ... ...
  // A2o ... 32o 22
  // have to assume that there are exactly 2 cards in the hand
  int bit1_idx = __ffsll(hand);
  int bit2_idx = __ffsll(hand >> bit1_idx) + bit1_idx;
  int rank1 = ((64 - bit1_idx) % 16) - 3;
  int rank2 = ((64 - bit2_idx) % 16) - 3;
  int suit1 = bit1_idx / 16;
  int suit2 = bit2_idx / 16;
  return (rank1 + rank2) + (
    (suit1 == suit2) ? (rank1 < rank2 ? rank1 : rank2) :
    (rank1 < rank2 ? rank2 : rank1)) * 12;
}
