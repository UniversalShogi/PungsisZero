#ifndef GAME_H
#define GAME_H

#include "board.h"
#include <vector>

class Game {
    public:
    std::vector<Board> history;
    Board current;
};

#endif