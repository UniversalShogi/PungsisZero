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
#include <mutex>
#include <thread>
#include <filesystem>

using namespace std::literals;

int main() {
    Board::init();
    
    MCTSModel _5b90c(3, 5, 90, 2, 30, 90, 30, 40);
    _5b90c->to(torch::kCUDA);
    int slaveNum;
    std::cout << "SLAVENUM: ";
    std::cin >> slaveNum;
    std::string selfPlayDir;
    std::cout << "SELFPLAYDIR: ";
    std::cin >> selfPlayDir;
    std::string modelDir;
    std::cout << "MODELDIR: ";
    std::cin >> modelDir;
    std::string kifuDir;
    std::cout << "KIFUDIR: ";
    std::cin >> kifuDir;
    GameTrainer<3> trainer(_5b90c, selfPlayDir, modelDir, kifuDir);
    
    BatchedMCTSGame* sampleGame = new BatchedMCTSGame(
        new BatchedMCTSPlayer(new BatchedMCTSTree(), 200, 600, "IDIOT_SENTE"),
        new BatchedMCTSPlayer(new BatchedMCTSTree(), 200, 600, "IDIOT_GOTE")
    );

    for (int i = 0;; i++) {
        std::cout << "CYCLE " << i << " START" << std::endl;
        BatchedMCTS<3> mcts(slaveNum, &trainer, sampleGame, true, false, false, 16, 16);
        while (!mcts.games.empty())
            mcts.step();
        std::cout << "CYCLE " << i << " END" << std::endl;
        // std::string lock;
        // std::cout << "LOCK: ";
        // std::cin >> lock;
        trainer.reload();
    }

    delete sampleGame;
    return 0;
}