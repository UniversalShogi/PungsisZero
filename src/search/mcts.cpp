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
        if (node == this->searchingNode)
            return node->expandNoisy(model);
        else
            return node->expandSilent(model);
    }

    MCTSNode* bestChild = nullptr;
    float bestPuct = -std::numeric_limits<float>::infinity();

    for (auto& [action, child] : node->childs) {
        float puct = child->Q + PUCT_CONSTANT * child->P * sqrt(node->N)/(1 + child->N);

        if (puct > bestPuct) {
            bestPuct = puct;
            bestChild = child;
        }
    }

    if (bestChild == nullptr)
        return 0;
    
    float V = -this->simulate(bestChild);
    bestChild->Q = ((bestChild->N - 1) * bestChild->Q + V)/ bestChild->N;

    return V;
}