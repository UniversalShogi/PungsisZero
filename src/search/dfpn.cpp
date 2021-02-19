#include "../action.h"
#include "../board.h"
#include "dfpn.h"
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

void DFPN::RID(Board state, int deltaThreshold, int phiThreshold, bool myKing, bool opponentKing, bool isMating, std::unordered_map<Board, Action, Zobrist>& tsumeMap) {
    if (this->aborted)
        return;

    auto& [ delta, phi, treeSize, visiting ] = this->lookupTTable(state);
    visiting = true;

    if (this->dfpnTTable.size() > entryThreshold)
        switch (gcMode) {
            case ABORT:
                this->aborted = true;
                return;
            case GC1:
                this->smallTreeGC1();
                break;
            case GC2:
                this->smallTreeGC2(this->gc2Threshold);
                break;
        }

    if (deltaThreshold <= delta || phiThreshold <= phi) {
        visiting = false;
        return;
    }
    
    std::vector<Action> legalActions = myKing ? state.getKActions(state.currentColour) : state.getNKActions(state.currentColour);

    int legalCount = legalActions.size();

    std::vector<Action> filteredActions;
    std::vector<Board> filteredBoards;
    
    for (int i = 0; i < legalCount; i++) {
        Board statecpy(state);
        statecpy.inflict(state.currentColour, legalActions[i]);
        statecpy.changeTurn();

        if (!isMating || statecpy.getAttackers(state.currentColour, statecpy.pieces[kingOf(!state.currentColour)].first())) {
            filteredActions.push_back(legalActions[i]);
            filteredBoards.push_back(statecpy);
        }
    }

    int filteredCount = filteredActions.size();

    if (filteredCount == 0) {
        delta = 0;
        phi = LULFINITY;
        visiting = false;
        return;
    }

    while (true) {
        int phiSum = 0;
        treeSize = 0;
        int deltaMin = LULFINITY;
        int phiMin = 0;
        int deltaSecondMin = LULFINITY;
        Action deltaMinAction;
        Board deltaMinState;

        for (int i = 0; i < filteredCount; i++) {
            auto [ subDelta, subPhi, subTreeSize, subVisiting ] = this->lookupTTable(filteredBoards[i]);
            if (subVisiting)
                continue;
            treeSize += subTreeSize;
            
            if (subDelta < deltaMin) {
                deltaSecondMin = deltaMin;
                deltaMin = subDelta;
                phiMin = subPhi;
                deltaMinState = filteredBoards[i];
                deltaMinAction = filteredActions[i];
            } else if (subDelta < deltaSecondMin) {
                deltaSecondMin = subDelta;
            }

            if (subPhi == LULFINITY) {
                delta = LULFINITY;
                phi = 0;
                if (isMating)
                    tsumeMap[state] = filteredActions[i];
                visiting = false;
                return;
            }

            phiSum += subPhi;
        }

        if (phiSum >= deltaThreshold || deltaMin >= phiThreshold) {
            delta = phiSum;
            phi = deltaMin;
            visiting = false;
            return;
        }

        int deltaSecondMinP1 = deltaSecondMin == LULFINITY ? LULFINITY : deltaSecondMin + 1;
        int newPhiThreshold = deltaThreshold == LULFINITY ? LULFINITY : deltaThreshold + phiMin - phiSum;

        this->RID(deltaMinState, phiThreshold < deltaSecondMinP1 ? phiThreshold : deltaSecondMinP1, newPhiThreshold, opponentKing, myKing, !isMating, tsumeMap);

        if (this->aborted)
            return;
    }

    visiting = false;
}

bool DFPN::solveTsume(Board initialState, bool withAttackingKing, std::unordered_map<Board, Action, Zobrist>& tsume) {
    this->RID(initialState, LULFINITY, LULFINITY, withAttackingKing, true, true, tsume);

    if (this->aborted) {
        this->aborted = false;
        return false;
    }

    return true;
}