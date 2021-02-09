#include "mctsactionprovider.h"
#include "../action.h"
#include "../board.h"

#include <vector>

Action MCTSActionProvider::nextAction(std::vector<Board>& history) {
    this->mcts.search(this->depth);
    return this->mcts.selectByGreedyPolicy();
}

void MCTSActionProvider::opponentMove(Action action) {
    this->mcts.opponentMove(action);
}