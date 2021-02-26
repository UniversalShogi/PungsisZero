#include "board.h"
#include "search/mctsmodel.h"
#include "engine/mctsactionprovider.h"
#include "search/mcts.h"
#include "search/dfpn.h"
#include "batchedselfplay.h"
#include "game.h"
#include "zobrist.h"
#include "kifu.h"

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
        std::filesystem::path originalPath = trainer.latestModelPath();
        std::cout << "TRAIN START" << std::endl;
        trainer.step();
        std::cout << "TRAIN END" << std::endl;
        std::ifstream originalIn(originalPath);
        MCTSModel original(3, 5, 90, 2, 30, 90, 30, 40);
        original->to(torch::kCUDA);
        torch::load(original, originalIn);
        int winCount = 0, loseCount = 0, drawCount = 0;

        for (int i = 0; i < 10; i++) {
            std::cout << "ARENA " << i << " START" << std::endl;
            MCTSActionProvider<3>* sente = new MCTSActionProvider<3>(new MCTS<3>(i % 2 == 0 ? trainer.model : original)
                , 600, 600, "ARENA_SENTE", 0.5, 0.2, 15, 0, 1);
            MCTSActionProvider<3>* gote = new MCTSActionProvider<3>(new MCTS<3>(i % 2 == 0 ? original : trainer.model)
                , 600, 600, "ARENA_GOTE", 0.5, 0.2, 15, 0, 1);
            Game arena(sente, gote, false);

            while (!arena.ended && arena.moveCount < 500) {
                std::cout << (std::string) arena.current << std::endl;
                arena.step();
            }
            
            if (arena.winner == i % 2) {
                std::cout << "TRAINED WON" << std::endl;
                winCount += 1;
            } else if (arena.winner == !(i % 2)) {
                std::cout << "TRAINED LOST" << std::endl;
                loseCount += 1;
            } else {
                std::cout << "TRAINED DREW" << std::endl;
                drawCount += 1;
            }

            trainer.saveKifu(100, Kifu::toKifu(arena));

            delete sente;
            delete gote;
        }

        std::cout << "TRAINED WON " << winCount << " GAMES, LOST " << loseCount << "GAMES, DREW " << drawCount << " GAMES" << std::endl;
        std::string lock;
        std::cout << "LOCK: ";
        std::cin >> lock;
    }

    return 0;
}