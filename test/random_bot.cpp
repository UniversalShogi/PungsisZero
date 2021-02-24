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
#include <sys/types.h>
#include <unistd.h>
#include <filesystem>

using namespace std::literals;

int main() {
    int pid = (int) getpid();
    std::cout << "PID: " << pid << std::endl;
    Board::init();
    // MCTSModel _10b128c(3, 10, 128, 2, 32, 128, 32, 64);
    // _10b128c->to(torch::kCUDA);
    // efficiencyTest(_10b128c);
    // MCTSModel _20b256c(3, 20, 256, 2, 64, 144, 48, 96);
    // _20b256c->to(torch::kCUDA);
    // efficiencyTest(_20b256c);

    // for (auto& entry : std::filesystem::directory_iterator(std::filesystem::path(gameDir))) {
    //     GameResult result;
    //     std::ifstream game(entry.path(), std::ios::binary);
    //     cereal::BinaryInputArchive gameArchive(game);
    //     gameArchive(result);
    //     std::cout << result.actionHistory.size() << std::endl;
    // }
    
    MCTSModel _5b90c(3, 5, 90, 2, 30, 90, 30, 40);
    _5b90c->to(torch::kCUDA);
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
    std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();
    std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
    BatchedMCTS<3> mcts(0, &trainer, sampleGame, true, false, true, 24, 24);
    begin = std::chrono::steady_clock::now();
    while (!mcts.games.empty())
        mcts.step();
    end = std::chrono::steady_clock::now();
    std::cout << "24 ELAPSED TIME: " << std::chrono::duration_cast<std::chrono::microseconds>(end - begin).count() << std::endl;
    // efficiencyTest(_5b90c, 2);
    delete sampleGame;
    return 0;
}