#ifndef BASICS_H
#define BASICS_H

constexpr int COLOUR_NUMBER = 2;
constexpr int DROP_NUMBER = 14;
constexpr int FILE_NUMBER = 9;
constexpr int FILE_ONE = 0;
constexpr int FILE_TWO = 1;
constexpr int FILE_NINE = 8;
constexpr int FILE_EIGHT = 7;
constexpr int RANK_NUMBER = 9;
constexpr int RANK_ONE = 0;
constexpr int RANK_TWO = 1;
constexpr int RANK_NINE = 8;
constexpr int RANK_EIGHT = 7;

constexpr char FILE_CHAR[] = "一二三四五六七八九";
constexpr char RANK_CHAR[] = "１２３４５６７８９";
constexpr char PIECE_CHAR[] = "歩と香杏桂圭銀全金角馬飛龍玉歩と香杏桂圭銀全金角馬飛龍王";
constexpr char DROP_CHAR[] = "歩香桂銀金角飛歩香桂銀金角飛";
constexpr char NOPIECE_CHAR[] = "無";

enum Piece {
    S_PAWN,
    S_TOKIN,
    S_LANCE,
    S_LANCEGOLD,
    S_KNIGHT,
    S_KNIGHTGOLD,
    S_SILVER,
    S_SILVERGOLD,
    S_GOLD,
    S_BISHOP,
    S_HORSE,
    S_ROOK,
    S_DRAGON,
    S_KING,
    G_PAWN,
    G_TOKIN,
    G_LANCE,
    G_LANCEGOLD,
    G_KNIGHT,
    G_KNIGHTGOLD,
    G_SILVER,
    G_SILVERGOLD,
    G_GOLD,
    G_BISHOP,
    G_HORSE,
    G_ROOK,
    G_DRAGON,
    G_KING,
    PIECE_NUMBER,
    NONE = -1
};

constexpr int PIECE_TO_GRAVE[PIECE_NUMBER] = { 7, 7, 8, 8, 9, 9, 10, 10, 11, 12, 12, 13, 13, -1, 0, 0, 1, 1, 2, 2, 3, 3, 4, 5, 5, 6, 6, -1 };
constexpr int GRAVE_TO_PIECE[DROP_NUMBER] = { 0, 2, 4, 6, 8, 9, 11, 14, 16, 18, 20, 22, 23, 25 };
constexpr bool PROMOTABLE_PIECE[PIECE_NUMBER] = { true, false, true, false, true, false, true, false, false, true, false, true, false, false,
    true, false, true, false, true, false, true, false, false, true, false, true, false, false };

enum QueenDirection {
    STUB,
    NE,
    N,
    NW,
    E,
    P,
    W,
    SE,
    S,
    SW
};

enum LionDirection {
    STUBL,
    NENE,
    NNE,
    NN,
    NNW,
    NWNW,
    ENE,
    NEL,
    NL,
    NWL,
    WNW,
    EE,
    EL,
    PL,
    WL,
    WW,
    ESE,
    SEL,
    SL,
    SWL,
    WSW,
    SESE,
    SSE,
    SS,
    SSW,
    SWSW
};

constexpr int PIECE_POWER[PIECE_NUMBER][8] = {
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

constexpr int NAIVE_SETUP[FILE_NUMBER][RANK_NUMBER] = {
    { G_LANCE, NONE, G_PAWN, NONE, NONE, NONE, S_PAWN, NONE, S_LANCE },
    { G_KNIGHT, G_BISHOP, G_PAWN, NONE, NONE, NONE, S_PAWN, S_ROOK, S_KNIGHT },
    { G_SILVER, NONE, G_PAWN, NONE, NONE, NONE, S_PAWN, NONE, S_SILVER },
    { G_GOLD, NONE, G_PAWN, NONE, NONE, NONE, S_PAWN, NONE, S_GOLD },
    { G_KING, NONE, G_PAWN, NONE, NONE, NONE, S_PAWN, NONE, S_KING },
    { G_GOLD, NONE, G_PAWN, NONE, NONE, NONE, S_PAWN, NONE, S_GOLD },
    { G_SILVER, NONE, G_PAWN, NONE, NONE, NONE, S_PAWN, NONE, S_SILVER },
    { G_KNIGHT, G_ROOK, G_PAWN, NONE, NONE, NONE, S_PAWN, S_BISHOP, S_KNIGHT },
    { G_LANCE, NONE, G_PAWN, NONE, NONE, NONE, S_PAWN, NONE, S_LANCE }
};

constexpr int EMPTY_BOARD[FILE_NUMBER][RANK_NUMBER] = {
    { NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE },
    { NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE },
    { NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE },
    { NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE },
    { NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE },
    { NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE },
    { NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE },
    { NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE },
    { NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE }
};

inline void calculateQueenDestination(int file, int rank, int direction, int dst[2], int distance = 1) {
    direction -= 1;
    dst[0] = file + distance * (direction % 3 - 1);
    dst[1] = rank + distance * (direction / 3 - 1);
}

inline void calculateLionDestination(int file, int rank, int direction, int dst[2]) {
    direction -= 1;
    dst[0] = file + direction % 5 - 2;
    dst[1] = rank + direction / 5 - 2;
}

inline int colourOf(int piece) {
    return piece >= PIECE_NUMBER / 2;
}

inline int dropColourOf(int piece) {
    return piece >= DROP_NUMBER / 2;
}

inline bool isBishopLike(int piece) {
    return piece == S_BISHOP || piece == S_HORSE || piece == G_BISHOP || piece == G_HORSE;
}

inline bool isRookLike(int piece) {
    return piece == S_ROOK || piece == S_DRAGON || piece == G_ROOK || piece == G_DRAGON;
}

inline bool isLanceLike(int piece) {
    return piece == S_LANCE || piece == G_LANCE;
}

inline bool isKnightLike(int piece) {
    return piece == S_KNIGHT || piece == G_KNIGHT;
}

inline bool isPawnLike(int piece) {
    return piece == S_PAWN || piece == G_PAWN;
}

inline bool isKingLike(int piece) {
    return piece == S_KING || piece == G_KING;
}

inline int opposingRank(int colour, int from) {
    from = colour == 0 ? from : -from;
    return colour * 8 + from;
}

inline bool isInBound(int file, int rank) {
    return FILE_ONE <= file && file <= FILE_NINE
        && RANK_ONE <= rank && rank <= RANK_NINE;
}

inline int promote(int piece) {
    return piece + 1;
}

inline int kingOf(int colour) {
    return colour == 0 ? S_KING : G_KING;
}

inline int bishopOf(int colour) {
    return colour == 0 ? S_BISHOP : G_BISHOP;
}

inline int horseOf(int colour) {
    return colour == 0 ? S_HORSE : G_HORSE;
}

inline int rookOf(int colour) {
    return colour == 0 ? S_ROOK : G_ROOK;
}

inline int dragonOf(int colour) {
    return colour == 0 ? S_DRAGON : G_DRAGON;
}

inline int lanceOf(int colour) {
    return colour == 0 ? S_LANCE : G_LANCE;
}

inline int pawnOf(int colour) {
    return colour == 0 ? S_PAWN : G_PAWN;
}

#endif