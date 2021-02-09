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

#include <string>
#include <iostream>
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
    ActionProvider* sente = new MCTSActionProvider(model, start, 800);
    ActionProvider* gote = new MCTSActionProvider(model, start, 800);
    Game selfPlay(sente, gote, false);
    std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();
    selfPlay.step();
    std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
    std::cout << std::chrono::duration_cast<std::chrono::nanoseconds>(end - begin).count() << std::endl;
    std::cout << (std::string) selfPlay.current << std::endl;
}