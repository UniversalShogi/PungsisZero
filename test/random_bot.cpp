#include "board.h"
#include "search/mctsmodel.h"
#include "search/dfpn.h"
#include "batchedselfplay.h"
#include "zobrist.h"

#include <string>
#include <iostream>
#include <fstream>
#include <bitset>
#include <random>
#include <stdlib.h>
#include <torch/torch.h>
#include <time.h>
#include <unordered_map>
#include <chrono>

main() {
    torch::NoGradGuard guard;
    Board::init();
    DFPN dfpn;

    int TSUME_SETUP[FILE_NUMBER][RANK_NUMBER] = {
        { NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE },
        { NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE },
        { NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE },
        { NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE },
        { G_KING, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE },
        { NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE },
        { NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE },
        { NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE },
        { NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE },
    };

    char TSUME_GRAVE[DROP_NUMBER] = { 9, 0, 0, 2, 4, 1, 0, 9, 4, 4, 2, 0, 1, 2 };

    Board tsumeBoard(TSUME_SETUP, TSUME_GRAVE);

    std::unordered_map<Board, Action, Zobrist> solution;
    std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();
    dfpn.solveTsume(tsumeBoard, false, solution);
    std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
    std::cout << "DFPN NODE COUNT: " << dfpn.dfpnTTable.size()
        << ", ELAPSED TIME: " << std::chrono::duration_cast<std::chrono::microseconds>(end - begin).count() << std::endl;
    
    // while (true) {
    //     std::cout << (std::string) tsumeBoard << std::endl;
    //     std::vector<Action> available = tsumeBoard.currentColour == 0 ? std::vector<Action>{solution[tsumeBoard]} : tsumeBoard.getKActions(tsumeBoard.currentColour);

    //     if (available.size() == 0) {
    //         std::cout << "CHECKMATED" << std::endl;
    //         break;
    //     }

    //     for (auto& action: available)
    //         std::cout << (std::string) action << " ";
    //     std::cout << std::endl;
    //     std::cout << "MODE: ";
        
    //     char x;
    //     std::cin >> x;

    //     switch (x) {
    //         case 'M': {
    //             int src[2], dst[2];
    //             bool promote;
    //             std::cout << "SRC: ";
    //             std::cin >> src[0];
    //             std::cin >> src[1];
    //             std::cout << "DST: ";
    //             std::cin >> dst[0];
    //             std::cin >> dst[1];
    //             std::cout << "PROMOTE: ";
    //             std::cin >> promote;
    //             for (auto& action: available)
    //                 if (action.type == MOVE && action.move.src == toBBIndex(src[0], src[1]) && action.move.dst == toBBIndex(dst[0], dst[1]) && action.move.promote == promote) {
    //                     tsumeBoard.inflict(tsumeBoard.currentColour, action);
    //                     tsumeBoard.changeTurn();
    //                     break;
    //                 }
    //             break;
    //         }

    //         case 'D': {
    //             int graveIndex, dst[2];
    //             std::cout << "GRAVEINDEX: ";
    //             std::cin >> graveIndex;
    //             std::cout << "DST: ";
    //             std::cin >> dst[0];
    //             std::cin >> dst[1];
    //             for (auto& action: available)
    //                 if (action.type == DROP && action.drop.dst == toBBIndex(dst[0], dst[1]) && action.drop.graveIndex == graveIndex) {
    //                     tsumeBoard.inflict(tsumeBoard.currentColour, action);
    //                     tsumeBoard.changeTurn();
    //                     break;
    //                 }
    //             break;
    //         }
    //     }
    // }
    
    // Board start;
    // MCTSModel model(3, 10, 128, 2, 32, 128, 32, 64);
    // model->to(torch::kCUDA);

    // for (int epoch = 0; epoch < 10; epoch++) {
    //     std::cout << "EPOCH " << epoch << " START" << std::endl;
    //     std::ofstream file("./epoch" + std::to_string(epoch) + ".epoch");

    //     for (int batch = 1; batch <= 100; batch++) {
    //         BatchedMCTS<3> batchedMCTS(model, Board(), batch);

    //         std::cout << batch << " BATCHED SELFPLAY START" << std::endl;
    //         std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();

    //         for (int i = 0; i < 100; i++) {
    //             batchedMCTS.step();
    //         }
    //         std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
    //         std::cout << batch << " BATCHED SELFPLAY END, ELAPSED TIME: " << std::chrono::duration_cast<std::chrono::microseconds>(end - begin).count()
    //             << ", EFFICIENCY: " << std::chrono::duration_cast<std::chrono::microseconds>(end - begin).count() / batch << std::endl;
    //         file << std::chrono::duration_cast<std::chrono::microseconds>(end - begin).count() / batch << std::endl;
    //     }

    //     file.close();
    //     std::cout << "EPOCH " << epoch << " END" << std::endl;
    // }
}