#include "batchedselfplay.h"
#include "action.h"
#include "board.h"
#include "search/mctsmodel.h"

#include <gsl/gsl_rng.h>
#include <gsl/gsl_randist.h>
#include <torch/torch.h>
#include <vector>
#include <time.h>
#include <iostream>
#include <utility>
#include <random>
#include <math.h>
#include <limits>

bool BatchedMCTSTree::search(std::vector<Board>& states) {
    searchingNode->N += 1;
    if (!searchingNode->expanded)
        return true;
    else if (searchingNode->lost)
        return false;
    else {
        Action bestAction;
        BatchedMCTSNode* bestChild = nullptr;
        float bestPuct = -std::numeric_limits<float>::infinity();

        for (auto& [action, child] : searchingNode->childs) {
            float Q = child->N == 0 ? searchingNode->Q - (searchingNode == searchingNode ? fpuRoot : fpuNonRoot) * sqrt(searchingNode->childP) : child->Q;
            float puct = Q + puctConstant * child->P * sqrt(searchingNode->N - 1 + 0.01f)/(1 + child->N);

            if (forcedPlayoutEnabled && searchingNode == rootNode
                && child->N > 0 && child->N < sqrt(forcedSimuConstant * child->P * (searchingNode->N - 1))) {
                bestChild = child;
                bestAction = action;
                child->forced += 1;
                break;
            }

            if (puct > bestPuct) {
                bestPuct = puct;
                bestChild = child;
                bestAction = action;
            }
        }

        if (bestChild == nullptr)
            return false;
        if (bestChild->N == 0)
            searchingNode->childP += bestChild->P;
        Board statecpy(states.back());
        statecpy.inflict(statecpy.currentColour, bestAction);
        statecpy.changeTurn();
        states.push_back(statecpy);
        searchingNode = bestChild;
        return search(states);
    }
}

void BatchedMCTSTree::backpropagate(float V) {
    searchingNode->Q = (searchingNode->Q * (searchingNode->N - 1) + V) / searchingNode->N;
    if (searchingNode != rootNode) {
        searchingNode = searchingNode->parent;
        backpropagate(-V);
    }
}

Action BatchedMCTSTree::selectByProportionalPolicy(std::default_random_engine& eng, double temperature) {
    if (rootNode->lost)
        return Action();
    int bestN = -1;
    BatchedMCTSNode* bestChild = rootNode;
    Action bestAction;

    for (auto& [action, child] : rootNode->childs)
        if (child->N > bestN) {
            bestChild = child;
            bestN = child->N;
            bestAction = action;
        }
        
    std::vector<double> weights;

    for (auto& [action, child] : rootNode->childs) {
        if (child != bestChild) {
            while (true) {
                if (child->N == 1) {
                    child->N = 0;
                    break;
                } else if (child->N == 0)
                    break;
                if (child->forced == 0)
                    break;
                if (child->Q + puctConstant * child->P * sqrt(rootNode->N - 2 + 0.01f)/child->N
                    < bestChild->Q + puctConstant * bestChild->P * sqrt(rootNode->N - 2 + 0.01f)/(bestChild->N + 1)) {
                    child->forced--;
                    child->N--;
                    rootNode->N--;
                } else
                    break;
            }
        }

        weights.push_back(pow(child->N, 1/temperature));
    }

    std::discrete_distribution<int> dis(weights.begin(), weights.end());

    auto [selectedAction, selectedChild] = rootNode->childs[dis(eng)];

#ifdef PROP_GC
    for (auto& [action, child] : rootNode->childs)
        if (child != selectedChild)
            child->clearChilds();
#endif
    this->rootNode = selectedChild;
    this->searchingNode = this->rootNode;
    return selectedAction;
}

Action BatchedMCTSTree::selectByNaivePolicy(std::default_random_engine& eng) {
    std::vector<double> weights;

    if (rootNode->lost)
        return Action();

    for (auto& [action, child] : rootNode->childs)
        weights.push_back(child->P);
    std::discrete_distribution<int> dis(weights.begin(), weights.end());
    auto [action, child] = rootNode->childs[dis(eng)];
    this->rootNode = child;
    this->searchingNode = this->rootNode;
    return action;            
}

void BatchedMCTSGame::act(Action action, gsl_rng* r) {
    if (this->ended)
        return;
    
    actionHistory.push_back(action);
    
    if (action.type == RESIGN || action.type == ILLEGAL) {
        winner = !current.currentColour;
        ended = true;
    } else if (action.type == SENNICHITE || action.type == JISHOGI) {
        winner = -1;
        ended = true;
    } else {
        std::vector<Action> availables = current.getKActions(current.currentColour);
        BatchedMCTSTree* tree = getOpponent()->tree;
        tree->expandRaw(current.getKActions(current.currentColour));
        tree->opponentMove(action);
        if (tree->dirichletEnabled && tree->rootNode->expanded)
            tree->addDirichlet(r);
        current.inflict(current.currentColour, action);
        current.changeTurn();
        history.push_back(Board(current));
        if (moveCount++ == moveCountThreshold)
            this->ended = true;
    }        
}

bool BatchedMCTSPlayer::step(std::default_random_engine& eng, gsl_rng* r, std::vector<Board>& states) {
    if (this->moveCount < naivePlayout) {
        this->tree->dirichletEnabled = false;

        if (currDepth++ == 0)
            return true;
        else {
            this->moveCount++;
            game->act(this->tree->selectByNaivePolicy(eng), r);
            currDepth = 0;
        }

        return false;
    } else if (this->moveCount == naivePlayout)
        this->tree->dirichletEnabled = true;
    
    if (currDepth++ == (isFull ? bigDepth : smallDepth)) {
        if (isFull)
            this->samples.push_back(this->tree->rootNode);
        this->moveCount++;
        game->act(this->tree->selectByProportionalPolicy(eng, temp), r);
        if (temp > endTemp)
            temp *= pow(0.5, 1.0 / halfLife);
        if (temp <= endTemp)
            temp = endTemp;
        std::discrete_distribution<int> dis({bigSimuProb, 1 - bigSimuProb});
        currDepth = 0;
        isFull = dis(eng) == 0;
    } else {
        if (this->tree->search(states))
            return true;
        else {
            this->tree->backpropagate(-1);
            return false;
        }
    }

    return false;
}

void BatchedMCTSTree::addDirichlet(gsl_rng* r) {
    double* alpha = new double[rootNode->childs.size()];
    for (int i = 0; i < rootNode->childs.size(); i++)
        alpha[i] = dirichletConstant;
    
    double* theta = new double[rootNode->childs.size()];
    gsl_ran_dirichlet(r, rootNode->childs.size(), alpha, theta);

    for (int i = 0; i < rootNode->childs.size(); i++) {
        BatchedMCTSNode* child = rootNode->childs[i].second;
        child->P = (1 - dirichletEpsilon) * child->P + dirichletEpsilon * theta[i];
    }

    delete[] theta;
    delete[] alpha;
}

void BatchedMCTSTree::expandRaw(const std::vector<Action>& availables) {
    if (searchingNode->expanded)
        return;
    for (int i = 0; i < availables.size(); i++) {
        Action action = availables[i];
        BatchedMCTSNode* child = new BatchedMCTSNode(searchingNode);
        searchingNode->childs.push_back(std::make_pair(action, child));
    }

    searchingNode->expanded = true;

    if (searchingNode->childs.empty())
        searchingNode->lost = true;
}

float BatchedMCTSTree::expand(const std::vector<Action>& availables, int j, torch::Tensor outputs[3]) {
    searchingNode->activateDFPN = outputs[2][j][0].item<float>();

    for (int i = 0; i < availables.size(); i++) {
        Action action = availables[i];
        BatchedMCTSNode* child = new BatchedMCTSNode(searchingNode);
        child->P = outputs[0][j][action.toModelOutput()][action.getPrincipalPosition() / 9][action.getPrincipalPosition() % 9].item<float>();
        searchingNode->childs.push_back(std::make_pair(action, child));
    }

    searchingNode->expanded = true;

    if (searchingNode->childs.empty()) {
        searchingNode->lost = true;
        return -1;
    }
    
    return outputs[1][j][0].item<float>() - outputs[1][j][2].item<float>();
}