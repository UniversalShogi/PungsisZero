#ifndef MCTS_H
#define MCTS_H

#include "../action.h"
#include "../board.h"
#include "mctsmodel.h"

#include <torch/torch.h>
#include <unordered_map>

class MCTSNode {
    public:
    Board state;
    MCTSNode* parent;
    int N;
    int Q;
    const int P;
    const int V;
    std::unordered_map<int, MCTSNode*> childs;

    ~MCTSNode() {
        for (auto& [hash, node] : childs)
            delete node;
    }
};

extern MCTSNode EMPTY_CYCLIC_NODE;

class MCTS {
    public:
    MCTSModel net;
    MCTSNode* rootNode;
    MCTSNode* searchingNode;

    MCTS(MCTSModel net, Board initialState) : net(net) {
        rootNode = new MCTSNode{initialState, &EMPTY_CYCLIC_NODE, 0, 0, 0, 0};
    }

    ~MCTS() {
        delete rootNode;
    }
};

#endif