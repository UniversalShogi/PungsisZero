#include "basics.h"

int PIECE_POWER[PIECE_NUMBER][8] = {
    { N },
    { NE, N, NW, E, W, S },
    {},
    { NE, N, NW, E, W, S },
    { NNE, NNW },
    { NE, N, NW, E, W, S },
    { NE, N, NW, SE, SW },
    { NE, N, NW, E, W, S },
    { NE, N, NW, E, W, S },
    {},
    { N, E, W, S },
    {},
    { NE, NW, SE, SW },
    { NE, N, NW, E, W, SE, S, SW },
    { S },
    { N, E, W, SE, S, SW },
    {},
    { N, E, W, SE, S, SW },
    { SSE, SSW },
    { N, E, W, SE, S, SW },
    { NE, NW, SE, S, SW },
    { N, E, W, SE, S, SW },
    { N, E, W, SE, S, SW },
    {},
    { N, E, W, S },
    {},
    { NE, NW, SE, SW },
    { NE, N, NW, E, W, SE, S, SW }
};

int NAIVE_SETUP[FILE_NUMBER][RANK_NUMBER] = {
    { G_LANCE, NONE, G_PAWN, NONE, NONE, NONE, S_PAWN, NONE, S_LANCE },
    { G_KNIGHT, G_BISHOP, G_PAWN, NONE, NONE, NONE, S_PAWN, S_ROOK, S_KNIGHT },
    { G_SILVER, NONE, G_PAWN, NONE, NONE, NONE, S_PAWN, NONE, S_SILVER },
    { G_GOLD, NONE, G_PAWN, NONE, NONE, NONE, S_PAWN, NONE, S_GOLD },
    { G_KING, NONE, G_PAWN, NONE, NONE, NONE, S_PAWN, NONE, S_KING },
    { G_GOLD, NONE, G_PAWN, NONE, NONE, NONE, S_PAWN, NONE, S_GOLD },
    { G_SILVER, NONE, G_PAWN, NONE, NONE, NONE, S_PAWN, NONE, S_SILVER },
    { G_KNIGHT, G_ROOK, G_PAWN, NONE, NONE, NONE, S_PAWN, S_BISHOP, S_KNIGHT },
    { G_LANCE, NONE, G_PAWN, NONE, NONE, NONE, S_PAWN, NONE, S_LANCE },
};