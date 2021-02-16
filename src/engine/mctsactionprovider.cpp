#include "mctsactionprovider.h"
#include "../action.h"
#include "../board.h"

#include <string>
#include <vector>
#include <random>
#include <math.h>

Action MCTSActionProvider::nextAction(std::vector<Board>& history) {
    if (history.size() < this->naivePlayout)
        return this->mcts->selectByNaivePolicy();
    std::discrete_distribution<int> dis({bigSimuProb, 1 - bigSimuProb});
    this->mcts->search(dis(this->mcts->eng) == 0 ? this->bigDepth : this->smallDepth);
    if (temp > endTemp)
        temp *= pow(0.5, 1.0 / halfLife);

    if (temp <= endTemp)
        temp = endTemp;
    return this->mcts->selectByProportionalPolicy(temp);
}

void MCTSActionProvider::opponentMove(Action action) {
    this->mcts->opponentMove(action);
}

std::string MCTSActionProvider::getName() const {
    return this->name;
}