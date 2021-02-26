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
    Board::init();
    int TSUME_SETUP[FILE_NUMBER][RANK_NUMBER] = {
        { NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE },
        { NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE },
        { NONE, S_GOLD, S_BISHOP, NONE, NONE, NONE, NONE, NONE, NONE },
        { NONE, G_GOLD, NONE, NONE, NONE, NONE, NONE, NONE, NONE },
        { G_KING, NONE, S_GOLD, NONE, NONE, NONE, NONE, NONE, S_KING },
        { NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE },
        { NONE, S_GOLD, NONE, NONE, NONE, NONE, NONE, NONE, NONE },
        { NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE },
        { NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE },
    };

    char TSUME_GRAVE[DROP_NUMBER] = { 1, };
    Board tsumeBoard(TSUME_SETUP, TSUME_GRAVE);
    std::cout << (std::string) tsumeBoard << std::endl;
    std::vector<Action> available = tsumeBoard.getKActions(tsumeBoard.currentColour);

    for (auto& action: available)
        std::cout << (std::string) action << " ";
    return 0;
}