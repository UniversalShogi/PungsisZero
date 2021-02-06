#include "bitboard.h"
#include "board.h"
#include "action.h"
#include "basics.h"
#include "zobrist.h"
#include "search/dfpn.h"

#include <string>
#include <iostream>
#include <bitset>
#include <random>
#include <stdlib.h>
#include <torch/torch.h>
#include <time.h>
#include <unordered_map>

int main() {
    Board::init();
    DFPN dfpn;

    int TSUME_SETUP[FILE_NUMBER][RANK_NUMBER] = {
        { NONE, G_LANCE, NONE, NONE, NONE, NONE, NONE, NONE, NONE },
        { NONE, G_KING, NONE, NONE, S_SILVER, NONE, NONE, NONE, NONE },
        { NONE, NONE, G_BISHOP, G_PAWN, NONE, NONE, NONE, NONE, NONE },
        { S_ROOK, NONE, NONE, G_SILVER, NONE, NONE, NONE, NONE, NONE },
        { NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE },
        { NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE },
        { NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE },
        { NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE },
        { NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE },
    };

    char TSUME_GRAVE[DROP_NUMBER] = { 0, 0, 1, 1, 0, 1, 0, 17, 4, 3, 1, 4, 0, 1 };

    Board tsumeBoard(TSUME_SETUP, TSUME_GRAVE);

    std::unordered_map<Board, Action, Zobrist> solution = dfpn.solveTsume(tsumeBoard, false);

    while (true) {
        std::cout << (std::string) tsumeBoard << std::endl;
        std::vector<Action> available = tsumeBoard.currentColour == 0 ? std::vector<Action>{solution[tsumeBoard]} : tsumeBoard.getKActions(tsumeBoard.currentColour);

        for (auto& action: available)
            std::cout << (std::string) action << " ";
        std::cout << std::endl;
        std::cout << "MODE: ";
        
        char x;
        std::cin >> x;

        switch (x) {
            case 'M': {
                int src[2], dst[2];
                bool promote;
                std::cout << "SRC: ";
                std::cin >> src[0];
                std::cin >> src[1];
                std::cout << "DST: ";
                std::cin >> dst[0];
                std::cin >> dst[1];
                std::cout << "PROMOTE: ";
                std::cin >> promote;
                for (auto& action: available)
                    if (action.isMove && action.move.src == toBBIndex(src[0], src[1]) && action.move.dst == toBBIndex(dst[0], dst[1]) && action.move.promote == promote) {
                        tsumeBoard.inflict(tsumeBoard.currentColour, action);
                        tsumeBoard.changeTurn();
                        break;
                    }
                break;
            }
            case 'D': {
                int graveIndex, dst[2];
                std::cout << "GRAVEINDEX: ";
                std::cin >> graveIndex;
                std::cout << "DST: ";
                std::cin >> dst[0];
                std::cin >> dst[1];
                for (auto& action: available)
                    if (!action.isMove && action.drop.dst == toBBIndex(dst[0], dst[1]) && action.drop.graveIndex == graveIndex) {
                        tsumeBoard.inflict(tsumeBoard.currentColour, action);
                        tsumeBoard.changeTurn();
                        break;
                    }
                break;
            }
        }
    }
    
    
    return 0;
}