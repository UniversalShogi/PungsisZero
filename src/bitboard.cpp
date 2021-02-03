#include "bitboard.h"

BitBoard BB_NONE;
BitBoard BB_ALL = BitBoard(0x1FFFFULL, 0xFFFFFFFFFFFFFFFFULL);

BitBoard::operator std::string() {
    std::string output;

    for (int r = RANK_ONE; r <= RANK_NINE; r++) {
        for (int f = FILE_NINE; f >= FILE_ONE; f--)
            output += this->test(toBBIndex(f, r)) ? 'O' : 'X';
        output += '\n';
    }
    
    return output;
}