#ifndef MCTS_H
#define MCTS_H

#include "../action.h"
#include "../board.h"
#include "mctsmodel.h"

#include <gsl/gsl_rng.h>
#include <gsl/gsl_randist.h>
#include <torch/torch.h>
#include <unordered_map>
#include <vector>
#include <time.h>
#include <iostream>

constexpr float PUCT_CONSTANT = 4;
constexpr double DIRICHLET_CONSTANT = 0.15;

class MCTSNode {
    public:
    Board state;
    int moveCount;
    MCTSNode* parent;
    bool expanded;
    bool lost;
    int N;
    int movecount;
    float Q;
    float P;
    std::unordered_map<Action, MCTSNode*, ActionHash> childs;

    MCTSNode() : state(BEMPTY), movecount(-1), parent(nullptr), expanded(false), lost(false), N(0), Q(0), P(0), childs() {}
    MCTSNode(Board state, MCTSNode* parent) : state(state), movecount(parent->moveCount + 1), parent(parent), expanded(false), lost(false), N(0), Q(0), P(0), childs() {}

    ~MCTSNode() {
        for (auto& [action, node] : childs)
            delete node;
    }

    torch::Tensor toInput() {
        float input[(PIECE_NUMBER + DROP_NUMBER) * 3 + 2][9][9] = {};
        this->state.toInput(input);
        this->parent->state.toInput(&input[PIECE_NUMBER + DROP_NUMBER]);
        this->parent->parent->state.toInput(&input[(PIECE_NUMBER + DROP_NUMBER) * 2]);

        for (int f = 0; f < FILE_NUMBER; f++)
            for (int r = 0; r < RANK_NUMBER; r++) {
                input[(PIECE_NUMBER + DROP_NUMBER) * 3][f][r] = this->state.currentColour;
                input[(PIECE_NUMBER + DROP_NUMBER) * 3 + 1][f][r] = this->moveCount;
            }
        
        return torch::from_blob(input, {(PIECE_NUMBER + DROP_NUMBER) * 3 + 2, 9, 9}, torch::TensorOptions().dtype(torch::kFloat32)).clone().unsqueeze(0);
    }

    void expand(MCTSModel model) {
        std::vector<Action> availables = state.getKActions(state.currentColour);
        gsl_rng* r = gsl_rng_alloc(gsl_rng_mt19937);
        gsl_rng_set(r, time(NULL));
        double* alpha = new double[availables.size()];
        for (int i = 0; i < availables.size(); i++)
            alpha[i] = DIRICHLET_CONSTANT;
        
        double* theta = new double[availables.size()];
        gsl_ran_dirichlet(r, availables.size(), alpha, theta);
        torch::Tensor outputs[2];

        model.get()->forward(this->toInput(), outputs);

        for (int i = 0; i < availables.size(); i++) {
            Action action = availables[i];
            Board statecpy(state);
            statecpy.inflict(state.currentColour, action);
            statecpy.changeTurn();
            
            MCTSNode* child = new MCTSNode(statecpy, this);
            child->P = 0.75 * outputs[0][0][action.toModelOutput()][action.getPrincipalPosition() / 9][action.getPrincipalPosition() % 9].item<float>() + 0.25 * theta[i];
            childs.insert({action, child});
        }

        this->expanded = true;

        if (this->childs.empty())
            this->lost = true;
        delete[] theta;
        delete[] alpha;
        gsl_rng_free(r);
    }
};

class MCTS {
    private:
    MCTSNode emptyCyclicNode;

    public:
    MCTSModel model;
    MCTSNode* rootNode;
    MCTSNode* searchingNode;

    MCTS(MCTSModel model, Board initialState) : model(model) {
        this->model.get()->eval();
        emptyCyclicNode = MCTSNode();
        emptyCyclicNode.parent = &emptyCyclicNode;
        rootNode = new MCTSNode(initialState, &emptyCyclicNode);
        searchingNode = rootNode;
    }

    float simulate(MCTSNode* node);

    void search(int depth) {
        for (int i = 0; i < depth; i++)
            simulate(this->searchingNode);
    }

    void opponentMove(Action action) {
        if (!this->searchingNode->expanded)
            this->searchingNode->expand(model);
        this->searchingNode = this->searchingNode->childs[action];
    }

    Action selectByGreedyPolicy() {
        int bestN = -1;
        MCTSNode* bestChild = searchingNode;
        Action bestAction;
        
        for (auto& [ action, child ] : searchingNode->childs)
            if (child->N > bestN) {
                bestChild = child;
                bestN = child->N;
                bestAction = action;
            }
        
        this->searchingNode = bestChild;
        
        return bestAction;
    }

    ~MCTS() {
        delete rootNode;
    }
};

#endif