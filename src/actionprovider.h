#ifndef MOVEPROVIDER_H
#define MOVEPROVIDER_H

#include "action.h"
#include "board.h"

#include <string>
#include <vector>

class ActionProvider {
    public:
    virtual Action nextAction(std::vector<Board>& history) = 0;
    virtual void opponentMove(Action action) = 0;
    virtual std::string getName() const = 0;

    virtual ~ActionProvider() {};
};

#endif