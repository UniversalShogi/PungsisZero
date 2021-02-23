#ifndef BATCHED_SELFPLAY_H
#define BATCHED_SELFPLAY_H

#include "action.h"
#include "board.h"
#include "search/mctsmodel.h"
#include "train.h"

#include <gsl/gsl_rng.h>
#include <gsl/gsl_randist.h>
#include <torch/torch.h>
#include <vector>
#include <time.h>
#include <iostream>
#include <utility>
#include <random>
#include <math.h>
#include <limits>

class BatchedMCTSNode {
    public:
    BatchedMCTSNode* parent;
    int moveCount;
    bool expanded;
    bool lost;
    int N;
    float childP;
    int forced;
    float Q;
    float P;
    std::vector<std::pair<Action, BatchedMCTSNode*>> childs;

    BatchedMCTSNode(BatchedMCTSNode* parent = nullptr) : parent(parent), moveCount(parent == nullptr ? 0 : parent->moveCount + 1), expanded(false), lost(false), N(0), childP(0), forced(0), Q(0), P(0), childs() {}

    void clearChilds() {
        for (auto& [action, node] : childs)
            delete node;
        this->childs.clear();
    }

    ~BatchedMCTSNode() {
        clearChilds();
    }
};

class BatchedMCTSTree {
    public:
    BatchedMCTSNode* initNode;
    BatchedMCTSNode* rootNode;
    BatchedMCTSNode* searchingNode;
    bool dirichletEnabled;
    bool forcedPlayoutEnabled;
    float fpuRoot;
    float fpuNonRoot;
    float puctConstant;
    double dirichletConstant;
    float dirichletEpsilon;
    float forcedSimuConstant;

    std::vector<BatchedMCTSNode*> samples;

    BatchedMCTSTree(bool dirichletEnabled = true, bool forcedPlayoutEnabled = true, float fpuRoot = 0.0f, float fpuNonRoot = 0.2f,
        float puctConstant = 1.1f, double dirichletConstant = 0.15, float dirichletEpsilon = 0.25,
        float forcedSimuConstant = 2) : dirichletEnabled(dirichletEnabled), forcedPlayoutEnabled(forcedPlayoutEnabled), fpuRoot(fpuRoot), fpuNonRoot(fpuNonRoot),
        puctConstant(puctConstant), dirichletConstant(dirichletConstant), dirichletEpsilon(dirichletEpsilon),
        forcedSimuConstant(forcedSimuConstant) {
        initNode = new BatchedMCTSNode();
        rootNode = initNode;
        searchingNode = rootNode;
    }

    void addDirichlet(gsl_rng* r);
    float expand(const std::vector<Action>& availables, int j, torch::Tensor outputs[2]);
    void expandRaw(const std::vector<Action>& availables);

    void opponentMove(Action opponentAction) {
        for (auto& [action, child] : rootNode->childs)
            if (action == opponentAction) {
                this->rootNode = child;
                this->searchingNode = this->rootNode;
            }
    }

    bool search(std::vector<Board>& states);
    void backpropagate(float V);
    Action selectByProportionalPolicy(std::default_random_engine& eng, double temperature);
    Action selectByNaivePolicy(std::default_random_engine& eng);

    ~BatchedMCTSTree() {
        delete initNode;
    }
};

class BatchedMCTSGame;

class BatchedMCTSPlayer {
    public:
    BatchedMCTSTree* tree;
    BatchedMCTSGame* game;
    int moveCount;
    int smallDepth;
    int bigDepth;
    int currDepth;
    double startTemp;
    double endTemp;
    int halfLife;
    int naivePlayout;
    double bigSimuProb;
    double temp;
    std::string name;
    std::vector<BatchedMCTSNode*> samples;
    bool isFull;

    BatchedMCTSPlayer(BatchedMCTSTree* tree, int smallDepth, int bigDepth, const std::string& name,
        double startTemp = 0.8, double endTemp = 0.2, int halfLife = 15, int naivePlayout = 7, double bigSimuProb = 0.25)
        : tree(tree), moveCount(0), smallDepth(smallDepth), bigDepth(bigDepth), currDepth(0), name(name), samples(), startTemp(startTemp)
        , endTemp(endTemp), halfLife(halfLife), naivePlayout(naivePlayout), bigSimuProb(bigSimuProb), temp(startTemp), isFull(false) {}

    bool step(std::default_random_engine& eng, gsl_rng* r, std::vector<Board>& states);

    ~BatchedMCTSPlayer() {
        delete tree;
    }
};

class BatchedMCTSGame {
    public:
    BatchedMCTSPlayer* sente;
    BatchedMCTSPlayer* gote;
    std::vector<Board> history;
    std::vector<Action> actionHistory;
    Board current;
    int winner = -1;
    int moveCount = 0;
    bool ended = false;
    int moveCountThreshold;

    BatchedMCTSGame(BatchedMCTSPlayer* sente, BatchedMCTSPlayer* gote, Board initState = Board(), int moveCountThreshold = 300)
        : sente(sente), gote(gote), history(), actionHistory(), moveCountThreshold(moveCountThreshold) {
        sente->game = this;
        gote->game = this;
        history.push_back(initState);
    }

    BatchedMCTSPlayer* getCurrentPlayer() {
        return this->current.currentColour == 0 ? this->sente : this->gote;
    }

    BatchedMCTSPlayer* getOpponent() {
        return this->current.currentColour == 1 ? this->sente : this->gote;
    }

    void act(Action action, gsl_rng* r);

    ~BatchedMCTSGame() {
        delete sente;
        delete gote;
    }
};

inline BatchedMCTSTree* newTreeCopy(BatchedMCTSTree* sampleTree) {
    return new BatchedMCTSTree(sampleTree->dirichletEnabled
        , sampleTree->forcedPlayoutEnabled, sampleTree->fpuRoot
        , sampleTree->fpuNonRoot, sampleTree->puctConstant
        , sampleTree->dirichletConstant, sampleTree->dirichletEpsilon
        , sampleTree->forcedSimuConstant);
}

inline BatchedMCTSPlayer* newPlayerCopy(BatchedMCTSPlayer* samplePlayer) {
    return new BatchedMCTSPlayer(newTreeCopy(samplePlayer->tree), samplePlayer->smallDepth
        , samplePlayer->bigDepth, samplePlayer->name, samplePlayer->startTemp
        , samplePlayer->endTemp, samplePlayer->halfLife, samplePlayer->naivePlayout
        , samplePlayer->bigSimuProb);
}

inline BatchedMCTSGame* newGameCopy(BatchedMCTSGame* sampleGame) {
    return new BatchedMCTSGame(newPlayerCopy(sampleGame->sente),
        newPlayerCopy(sampleGame->gote),
        sampleGame->history.front(),
        sampleGame->moveCountThreshold
    );
}

template <int n = 3>
class BatchedMCTS {
    public:
    GameTrainer<n>* trainer;
    bool saveGames;

    gsl_rng* r;
    std::vector<BatchedMCTSGame*> games;
    std::vector<BatchedMCTSTree*> pending;
    std::vector<std::vector<Board>> pendingStates;
    float (*binput)[PIECE_NUMBER * n + COLOUR_NUMBER * 3 + 1][FILE_NUMBER][RANK_NUMBER];
    float (*ninput)[DROP_NUMBER * n + 1];
    int gameCtr;
    int maxGames;

    BatchedMCTS(GameTrainer<n>* trainer, bool saveGames, BatchedMCTSGame* sampleGame, int gameCount = 100, int maxGames = 1000)
        : trainer(trainer), saveGames(saveGames), r(gsl_rng_alloc(gsl_rng_mt19937)), games(), pending()
        , gameCtr(0), maxGames(maxGames) {
        trainer->model->eval();
        gsl_rng_set(r, trainer->rd());

        gameCtr += gameCount;
        binput = new float[gameCount][PIECE_NUMBER * n + COLOUR_NUMBER * 3 + 1][FILE_NUMBER][RANK_NUMBER];
        ninput = new float[gameCount][DROP_NUMBER * n + 1];

        for (int i = 0; i < gameCount; i++)
            games.push_back(newGameCopy(sampleGame));
    }

    void step() {
        torch::NoGradGuard guard;
        for (int i = 0; i < games.size(); i++) {
            BatchedMCTSGame* game = games[i];
            std::vector<Board> states(game->history.end() - std::min<int>(n, game->history.size()), game->history.end());

            while (!game->getCurrentPlayer()->step(this->trainer->eng, this->r, states)) {
                states = std::vector<Board>(game->history.end() - std::min<int>(n, game->history.size()), game->history.end());
                if (game->ended) {
                    if (saveGames) {
                        GameResult result;
                        result.winner = game->winner;
                        result.actionHistory = game->actionHistory;

                        for (auto& node : game->sente->samples) {
                            Sample sample;
                            sample.moveCount = node->moveCount;
                            
                            for (auto& [action, child] : node->childs)
                                sample.mcts_policy.emplace_back(action, child->N);
                            result.senteSamples.push_back(sample);
                        }

                        for (auto& node : game->gote->samples) {
                            Sample sample;
                            sample.moveCount = node->moveCount;
                            
                            for (auto& [action, child] : node->childs)
                                sample.mcts_policy.emplace_back(action, child->N);
                            result.goteSamples.push_back(sample);
                        }

                        trainer->serialize(result);
                    } else
                        trainer->arenaResults.push_back(game->winner);

                    if (gameCtr + 1 < maxGames) {
                        gameCtr++;
                        BatchedMCTSGame* newGame = newGameCopy(game);
                        games[i] = newGame;
                        delete game;
                        game = games[i];
                    } else {
                        delete game;
                        game = nullptr;
                        games.erase(games.begin() + i--);
                        break;
                    }
                }
            }

            if (game == nullptr)
                continue;
            this->pending.push_back(game->getCurrentPlayer()->tree);
            while (states.size() < n)
                states.insert(states.begin(), BEMPTY);
            this->pendingStates.push_back(std::move(states));
        }

        if (pending.empty())
            return;

        for (int j = 0; j < pendingStates.size(); j++)
            Board::toInputs(n, binput[j], ninput[j], pendingStates[j]);
        torch::Tensor outputs[2];
        trainer->model->forward(
            torch::from_blob(binput, { (int) pendingStates.size(), PIECE_NUMBER * n + COLOUR_NUMBER * 3 + 1, FILE_NUMBER, RANK_NUMBER }, torch::TensorOptions().dtype(torch::kFloat32)).to(torch::kCUDA),
            torch::from_blob(ninput, { (int) pendingStates.size(), DROP_NUMBER * n + 1 }, torch::TensorOptions().dtype(torch::kFloat32)).to(torch::kCUDA),
            outputs
        );

        for (int j = 0; j < pending.size(); j++) {
            bool addDirichlet = pending[j]->dirichletEnabled && !pending[j]->rootNode->expanded;
            pending[j]->backpropagate(pending[j]->expand(pendingStates[j].back().getKActions(pendingStates[j].back().currentColour), j, outputs));
            if (addDirichlet)
                pending[j]->addDirichlet(r);
        }

        pending.clear();
        pendingStates.clear();
    }

    ~BatchedMCTS() {
        for (auto& game : games)
            delete game;
        gsl_rng_free(r);
        delete[] binput;
        delete[] ninput;
    }
};


#endif