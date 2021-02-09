#include "bitboard.h"
#include "board.h"
#include "action.h"
#include "basics.h"
#include "zobrist.h"
#include "game.h"
#include "search/mcts.h"
#include "actionprovider.h"
#include "engine/mctsactionprovider.h"
#include "search/mctsmodel.h"
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

main() {
    Board::init();
    Board start;
    MCTSModel model;
    int depth;
    std::cout << "DEPTH: ";
    std::cin >> depth;
    ActionProvider* sente = new MCTSActionProvider(model, start, depth, "MCTS_Sente");
    ActionProvider* gote = new MCTSActionProvider(model, start, depth, "MCTS_Gote");
    Game selfPlay(sente, gote, false);
    std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();

    while (!selfPlay.ended) {
        selfPlay.step();
        std::cout << (std::string) selfPlay.current << std::endl;
    }

    std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
    std::cout << std::chrono::duration_cast<std::chrono::nanoseconds>(end - begin).count() << std::endl;
    std::ofstream file;
    file.open("./kifu.txt", std::ofstream::out);
    file << Kifu::toKifu(selfPlay);
    file.close();
}