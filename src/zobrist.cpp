#include "zobrist.h"

#include "board.h"
#include "basics.h"
#include "intrinsics.h"
#include <random>
#include <limits>

uint64_t PIECE_RANDOM[PIECE_NUMBER][FILE_NUMBER * RANK_NUMBER];
uint64_t DROP_RANDOM[DROP_NUMBER][FILE_NUMBER * 2];

void Zobrist::init() {
    std::random_device rd;
    std::default_random_engine eng(rd());
    std::uniform_int_distribution<uint64_t> dis(std::numeric_limits<uint64_t>::min(), std::numeric_limits<uint64_t>::max());

    for (int i = 0; i < PIECE_NUMBER; i++)
        for (int j = 0; j < FILE_NUMBER * RANK_NUMBER; j++)
            PIECE_RANDOM[i][j] = dis(eng);
    for (int i = 0; i < DROP_NUMBER; i++)
        for (int j = 0; j < FILE_NUMBER * 2; j++)
            DROP_RANDOM[i][j] = dis(eng);
}