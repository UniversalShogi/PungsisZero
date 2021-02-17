#include "mcts.h"
#include "mctsmodel.h"

#include <limits>
#include <math.h>

float MCTS::simulate(MCTSNode* node) {
    node->N += 1;

    if (node->lost) {
        return -1;
    }

    if (!node->expanded) {
        if (this->dirichletEnabled && node == this->searchingNode)
            return node->expandNoisy(model, this->dirichletConstant, this->dirichletEpsilon, this->r);
        else
            return node->expandSilent(model);
    }

    MCTSNode* bestChild = nullptr;
    float bestPuct = -std::numeric_limits<float>::infinity();

    for (auto& [action, child] : node->childs) {
        float Q = child->N == 0 ? node->Q - (node == searchingNode ? fpuRoot : fpuNonRoot) * sqrt(node->childP) : child->Q;
        float puct = Q + puctConstant * child->P * sqrt(node->N - 1 + 0.01f)/(1 + child->N);

        if (this->forcedPlayoutEnabled && node == this->searchingNode
            && child->N > 0 && child->N < sqrt(forcedSimuConstant * child->P * (node->N - 1))) {
            bestChild = child;
            child->forced += 1;
            break;
        }

        if (puct > bestPuct) {
            bestPuct = puct;
            bestChild = child;
        }
    }

    if (bestChild == nullptr)
        return 0;
    if (bestChild->N == 0)
        node->childP += bestChild->P;
    float V = -this->simulate(bestChild);
    bestChild->Q = ((bestChild->N - 1) * bestChild->Q + V)/ bestChild->N;

    return V;
}