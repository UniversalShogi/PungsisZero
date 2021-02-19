#ifndef BATCHED_SELFPLAY_H
#define BATCHED_SELFPLAY_H

#include "action.h"
#include "board.h"
#include "search/mctsmodel.h"

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
    float activateDFPN;
    std::vector<std::pair<Action, BatchedMCTSNode*>> childs;

    BatchedMCTSNode(BatchedMCTSNode* parent = nullptr) : parent(parent), moveCount(parent == nullptr ? 0 : parent->moveCount + 1), expanded(false), lost(false), N(0), childP(0), forced(0), Q(0), P(0), activateDFPN(0), childs() {}
    
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
    float expand(const std::vector<Action>& availables, int j, torch::Tensor outputs[3]);
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

    BatchedMCTSPlayer(BatchedMCTSTree* tree, int smallDepth, int bigDepth, std::string name,
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

    BatchedMCTSGame(BatchedMCTSPlayer* sente, BatchedMCTSPlayer* gote, Board initState = Board(), int moveCountThreshold = 500)
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

template <int n = 3>
class BatchedMCTS {
    public:
    MCTSModel model;
    std::random_device rd;
    std::default_random_engine eng;
    gsl_rng* r;
    std::vector<BatchedMCTSGame*> games;
    std::vector<BatchedMCTSTree*> pending;
    std::vector<std::vector<Board>> pendingStates;
    float (*binput)[PIECE_NUMBER * n + COLOUR_NUMBER * 3 + 1][FILE_NUMBER][RANK_NUMBER];
    float (*ninput)[DROP_NUMBER * n + 1];
    int gameCtr;
    int maxGames;
    Board initialState;

    BatchedMCTS(MCTSModel model, Board initialState = Board(), int gameCount = 100, int maxGames = 1000)
        : model(model), rd(), eng(rd()), r(gsl_rng_alloc(gsl_rng_mt19937)), games(), pending()
        , gameCtr(0), maxGames(maxGames), initialState(initialState) {
        this->model->eval();
        gsl_rng_set(r, rd());

        gameCtr += gameCount;
        binput = new float[gameCount][PIECE_NUMBER * n + COLOUR_NUMBER * 3 + 1][FILE_NUMBER][RANK_NUMBER];
        ninput = new float[gameCount][DROP_NUMBER * n + 1];

        for (int i = 0; i < gameCount; i++) {
            games.push_back(new BatchedMCTSGame(
                new BatchedMCTSPlayer(new BatchedMCTSTree(), 200, 600, "IDIOT_SENTE"),
                new BatchedMCTSPlayer(new BatchedMCTSTree(), 200, 600, "IDIOT_GOTE"),
                initialState
            ));
        }
    }

    void step() {
        for (int i = 0; i < games.size(); i++) {
            BatchedMCTSGame* game = games[i];
            std::vector<Board> states(game->history.end() - std::min<int>(n, game->history.size()), game->history.end());

            while (!game->getCurrentPlayer()->step(this->eng, this->r, states)) {
                states = std::vector<Board>(game->history.end() - std::min<int>(n, game->history.size()), game->history.end());
                if (game->ended) {
                    delete game;
                    game = nullptr;

                    if (gameCtr + 1 < maxGames) {
                        gameCtr++;
                        
                        games[i] = new BatchedMCTSGame(
                            new BatchedMCTSPlayer(new BatchedMCTSTree(), 200, 600, "IDIOT_SENTE"),
                            new BatchedMCTSPlayer(new BatchedMCTSTree(), 200, 600, "IDIOT_GOTE"),
                            initialState
                        );
                        game = games[i];
                    } else {
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

        for (int j = 0; j < pendingStates.size(); j++)
            Board::toInputs(n, binput[j], ninput[j], pendingStates[j]);
        torch::Tensor outputs[3];
        model->forward(
            torch::from_blob(binput, { (int) pendingStates.size(), PIECE_NUMBER * 3 + COLOUR_NUMBER * 3 + 1, FILE_NUMBER, RANK_NUMBER }, torch::TensorOptions().dtype(torch::kFloat32)).clone().to(torch::kCUDA),
            torch::from_blob(ninput, { (int) pendingStates.size(), DROP_NUMBER * 3 + 1 }, torch::TensorOptions().dtype(torch::kFloat32)).clone().to(torch::kCUDA),
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
    }
};


#endif