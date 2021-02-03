#ifndef ACTION_H
#define ACTION_H

#include <string>

class MoveAction {
    public:
    int piece;
    int src;
    int dst;
    bool promote;

    operator std::string() {
        std::string out = std::to_string(this->src / 9) + ',' + std::to_string(this->src % 9) + "->" + std::to_string(this->dst / 9) + ',' + std::to_string(this->dst % 9);
        if (this->promote) out += 'P';
        return out;
    }

    operator int() {
        return 0 | piece << 1 | src << 6 | dst << 13 | promote << 20;
    }
};

class DropAction {
    public:
    int graveIndex;
    int dst;

    operator std::string() {
        std::string out = std::to_string(this->graveIndex) + "->" + std::to_string(this->dst / 9) + ',' + std::to_string(this->dst % 9);
        return out;
    }

    operator int() {
        return 1 | graveIndex << 1 | dst << 5;
    }
};

#endif