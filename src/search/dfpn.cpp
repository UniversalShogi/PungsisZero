#include "../action.h"
#include "../board.h"
#include "dfpn.h"
#include "../action.h"
#include "../zobrist.h"

#include <algorithm>
#include <limits>
#include <utility>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <iostream>
#include <numeric>
#include <thread>
#include <chrono>

constexpr int INFINITY = std::numeric_limits<int>::max();

void DFPN::RID(Board state, int deltaThreshold, int phiThreshold, bool myKing, bool opponentKing, bool isMating, std::unordered_set<Board, Zobrist>& history, std::unordered_map<Board, Action, Zobrist>& tsumeMap) {
    auto& [ delta, phi ] = this->lookupTTable(state);

    if (deltaThreshold <= delta || phiThreshold <= phi)
        return;
    
    std::vector<Action> legalActions = myKing ? state.getKActions(state.currentColour) : state.getNKActions(state.currentColour);

    int legalCount = legalActions.size();

    std::vector<Action> filteredActions;
    std::vector<Board> filteredBoards;
    
    for (int i = 0; i < legalCount; i++) {
        Board statecpy(state);
        statecpy.inflict(state.currentColour, legalActions[i]);
        statecpy.changeTurn();

        if (history.find(statecpy) == history.end() &&
            (!isMating || statecpy.getAttackers(state.currentColour, statecpy.pieces[kingOf(!state.currentColour)].first()))) {
            filteredActions.push_back(legalActions[i]);
            filteredBoards.push_back(statecpy);
        }
    }

    int filteredCount = filteredActions.size();

    if (filteredCount == 0) {
        delta = 0;
        phi = INFINITY;
        return;
    }

    delta = deltaThreshold, phi = phiThreshold;

    while (true) {
        int phiSum = 0;
        int deltaMin = INFINITY;
        int phiMin = 0;
        int deltaSecondMin = INFINITY;
        Action deltaMinAction;
        Board deltaMinState;

        for (int i = 0; i < filteredCount; i++) {
            auto [subDelta, subPhi] = this->lookupTTable(filteredBoards[i]);
            
            if (subDelta < deltaMin) {
                deltaSecondMin = deltaMin;
                deltaMin = subDelta;
                phiMin = subPhi;
                deltaMinState = filteredBoards[i];
                deltaMinAction = filteredActions[i];
            } else if (subDelta < deltaSecondMin) {
                deltaSecondMin = subDelta;
            }

            if (subPhi == INFINITY) {
                delta = INFINITY;
                phi = 0;
                if (isMating)
                    tsumeMap[state] = filteredActions[i];
                return;
            }

            phiSum += subPhi;
        }

        if (phiSum >= deltaThreshold || deltaMin >= phiThreshold) {
            delta = phiSum;
            phi = deltaMin;
            return;
        }

        int deltaSecondMinP1 = deltaSecondMin == INFINITY ? INFINITY : deltaSecondMin + 1;
        int newPhiThreshold = deltaThreshold == INFINITY ? INFINITY : deltaThreshold + phiMin - phiSum;

        auto [ it, success ] = history.insert(deltaMinState);
        this->RID(deltaMinState, phiThreshold < deltaSecondMinP1 ? phiThreshold : deltaSecondMinP1, newPhiThreshold, opponentKing, myKing, !isMating, history, tsumeMap);
        history.erase(it);
    }
}

std::unordered_map<Board, Action, Zobrist> DFPN::solveTsume(Board initialState, bool withAttackingKing) {
    std::unordered_map<Board, Action, Zobrist> tsume;
    std::unordered_set<Board, Zobrist> history;
    this->RID(initialState, INFINITY, INFINITY, withAttackingKing, true, true, history, tsume);
    return tsume;
}