#ifndef MCTS_H
#define MCTS_H

#include "action.h"
#include "board.h"
#include "pungsiszero.h"

#include <torch/torch.h>
#include <unordered_map>

class Node {
    public:
    Board state;
    Node* parent;
    int N;
    int Q;
    const int P;
    const int V;
    std::unordered_map<int, Node*> childs;

    ~Node() {
        for (auto& [hash, node] : childs)
            delete node;
    }
};

extern Node EMPTY_CYCLIC_NODE;

class MonteCarloTreeSearch {
    public:
    PungsisZero net;
    Node* rootNode;
    Node* searchingNode;

    MonteCarloTreeSearch(PungsisZero net, Board initialState) : net(net) {
        rootNode = new Node{initialState, &EMPTY_CYCLIC_NODE, 0, 0, 0, 0};
    }

    ~MonteCarloTreeSearch() {
        delete rootNode;
    }
};

#endif