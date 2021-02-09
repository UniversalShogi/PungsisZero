#ifndef TDFPN_H
#define TDFPN_H

#include "../action.h"
#include "../board.h"
#include "../zobrist.h"

#include <limits>
#include <utility>
#include <unordered_map>
#include <vector>

#ifndef DFPN_H
constexpr int LULFINITY = std::numeric_limits<int>::max();
#endif

class TDFPNNode {
    public:
    Action action;
    Board state;
    int delta;
    int phi;
    bool expanded;
    std::vector<TDFPNNode*> childs;

    ~TDFPNNode() {
        for (auto& child : childs)
            delete child;
    }
};

class TDFPN {
    public:
    std::unordered_map<Board, Action, Zobrist> solveTsume(Board initialState, bool withAttackingKing);
    TDFPNNode* LULFINITY_DELTA;

    TDFPN() {
        LULFINITY_DELTA = new TDFPNNode{Action(MoveAction{0, 0, false}), Board(), LULFINITY, 0, false, std::vector<TDFPNNode*>()};
    }

    private:
    void RID(TDFPNNode* node, int deltaThreshold, int phiThreshold, bool myKing, bool opponentKing, bool isMating, std::vector<Board>& history, std::unordered_map<Board, Action, Zobrist>& tsumeMap);
};

#endif