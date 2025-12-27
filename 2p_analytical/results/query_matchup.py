import json
data = json.load(open('canonical_probabilities_headsup.json'))
print('Loaded database.')

RANKS = "AKQJT98765432"

while True:
    print()
    hand1 = input("Hero's hand: ")
    hand1 = hand1[:2].upper() + hand1[2:].lower()
    if RANKS.index(hand1[0]) > RANKS.index(hand1[1]):
        hand1 = hand1[1] + hand1[0] + hand1[2:]
    if hand1 not in data:
        print('Not a valid hand!')
        continue

    hand2 = input("Villain's hand: ")
    hand2 = hand2[:2].upper() + hand2[2:].lower()
    if RANKS.index(hand2[0]) > RANKS.index(hand2[1]):
        hand2 = hand2[1] + hand2[0] + hand2[2:]
    if hand2 not in data:
        print('Not a valid hand!')
        continue

    win = data[hand1][hand2]
    lose = data[hand2][hand1]

    print(f'{hand1} vs {hand2}')
    print(f'Win : {win:7.4f}%')
    print(f'Lose: {lose:7.4f}%')
    print(f'Tie : {100 - win - lose:7.4f}%')
