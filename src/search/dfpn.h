#ifndef DFPN_H
#define DFPN_H

#include "../action.h"
#include "../board.h"
#include "../zobrist.h"

#include <utility>
#include <unordered_map>
#include <vector>
#include <tuple>
#include <queue>
#include <limits>

typedef std::pair<Board, std::tuple<int, int, int, bool>> DFPNEntry;

#ifndef TDFPN_H
constexpr int LULFINITY = std::numeric_limits<int>::max();
#endif

class DFPNEntryCompare {
    public:
    bool operator()(const DFPNEntry& a, const DFPNEntry& b) {
        return std::get<2>(a.second) > std::get<2>(b.second);
    }
};

class DFPN {
    public:
    std::unordered_map<Board, std::tuple<int, int, int, bool>, Zobrist> dfpnTTable;

    std::unordered_map<Board, Action, Zobrist> solveTsume(Board initialState, bool withAttackingKing);

    private:
    void RID(Board state, int deltaThreshold, int phiThreshold, bool myKing, bool opponentKing, bool isMating, std::unordered_map<Board, Action, Zobrist>& tsumeMap);

    auto& lookupTTable(Board state) {
        if (this->dfpnTTable.find(state) == this->dfpnTTable.end())
            this->dfpnTTable[state] = std::make_tuple(1, 1, 1, false);
        
        return this->dfpnTTable[state];
    }

    void smallTreeGC() {
        if (this->dfpnTTable.size() >= 3600000) {
            std::cout << "SmallTreeGC1 START, ENTRIES: " << this->dfpnTTable.size() << std::endl;
            std::priority_queue<DFPNEntry, std::vector<DFPNEntry>, DFPNEntryCompare> pq(this->dfpnTTable.begin(), this->dfpnTTable.end());
            
            while (this->dfpnTTable.size() > 3000000) {
                auto [ board, node ] = pq.top();
                auto [ delta, phi, treeSize, visiting ] = node;
                if (!visiting && delta != LULFINITY && phi != LULFINITY)
                    this->dfpnTTable.erase(board);
                pq.pop();
            }
            std::cout << "SmallTreeGC1 END, ENTRIES: " << this->dfpnTTable.size() << std::endl;
        }
    }

    void smallTreeGC2(int threshold = 10) {
        if (this->dfpnTTable.size() >= 3600000) {
            std::cout << "SmallTreeGC2 START, ENTRIES: " << this->dfpnTTable.size() << std::endl;
            
            for (auto& [ board, node ] : this->dfpnTTable) {
                auto [ delta, phi, treeSize, visiting ] = node;
                if (!visiting && delta != LULFINITY && phi != LULFINITY && treeSize <= 10)
                    this->dfpnTTable.erase(board);
            }

            std::cout << "SmallTreeGC2 END, ENTRIES: " << this->dfpnTTable.size() << std::endl;
        }
    }
};

#endif