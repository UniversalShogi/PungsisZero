#include "mcts.h"
#include "mctsmodel.h"

#include <limits>
#include <math.h>

float MCTS::simulate(MCTSNode* node) {
    node->N += 1;

    float V;

    if (node->lost) {
        V = -1;
        return V;
    }

    if (!node->expanded) {
        if (node == this->searchingNode)
            node->expandNoisy(model);
        else
            node->expandSilent(model);

        if (node->lost) {
            V = -1;
            return V;
        } else {
            torch::Tensor outputs[2];
            model.get()->forward(node->toInput(), outputs);
            V = outputs[1][0][0].item<float>();
            return V;
        }
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
    
    V = -this->simulate(bestChild);
    bestChild->Q = ((bestChild->N - 1) * bestChild->Q + V)/ bestChild->N;

    return V;
}