#ifndef DFPN_H
#define DFPN_H

#include "../action.h"
#include "../board.h"
#include "../zobrist.h"

#include <utility>
#include <unordered_map>
#include <unordered_set>

class DFPN {
    public:
    std::unordered_map<Board, std::pair<int, int>, Zobrist> dfpnTTable;

    std::unordered_map<Board, Action, Zobrist> solveTsume(Board initialState, bool withAttackingKing);

    private:
    void RID(Board state, int deltaThreshold, int phiThreshold, bool myKing, bool opponentKing, bool isMating, std::unordered_set<Board, Zobrist>& history, std::unordered_map<Board, Action, Zobrist>& tsumeMap);

    auto& lookupTTable(Board state) {
        if (this->dfpnTTable.find(state) == this->dfpnTTable.end())
            this->dfpnTTable[state] = std::make_pair(1, 1);
        
        return this->dfpnTTable[state];
    }
};

#endif