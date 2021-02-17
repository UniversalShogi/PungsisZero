#include "mctsactionprovider.h"
#include "../action.h"
#include "../board.h"

#include <string>
#include <vector>
#include <random>
#include <math.h>

Action MCTSActionProvider::nextAction(std::vector<Board>& history) {
    Board states[4];
    Action action;

    for (int i = 0; i < 3; i++)
        states[2 - i] = history.size() >= i + 1 ? history[history.size() - i - 1] : BEMPTY;

    if (history.size() < this->naivePlayout) {
        this->mcts->stimulateSearching(states);
        action = this->mcts->selectByNaivePolicy();
    } else {
        std::discrete_distribution<int> dis({bigSimuProb, 1 - bigSimuProb});
        bool isBig = dis(this->mcts->eng) == 0;
        if (isBig)
            samples.push_back(this->mcts->searchingNode);
        
        this->mcts->search(states, isBig ? this->bigDepth : this->smallDepth);
        if (temp > endTemp)
            temp *= pow(0.5, 1.0 / halfLife);

        if (temp <= endTemp)
            temp = endTemp;
        action = this->mcts->selectByProportionalPolicy(temp);
    }

    states[3] = Board(states[2]);
    states[3].inflict(states[3].currentColour, action);
    states[3].changeTurn();
    this->mcts->stimulateSearching(&states[1]);
    return action;
}

void MCTSActionProvider::opponentMove(Action action) {
    this->mcts->opponentMove(action);
}

std::string MCTSActionProvider::getName() const {
    return this->name;
}