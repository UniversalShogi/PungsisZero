#ifndef MCTSMOVEPROVIDER_H
#define MCTSMOVEPROVIDER_H

#include "../actionprovider.h"
#include "../search/mcts.h"
#include "../search/mctsmodel.h"
#include "../board.h"

#include <vector>

class MCTSActionProvider : public ActionProvider {
    public:
    MCTS mcts;
    int depth;

    MCTSActionProvider(MCTSModel model, Board initialState, int depth) : mcts(model, initialState), depth(depth) {}

    Action nextAction(std::vector<Board>& history) override;
    void opponentMove(Action action) override;
};

#endif