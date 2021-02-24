#include "board.h"
#include "search/mctsmodel.h"
#include "engine/mctsactionprovider.h"
#include "search/mcts.h"
#include "search/dfpn.h"
#include "batchedselfplay.h"
#include "game.h"
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
    std::cout << trainer.latestModelPath().native() << std::endl;

    while (true) {
        std::cout << "TRAIN START" << std::endl;
        trainer.step();
        std::cout << "TRAIN END" << std::endl;
        std::string lock;
        std::cout << "LOCK: ";
        std::cin >> lock;
    }

    return 0;
}