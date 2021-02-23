#ifndef TRAIN_H
#define TRAIN_H

#include "search/mctsmodel.h"
#include "action.h"
#include "board.h"
#include "basics.h"

#include <iostream>
#include <fstream>
#include <filesystem>
#include <string>
#include <sys/types.h>
#include <cereal/archives/binary.hpp>
#include <cereal/types/utility.hpp>
#include <cereal/types/vector.hpp>
#include <unistd.h>
#include <thread>
#include <chrono>
#include <utility>
#include <algorithm>
#include <random>
#include <torch/torch.h>
#include <tuple>

namespace fs = std::filesystem;
namespace chrono = std::chrono;

inline long long currentMillis() {
    return chrono::duration_cast<chrono::milliseconds>(chrono::system_clock::now().time_since_epoch()).count();
}

class Sample {
    public:
    unsigned short moveCount;
    std::vector<std::pair<Action, unsigned short>> mcts_policy;

    void serialize(auto& archive) {
        archive(moveCount, mcts_policy);
    }
};

class GameResult {
    public:
    int winner;
    std::vector<Action> actionHistory;
    std::vector<Board> history;
    std::vector<Sample> senteSamples;
    std::vector<Sample> goteSamples;

    void serialize(auto& archive) {
        archive(winner, actionHistory, senteSamples, goteSamples);
    }

    void calibrate() {
        history.clear();
        history.push_back(Board());

        for (auto& action : actionHistory) {
            Board current = history.back();
            current.inflict(current.currentColour, action);
            current.changeTurn();
            history.push_back(current);
        }
    }
};

template <int n>
class GameTrainer {
    public:
    MCTSModel model;
    torch::optim::Adam optimizer;
    int batchesPerCheckpoint;
    int batchSize;
    int leastSamples;
    int arenaGames;
    fs::path selfPlayDir;
    fs::path modelDir;
    std::vector<int> arenaResults;
    std::random_device rd;
    std::default_random_engine eng;
    float (*binput)[PIECE_NUMBER * n + COLOUR_NUMBER * 3 + 1][FILE_NUMBER][RANK_NUMBER];
    float (*ninput)[DROP_NUMBER * n + 1];
    float valueLossConstant;

    GameTrainer(MCTSModel model, const std::string& _selfPlayDir, const std::string& _modelDir,
        int batchesPerCheckpoint = 100, int batchSize = 256, int leastSamples = 250000, int arenaGames = 100,
        float valueLossConstant = 1.5f, double lr = 6e-5, double l2Penalty = 3e-5)
        : model(model), optimizer(model->parameters(), torch::optim::AdamOptions(lr).weight_decay(l2Penalty))
        , selfPlayDir(_selfPlayDir), modelDir(_modelDir), batchesPerCheckpoint(batchesPerCheckpoint), batchSize(batchSize)
        , leastSamples(leastSamples), arenaGames(arenaGames), valueLossConstant(valueLossConstant), arenaResults(), rd(), eng(rd()) {
        fs::create_directory(selfPlayDir);
        fs::create_directory(modelDir);
        
        if (fs::is_empty(modelDir))
            checkpoint();
        else
            reload();

        binput = new float[batchSize][PIECE_NUMBER * n + COLOUR_NUMBER * 3 + 1][FILE_NUMBER][RANK_NUMBER];
        ninput = new float[batchSize][DROP_NUMBER * n + 1];

        model->eval();
    }

    void forceLrPenalty(double lr, double l2Penalty) {
        for (auto& group : optimizer.param_groups()) {
            if (group.has_options()) {
                torch::optim::AdamOptions& options = (torch::optim::AdamOptions&) group.options();
                options.lr(lr).weight_decay(l2Penalty);
            }
        }
    }

    void reload() {
        std::vector<fs::path> checkpoints;
        for (auto& checkpoint : fs::directory_iterator(modelDir))
            if (checkpoint.path().filename().native().rfind("model", 0) == 0)
                checkpoints.push_back(checkpoint.path());
        std::string fileName = (*std::max_element(checkpoints.begin(), checkpoints.end(), [](auto& a, auto& b) {
            return fs::last_write_time(a) < fs::last_write_time(b);
        })).filename();
        std::ifstream modelIn(modelDir / fileName);
        torch::load(model, modelIn);
        std::ifstream optimIn(modelDir / (fileName + "optim"));
        torch::load(optimizer, optimIn);
    }

    void checkpoint() const {
        std::string current = std::to_string(currentMillis());
        std::ofstream modelOut(modelDir / ("model" + current));
        torch::save(model, modelOut);
        std::ofstream optimOut(std::ofstream(modelDir / ("model" + current + "optim")));
        torch::save(optimizer, optimOut);
    }

    void serialize(const GameResult& result) const {
        std::ofstream gameFile(selfPlayDir / ("game" +
            std::to_string(currentMillis()) + "p" + std::to_string(getpid())),
            std::ios::binary);
        cereal::BinaryOutputArchive gameFileArchive(gameFile);
        gameFileArchive(result);
    }

    void step() {
        std::vector<GameResult> games;
        std::vector<std::tuple<int, bool, int>> sampleIndexes;
        model->train();
        std::vector<fs::path> gameFiles;

        for (auto& gameFile : fs::directory_iterator(selfPlayDir))
            gameFiles.push_back(gameFile.path());
        
        std::sort(gameFiles.begin(), gameFiles.end(), [](auto& a, auto& b) {
            return fs::last_write_time(a) < fs::last_write_time(b);
        });

        while (!gameFiles.empty() && sampleIndexes.size() < leastSamples) {
            std::ifstream gameFile(gameFiles.back());
            gameFiles.pop_back();
            games.emplace_back();
            cereal::BinaryInputArchive gameFileArchive(gameFile);
            gameFileArchive(games.back());
            games.back().calibrate();
            for (int i = 0; i < games.back().senteSamples.size(); i++)
                sampleIndexes.emplace_back(games.size() - 1, 0, i);
            for (int i = 0; i < games.back().goteSamples.size(); i++)
                sampleIndexes.emplace_back(games.size() - 1, 1, i);
        }

        std::shuffle(sampleIndexes.begin(), sampleIndexes.end(), eng);

        int batchesProcessed = 0;

        while (!sampleIndexes.empty()) {
            optimizer.zero_grad();
            std::vector<std::tuple<int, int, int>> batch;

            while (!sampleIndexes.empty() && batch.size() < batchSize) {
                auto& [ gameIndex, player, sampleIndex ] = sampleIndexes.back();
                GameResult& game = games[gameIndex];
                Sample& sample = player == 0 ? game.senteSamples[sampleIndex] : game.goteSamples[sampleIndex];   
                std::vector<Board> states(game.history.begin() + sample.moveCount - std::min<int>(n, sample.moveCount)
                    , game.history.begin() + sample.moveCount);
                while (states.size() < n)
                    states.insert(states.begin(), BEMPTY);
                Board::toInputs(n, binput[batch.size()], ninput[batch.size()], states);
                batch.emplace_back(gameIndex, player, sampleIndex);
                sampleIndexes.pop_back();
            }

            torch::Tensor outputs[2];
            int thisBatchSize = (int) batch.size();
            model->forward(
                torch::from_blob(binput, { thisBatchSize, PIECE_NUMBER * n + COLOUR_NUMBER * 3 + 1, FILE_NUMBER, RANK_NUMBER }, torch::TensorOptions().dtype(torch::kFloat32)).to(torch::kCUDA),
                torch::from_blob(ninput, { thisBatchSize, DROP_NUMBER * n + 1 }, torch::TensorOptions().dtype(torch::kFloat32)).to(torch::kCUDA),
                outputs
            );
            torch::Tensor valueReal = torch::zeros({ thisBatchSize, 3 }, torch::TensorOptions().dtype(torch::kFloat32).device(torch::kCUDA));
            torch::Tensor mcts_policy = torch::zeros({ thisBatchSize, 139, 9, 9 }, torch::TensorOptions().dtype(torch::kFloat32).device(torch::kCUDA));
            
            for (int i = 0; i < batch.size(); i++) {
                auto& [ gameIndex, player, sampleIndex ] = batch[i];
                GameResult& game = games[gameIndex];
                valueReal[i][0] = player == game.winner;
                valueReal[i][1] = game.winner == -1;
                valueReal[i][2] = !player == game.winner;
                Sample& sample = player == 0 ? game.senteSamples[sampleIndex] : game.goteSamples[sampleIndex];
                for (auto& [action, N] : sample.mcts_policy)
                    mcts_policy[i][action.toModelOutput()][action.getPrincipalPosition() / 9][action.getPrincipalPosition() % 9] = N;
            }

            torch::Tensor loss = -(torch::sum(mcts_policy * torch::log(outputs[0])) + torch::sum(valueReal * torch::log(outputs[1]))) / thisBatchSize;
            loss.backward();
            optimizer.step();

            batchesProcessed++;

            if (batchesProcessed % batchesPerCheckpoint == 0)
                checkpoint();
        }

        checkpoint();
        model->eval();
    }

    ~GameTrainer() {
        delete[] binput;
        delete[] ninput;
    }
};
#endif