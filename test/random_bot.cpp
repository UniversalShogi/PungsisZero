#include "bitboard.h"
#include "board.h"
#include "basics.h"
#include "pungsiszero.h"

#include <string>
#include <iostream>
#include <bitset>
#include <random>
#include <stdlib.h>
#include <torch/torch.h>
#include <time.h>

int main() {
    Board::init();
    Board a;
    PungsisZero pungsisZero;
    torch::Tensor randInput = torch::rand({1, 126, 9, 9});
    torch::Tensor outputs[2];
    pungsisZero.get()->forward(randInput, outputs);
    std::cout << outputs[1] << std::endl;

    while (true) {
        std::cout << (std::string) a << std::endl;
        std::vector<MoveAction> availableMoves = a.getAvailableMoves(a.currentColour);
        std::vector<DropAction> availableDrops = a.getAvailableDrops(a.currentColour);

        for (MoveAction& move : availableMoves)
            std::cout << (std::string) move << " ";
        std::cout << std::endl;
        for (DropAction& drop : availableDrops)
            std::cout << (std::string) drop << " ";
        std::cout << std::endl;
        std::cout << "PAWN DROP MASK" << std::endl << (std::string) a.pawnDropMask[a.currentColour] << std::endl;
        std::cout << "CHECKS" << std::endl << (std::string) a.getAttackers(!a.currentColour, a.pieces[kingOf(a.currentColour)].first()) << std::endl;
        std::cout << "PINNED" << std::endl << (std::string) a.getPinning(!a.currentColour, a.pieces[kingOf(a.currentColour)].first());
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
                for (MoveAction& move : availableMoves)
                    if (move.src == toBBIndex(src[0], src[1]) && move.dst == toBBIndex(dst[0], dst[1]) && move.promote == promote) {
                        a.inflict(a.currentColour, move);
                        a.changeTurn();
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
                for (DropAction& drop : availableDrops)
                    if (drop.dst == toBBIndex(dst[0], dst[1]) && drop.graveIndex == graveIndex) {
                        a.inflict(a.currentColour, drop);
                        a.changeTurn();
                        break;
                    }
                break;
            }
        }
    }

    return 0;
}