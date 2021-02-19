#ifndef MCTSMOVEPROVIDER_H
#define MCTSMOVEPROVIDER_H

#include "../actionprovider.h"
#include "../search/mcts.h"
#include "../search/mctsmodel.h"
#include "../board.h"
#include "../action.h"

#include <string>
#include <vector>
#include <random>
#include <math.h>

template <int n>
class MCTSActionProvider : public ActionProvider {
    public:
    MCTS<n>* mcts;
    int smallDepth;
    int bigDepth;
    double startTemp;
    double endTemp;
    int halfLife;
    int naivePlayout;
    double bigSimuProb;
    double temp;
    std::string name;
    std::vector<MCTSNode<n>*> samples;

    MCTSActionProvider(MCTS<n>* mcts, int smallDepth, int bigDepth, std::string name,
        double startTemp = 0.8, double endTemp = 0.2, int halfLife = 15, int naivePlayout = 14, double bigSimuProb = 0.25)
        : mcts(mcts), smallDepth(smallDepth), bigDepth(bigDepth), name(name), startTemp(startTemp)
        , endTemp(endTemp), halfLife(halfLife), naivePlayout(naivePlayout), bigSimuProb(bigSimuProb), temp(startTemp) {}

    Action nextAction(std::vector<Board>& history) override {
        Board states[n + 1];
        Action action;

        for (int i = 0; i < n; i++)
            states[n - i - 1] = history.size() >= i + 1 ? history[history.size() - i - 1] : BEMPTY;

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

        states[n] = Board(states[n - 1]);
        states[n].inflict(states[n].currentColour, action);
        states[n].changeTurn();
        this->mcts->stimulateSearching(&states[1]);
        return action;
    }

    void opponentMove(Action action) override {
        this->mcts->opponentMove(action);
    }

    std::string getName() const override {
        return this->name;
    }

    ~MCTSActionProvider() override {
        delete mcts;
    }
};

#endif