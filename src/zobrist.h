#ifndef ZOBRIST_H
#define ZOBRIST_H

#include "board.h"
#include "basics.h"
#include "intrinsics.h"

extern uint64_t PIECE_RANDOM[PIECE_NUMBER][FILE_NUMBER * RANK_NUMBER];
extern uint64_t DROP_RANDOM[DROP_NUMBER][FILE_NUMBER * 2];

class Zobrist {
    public:

    static void init();
    
    std::size_t operator()(const Board& board) const {
        uint64_t hash = 0ULL;

        for (int i = 0; i < PIECE_NUMBER; i++)
            board.pieces[i].forEach([&](int src) {
                hash ^= PIECE_RANDOM[i][src];
            });
        for (int i = 0; i < DROP_NUMBER; i++)
            hash ^= DROP_RANDOM[i][board.graveInfo[i]];
        
        return hash;
    }
};

#endif