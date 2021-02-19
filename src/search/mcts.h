#ifndef MCTS_H
#define MCTS_H

#include "../action.h"
#include "../board.h"
#include "mctsmodel.h"

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

template <int n>
class MCTSNode {
    public:
    int moveCount;
    bool expanded;
    bool lost;
    int N;
    float childP;
    int forced;
    float Q;
    float P;
    float activateDFPN;
    std::vector<std::pair<Action, MCTSNode<n>*>> childs;

    MCTSNode(int moveCount) : moveCount(moveCount), expanded(false), lost(false), N(0), childP(0), forced(0), Q(0), P(0), activateDFPN(0), childs() {}
    
    void clearChilds() {
        for (auto& [action, node] : childs)
            delete node;
        this->childs.clear();
    }

    ~MCTSNode() {
        clearChilds();
    }

    static void toInput(Board states[n], torch::Tensor inputs[2]) {
        float binput[PIECE_NUMBER * n + COLOUR_NUMBER * 3 + 1][9][9] = {};
        float ninput[DROP_NUMBER * n + 1] = {};
        Board::toInputs(n, binput, ninput, states);
        
        inputs[0] = torch::from_blob(binput, {PIECE_NUMBER * n + COLOUR_NUMBER * 3 + 1, 9, 9}, torch::TensorOptions().dtype(torch::kFloat32)).clone().to(torch::kCUDA).unsqueeze(0);
        inputs[1] = torch::from_blob(ninput, {DROP_NUMBER * n + 1}, torch::TensorOptions().dtype(torch::kFloat32)).clone().to(torch::kCUDA).unsqueeze(0);
    }

    float expandNoisy(MCTSModel model, Board states[n], float dirichletConstant, double dirichletEpsilon, gsl_rng* r) {
        std::vector<Action> availables = states[n - 1].getKActions(states[n - 1].currentColour);
        double* alpha = new double[availables.size()];
        for (int i = 0; i < availables.size(); i++)
            alpha[i] = dirichletConstant;
        
        double* theta = new double[availables.size()];
        gsl_ran_dirichlet(r, availables.size(), alpha, theta);
        torch::Tensor inputs[2];
        toInput(states, inputs);
        torch::Tensor outputs[3];

        model->forward(inputs[0], inputs[1], outputs);
        this->activateDFPN = outputs[2][0][0].item<float>();

        for (int i = 0; i < availables.size(); i++) {
            Action action = availables[i];
            MCTSNode<n>* child = new MCTSNode(this->moveCount + 1);
            child->P = (1 - dirichletEpsilon) * outputs[0][0][action.toModelOutput()][action.getPrincipalPosition() / 9][action.getPrincipalPosition() % 9].item<float>() + dirichletEpsilon * theta[i];
            childs.push_back(std::make_pair(action, child));
        }

        this->expanded = true;

        delete[] theta;
        delete[] alpha;

        if (this->childs.empty()) {
            this->lost = true;
            return -1;
        }

        return outputs[1][0][0].item<float>() - outputs[1][0][2].item<float>();
    }

    float expandSilent(MCTSModel model, Board states[n]) {
        std::vector<Action> availables = states[n - 1].getKActions(states[n - 1].currentColour);
        torch::Tensor inputs[2];
        toInput(states, inputs);
        torch::Tensor outputs[3];

        model->forward(inputs[0], inputs[1], outputs);
        this->activateDFPN = outputs[2][0][0].item<float>();

        for (int i = 0; i < availables.size(); i++) {
            Action action = availables[i];
            MCTSNode<n>* child = new MCTSNode(this->moveCount + 1);
            child->P = outputs[0][0][action.toModelOutput()][action.getPrincipalPosition() / 9][action.getPrincipalPosition() % 9].item<float>();
            childs.push_back(std::make_pair(action, child));
        }

        this->expanded = true;

        if (this->childs.empty()) {
            this->lost = true;
            return -1;
        }
        
        return outputs[1][0][0].item<float>() - outputs[1][0][2].item<float>();
    }
};

template <int n>
class MCTS {
    public:
    MCTSModel model;
    MCTSNode<n>* rootNode;
    MCTSNode<n>* searchingNode;
    std::random_device rd;
    std::default_random_engine eng;
    gsl_rng* r;
    bool dirichletEnabled;
    bool forcedPlayoutEnabled;
    float fpuRoot;
    float fpuNonRoot;
    float puctConstant;
    double dirichletConstant;
    float dirichletEpsilon;
    float forcedSimuConstant;

    MCTS(MCTSModel model, bool dirichletEnabled = true, bool forcedPlayoutEnabled = true, float fpuRoot = 0.0f, float fpuNonRoot = 0.2f,
        float puctConstant = 1.1f, double dirichletConstant = 0.15, float dirichletEpsilon = 0.25,
        float forcedSimuConstant = 2)
        : model(model), rd(), eng(rd()), r(gsl_rng_alloc(gsl_rng_mt19937)), dirichletEnabled(dirichletEnabled), forcedPlayoutEnabled(forcedPlayoutEnabled), fpuRoot(fpuRoot), fpuNonRoot(fpuNonRoot),
        puctConstant(puctConstant), dirichletConstant(dirichletConstant), dirichletEpsilon(dirichletEpsilon),
        forcedSimuConstant(forcedSimuConstant) {
        this->model->eval();
        rootNode = new MCTSNode<n>(0);
        searchingNode = rootNode;
        gsl_rng_set(r, rd());
    }

    void stimulateSearching(Board states[3]) {
        if (!this->searchingNode->expanded)
            this->searchingNode->expandSilent(model, states);
    }

    float simulate(MCTSNode<n>* node, Board states[n]) {
        node->N += 1;

        if (node->lost) {
            return -1;
        }

        if (!node->expanded) {
            if (this->dirichletEnabled && node == this->searchingNode)
                return node->expandNoisy(model, states, this->dirichletConstant, this->dirichletEpsilon, this->r);
            else
                return node->expandSilent(model, states);
        }

        Action bestAction;
        MCTSNode<n>* bestChild = nullptr;
        float bestPuct = -std::numeric_limits<float>::infinity();

        for (auto& [action, child] : node->childs) {
            float Q = child->N == 0 ? node->Q - (node == searchingNode ? fpuRoot : fpuNonRoot) * sqrt(node->childP) : child->Q;
            float puct = Q + puctConstant * child->P * sqrt(node->N - 1 + 0.01f)/(1 + child->N);

            if (this->forcedPlayoutEnabled && node == this->searchingNode
                && child->N > 0 && child->N < sqrt(forcedSimuConstant * child->P * (node->N - 1))) {
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
            return 0;
        if (bestChild->N == 0)
            node->childP += bestChild->P;
        Board newStates[n];
        for (int i = 1; i < n; i++)
            newStates[i] = states[i - 1];
        newStates[n - 1] = Board(states[n - 1]);
        newStates[n - 1].inflict(newStates[n - 1].currentColour, bestAction);
        newStates[n - 1].changeTurn();
        float V = -this->simulate(bestChild, newStates);
        bestChild->Q = ((bestChild->N - 1) * bestChild->Q + V) / bestChild->N;

        return V;
    }

    void search(Board states[n], int depth) {
        for (int i = 0; i < depth; i++)
            simulate(this->searchingNode, states);
    }

    void opponentMove(Action opponentAction) {
        for (auto& [action, child] : searchingNode->childs)
            if (action == opponentAction) {
                this->searchingNode = child;
                return;
            }
    }

    Action selectByGreedyPolicy() {
        int bestN = -1;
        MCTSNode<n>* bestChild = searchingNode;
        Action bestAction;
        
        for (auto& [action, child] : searchingNode->childs)
            if (child->N > bestN) {
                bestChild = child;
                bestN = child->N;
                bestAction = action;
            }
        
        this->searchingNode = bestChild;
        
        return bestAction;
    }

    Action selectByProportionalPolicy(double temperature) {
        if (searchingNode->lost)
            return Action();
        int bestN = -1;
        MCTSNode<n>* bestChild = searchingNode;
        Action bestAction;

        for (auto& [action, child] : searchingNode->childs)
            if (child->N > bestN) {
                bestChild = child;
                bestN = child->N;
                bestAction = action;
            }
            
        std::vector<double> weights;

        for (auto& [action, child] : searchingNode->childs) {
            if (child != bestChild) {
                while (true) {
                    if (child->N == 1) {
                        child->N = 0;
                        break;
                    } else if (child->N == 0)
                        break;
                    if (child->forced == 0)
                        break;
                    if (child->Q + puctConstant * child->P * sqrt(searchingNode->N - 2 + 0.01f)/child->N
                        < bestChild->Q + puctConstant * bestChild->P * sqrt(searchingNode->N - 2 + 0.01f)/(bestChild->N + 1)) {
                        child->forced--;
                        child->N--;
                        searchingNode->N--;
                    } else
                        break;
                }
            }

            weights.push_back(pow(child->N, 1/temperature));
        }

        std::discrete_distribution<int> dis(weights.begin(), weights.end());

        auto [selectedAction, selectedChild] = searchingNode->childs[dis(eng)];

#ifdef PROP_GC
        for (auto& [action, child] : searchingNode->childs)
            if (child != selectedChild)
                child->clearChilds();
#endif
        this->searchingNode = selectedChild;
        return selectedAction;
    }

    Action selectByNaivePolicy() {
        std::vector<double> weights;

        if (searchingNode->lost)
            return Action();

        for (auto& [action, child] : searchingNode->childs)
            weights.push_back(child->P);
        std::discrete_distribution<int> dis(weights.begin(), weights.end());
        auto [action, child] = searchingNode->childs[dis(eng)];
        this->searchingNode = child;
        return action;            
    }

    ~MCTS() {
        delete rootNode;
        gsl_rng_free(r);
    }
};

#endif