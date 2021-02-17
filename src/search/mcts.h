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
    std::vector<std::pair<Action, MCTSNode*>> childs;

    MCTSNode(int moveCount) : moveCount(moveCount), expanded(false), lost(false), N(0), childP(0), forced(0), Q(0), P(0), activateDFPN(0), childs() {}
    
    void clearChilds() {
        for (auto& [action, node] : childs)
            delete node;
        this->childs.clear();
    }

    ~MCTSNode() {
        clearChilds();
    }

    void toInput(Board states[3], torch::Tensor inputs[2]) {
        float binput[PIECE_NUMBER * 3 + COLOUR_NUMBER * 3 + 1][9][9] = {};
        float ninput[DROP_NUMBER * 3 + 1] = {};
        states[2].toInput(binput, ninput);
        states[1].toInput(&binput[PIECE_NUMBER], &ninput[DROP_NUMBER]);
        states[0].toInput(&binput[PIECE_NUMBER * 2], &ninput[DROP_NUMBER * 2]);
        states[2].toFeatureInput(&binput[PIECE_NUMBER * 3], &ninput[DROP_NUMBER * 3]);
        
        inputs[0] = torch::from_blob(binput, {PIECE_NUMBER * 3 + COLOUR_NUMBER * 3 + 1, 9, 9}, torch::TensorOptions().dtype(torch::kFloat32)).clone().to(torch::kCUDA).unsqueeze(0);
        inputs[1] = torch::from_blob(ninput, {DROP_NUMBER * 3 + 1}, torch::TensorOptions().dtype(torch::kFloat32)).clone().to(torch::kCUDA).unsqueeze(0);
    }

    float expandNoisy(MCTSModel model, Board states[3], float dirichletConstant, double dirichletEpsilon, gsl_rng* r) {
        std::vector<Action> availables = states[2].getKActions(states[2].currentColour);
        double* alpha = new double[availables.size()];
        for (int i = 0; i < availables.size(); i++)
            alpha[i] = dirichletConstant;
        
        double* theta = new double[availables.size()];
        gsl_ran_dirichlet(r, availables.size(), alpha, theta);
        torch::Tensor inputs[2];
        this->toInput(states, inputs);
        torch::Tensor outputs[3];

        model->forward(inputs[0], inputs[1], outputs);
        this->activateDFPN = outputs[2][0][0].item<float>();

        for (int i = 0; i < availables.size(); i++) {
            Action action = availables[i];
            MCTSNode* child = new MCTSNode(this->moveCount + 1);
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

    float expandSilent(MCTSModel model, Board states[3]) {
        std::vector<Action> availables = states[2].getKActions(states[2].currentColour);
        torch::Tensor inputs[2];
        this->toInput(states, inputs);
        torch::Tensor outputs[3];

        model->forward(inputs[0], inputs[1], outputs);
        this->activateDFPN = outputs[2][0][0].item<float>();

        for (int i = 0; i < availables.size(); i++) {
            Action action = availables[i];
            MCTSNode* child = new MCTSNode(this->moveCount + 1);
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

class MCTS {
    public:
    MCTSModel model;
    MCTSNode* rootNode;
    MCTSNode* searchingNode;
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
        rootNode = new MCTSNode(0);
        searchingNode = rootNode;
        gsl_rng_set(r, rd());
    }

    void stimulateSearching(Board states[3]) {
        if (!this->searchingNode->expanded)
            this->searchingNode->expandSilent(model, states);
    }

    float simulate(MCTSNode* node, Board states[3]);

    void search(Board states[3], int depth) {
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
        MCTSNode* bestChild = searchingNode;
        Action bestAction;
        
        for (auto& [action, child] : searchingNode->childs)
            if (child->N > bestN) {
                bestChild = child;
                bestN = child->N;
                bestAction = action;
            }
        
        this->searchingNode = bestChild;

        std::cout << bestChild->Q << std::endl;
        
        return bestAction;
    }

    Action selectByProportionalPolicy(double temperature) {
        if (searchingNode->lost)
            return Action();
        int bestN = -1;
        MCTSNode* bestChild = searchingNode;
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