#ifndef GAME_H
#define GAME_H

#include "board.h"
#include "actionprovider.h"

#include <vector>
#include <algorithm>

class Game {
    public:
    std::vector<Board> history;
    std::vector<Action> actionHistory;
    Board current;
    ActionProvider* sente;
    ActionProvider* gote;
    int winner = -1;
    int movecount = 0;
    bool ended = false;
    bool restricted;

    Game(ActionProvider* sente, ActionProvider* gote, bool restricted=true) : current(), history(), sente(sente), gote(gote), restricted(restricted) {
        history.push_back(Board(this->current));
    }
    
    void step() {
        if (this->ended)
            return;
        Action action;

        if (current.currentColour == 0)
            action = sente->nextAction(history);
        else
            action = gote->nextAction(history);
        
        actionHistory.push_back(action);

        if (restricted && (action.type == MOVE || action.type == DROP)) {
            std::vector<Action> availables = current.getKActions(current.currentColour);
            if (std::find(availables.begin(), availables.end(), action) == availables.end()) {
                action = Action(ILLEGAL);
                actionHistory.push_back(action);
            } else {
                Board currentcpy(current);
                currentcpy.inflict(current.currentColour, action);
                currentcpy.changeTurn();
                if (std::count(history.begin(), history.end(), currentcpy) >= 4) {
                    if (currentcpy.getAttackers(current.currentColour, currentcpy.pieces[kingOf(!current.currentColour)].first()))
                        action = Action(ILLEGAL);
                    else
                        action = Action(SENNICHITE);
                    actionHistory.push_back(action);
                }
            }
        }
        
        if (action.type == RESIGN || action.type == ILLEGAL) {
            winner = !current.currentColour;
            ended = true;
        } else if (action.type == SENNICHITE || action.type == JISHOGI) {
            winner = -1;
            ended = true;
        } else {
            if (current.currentColour == 0)
                gote->opponentMove(action);
            else
                sente->opponentMove(action);
            current.inflict(current.currentColour, action);
            current.changeTurn();
            history.push_back(Board(current));
            movecount++;
        }
    }
};

#endif