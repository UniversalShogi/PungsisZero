#ifndef ACTION_H
#define ACTION_H

#include <string>

#include "basics.h"

class MoveAction {
    public:
    int piece;
    int src;
    int dst;
    bool promote;

    operator std::string() const {
        std::string out = std::to_string(this->src / 9) + ',' + std::to_string(this->src % 9) + "->" + std::to_string(this->dst / 9) + ',' + std::to_string(this->dst % 9);
        if (this->promote) out += 'P';
        return out;
    }

    operator int() const {
        return piece | src << 5 | dst << 12 | promote << 19;
    }
};

class DropAction {
    public:
    int graveIndex;
    int dst;

    operator std::string() const {
        std::string out = std::to_string(this->graveIndex) + "->" + std::to_string(this->dst / 9) + ',' + std::to_string(this->dst % 9);
        return out;
    }

    operator int() const {
        return graveIndex | dst << 4;
    }
};

enum ActionType {
    MOVE,
    DROP,
    RESIGN,
    ILLEGAL,
    SENNICHITE
};

extern int MODEL_OUTPUT[FILE_NUMBER * RANK_NUMBER][FILE_NUMBER * RANK_NUMBER];

class Action {
    public:
    union {
        MoveAction move;
        DropAction drop;
    };
    ActionType type;

    Action(MoveAction move) : move(move), type(MOVE) {}
    Action(DropAction drop) : drop(drop), type(DROP) {}
    Action() : type(RESIGN) {}

    operator std::string() const {
        switch (this->type) {
            case MOVE:
                return (std::string) this->move;
            case DROP:
                return (std::string) this->drop;
            case RESIGN:
                return "RESIGN";
            case ILLEGAL:
                return "ILLEGAL";
            case SENNICHITE:
                return "SENNICHITE";
            default:
                return "UNKNOWN";
        }
    }

    operator int() const {
        int hash = this->type;
        if (this->type == MOVE)
            hash |= ((int) this->move) << 3;
        else if (this->type == DROP)
            hash = ((int) this->drop) << 3;
        return hash;
    }

    bool operator==(const Action& other) const {
        return (int) *this == (int) other;
    }

    int toModelOutput() const {
        if (type == MOVE)
            return MODEL_OUTPUT[this->move.src][this->move.dst];
        else if (type == DROP)
            return this->drop.graveIndex % 7;
        else
            return -1;
    }

    int getPrincipalPosition() const {
        if (type == MOVE)
            return this->move.src;
        else if (type == DROP)
            return this->drop.dst;
        else
            return -1;
    }
};

class ActionHash {
    public:
    std::size_t operator()(const Action& action) const {
        return (int) action;
    }
};


#endif