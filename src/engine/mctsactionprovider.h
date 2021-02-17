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
    MCTS* mcts;
    int smallDepth;
    int bigDepth;
    double startTemp;
    double endTemp;
    int halfLife;
    int naivePlayout;
    double bigSimuProb;
    double temp;
    std::string name;
    std::vector<MCTSNode*> samples;

    MCTSActionProvider(MCTS* mcts, int smallDepth, int bigDepth, std::string name,
        double startTemp = 0.8, double endTemp = 0.2, int halfLife = 15, int naivePlayout = 14, double bigSimuProb = 0.25)
        : mcts(mcts), smallDepth(smallDepth), bigDepth(bigDepth), name(name), startTemp(startTemp)
        , endTemp(endTemp), halfLife(halfLife), naivePlayout(naivePlayout), bigSimuProb(bigSimuProb), temp(startTemp) {}

    Action nextAction(std::vector<Board>& history) override;
    void opponentMove(Action action) override;
    std::string getName() const override;

    ~MCTSActionProvider() override {
        delete mcts;
    }
};

#endif