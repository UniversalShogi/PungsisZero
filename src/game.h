#ifndef GAME_H
#define GAME_H

#include "board.h"
#include <unordered_set>

class Game {
    public:
    std::unordered_set<Board> history;
    Board current;
};

#endif