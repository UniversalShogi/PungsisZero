#ifndef MCTSMOVEPROVIDER_H
#define MCTSMOVEPROVIDER_H

#include "../actionprovider.h"
#include "../search/mcts.h"
#include "../search/mctsmodel.h"
#include "../board.h"

#include <vector>
#include <string>

class MCTSActionProvider : public ActionProvider {
    public:
    MCTS mcts;
    int depth;
    std::string name;

    MCTSActionProvider(MCTSModel model, Board initialState, int depth, std::string name) : mcts(model, initialState), depth(depth), name(name) {}

    Action nextAction(std::vector<Board>& history) override;
    void opponentMove(Action action) override;
    std::string getName() const override;
};

#endif