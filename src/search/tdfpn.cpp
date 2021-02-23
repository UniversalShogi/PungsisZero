#include "../action.h"
#include "../board.h"
#include "tdfpn.h"
#include "../action.h"
#include "../zobrist.h"

#include <algorithm>
#include <utility>
#include <unordered_map>
#include <vector>
#include <iostream>
#include <numeric>
#include <thread>
#include <chrono>

void TDFPN::RID(TDFPNNode* node, int deltaThreshold, int phiThreshold, bool myKing, bool opponentKing, bool isMating, std::vector<Board>& history, std::unordered_map<Board, Action, Zobrist>& tsumeMap) {
    if (deltaThreshold <= node->delta || phiThreshold <= node->phi)
        return;
    
    if (!node->expanded) {
        std::vector<Action> legalActions = myKing ? node->state.getKActions(node->state.currentColour) : node->state.getNKActions(node->state.currentColour);
        int legalCount = legalActions.size();

        for (int i = 0; i < legalCount; i++) {
            Board statecpy(node->state);
            statecpy.inflict(node->state.currentColour, legalActions[i]);
            statecpy.changeTurn();

            if (std::find(history.begin(), history.end(), statecpy) == history.end() &&
                (!isMating || statecpy.getAttackers(node->state.currentColour, statecpy.pieces[kingOf(!node->state.currentColour)].first())))
                node->childs.push_back(new TDFPNNode{legalActions[i], statecpy, 1, 1, false, std::vector<TDFPNNode*>()});
        }

        node->expanded = true;
    }

    if (node->childs.empty()) {
        node->delta = 0;
        node->phi = LULFINITY;
        return;
    }

    int childCount = node->childs.size();

    while (true) {
        int phiSum = 0;
        TDFPNNode* deltaMin = LULFINITY_DELTA;
        int deltaSecondMin = LULFINITY;

        for (auto& child : node->childs) {
            if (child->delta < deltaMin->delta) {
                deltaSecondMin = deltaMin->delta;
                deltaMin = child;
            } else if (child->delta < deltaSecondMin) {
                deltaSecondMin = child->delta;
            }

            if (child->phi == LULFINITY) {
                node->delta = LULFINITY;
                node->phi = 0;
                if (isMating)
                    tsumeMap[node->state] = child->action;
                for (auto& childschild : child->childs)
                    delete childschild;
                child->childs = std::vector<TDFPNNode*>();
                return;
            }

            phiSum += child->phi;
        }

        if (phiSum >= deltaThreshold || deltaMin->delta >= phiThreshold) {
            node->delta = phiSum;
            node->phi = deltaMin->delta;
            return;
        }

        int deltaSecondMinP1 = deltaSecondMin == LULFINITY ? LULFINITY : deltaSecondMin + 1;
        int newPhiThreshold = deltaThreshold == LULFINITY ? LULFINITY : deltaThreshold + deltaMin->phi - phiSum;

        history.push_back(deltaMin->state);
        this->RID(deltaMin, phiThreshold < deltaSecondMinP1 ? phiThreshold : deltaSecondMinP1, newPhiThreshold, opponentKing, myKing, !isMating, history, tsumeMap);
        history.pop_back();
    }
}

std::unordered_map<Board, Action, Zobrist> TDFPN::solveTsume(Board initialState, bool withAttackingKing) {
    std::unordered_map<Board, Action, Zobrist> tsume;
    std::vector<Board> history;
    TDFPNNode* rootNode = new TDFPNNode{Action(MoveAction(0, 0, 0, false)), initialState, 1, 1, false, std::vector<TDFPNNode*>()};
    history.push_back(initialState);
    this->RID(rootNode, LULFINITY, LULFINITY, withAttackingKing, true, true, history, tsume);
    delete rootNode;
    return tsume;
}