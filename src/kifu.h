#ifndef KIFU_H
#define KIFU_H

#include <string>
#include <iomanip>
#include <sstream>
#include <iostream>
#include <chrono>
#include <time.h>

#include "action.h"
#include "basics.h"
#include "game.h"

constexpr char FILE_CHAR[] = "１２３４５６７８９";
constexpr char RANK_CHAR[] = "一二三四五六七八九";

class Kifu {
    public:
    static std::string toKifu(Game game) {
        std::stringstream kifu;
        time_t now = time(NULL);
        kifu << std::put_time(localtime(&now), "#KIF version=2.0 encoding=UTF-8\n\
開始日時：%Y/%m/%d\n\
場所：PungsisZero\n\
持ち時間：5分+30秒\n\
手合割：平手\n") << "先手：" << game.sente->getName() << "\n後手：" << game.gote->getName() << "\n手数----指手---------消費時間--";

        for (int i = 0; i < game.actionHistory.size(); i++) {
            Action action = game.actionHistory[i];
            time_t pmoveCount = (i + 2) / 2;
            kifu << '\n' << i + 1 << "   ";

            switch (action.type) {
                case MOVE:
                    kifu << std::string(FILE_CHAR + action.move.dst / 9 * 3, 3)
                        << std::string(RANK_CHAR + action.move.dst % 9 * 3, 3);
                    if (PROMOTED[action.move.piece] && !isPawnLike(action.move.piece - 1) && !isRookLike(action.move.piece) && !isBishopLike(action.move.piece))
                        kifu << "成" << std::string(PIECE_CHAR + (action.move.piece - 1) * 3, 3);
                    else
                        kifu << std::string(PIECE_CHAR + action.move.piece * 3, 3);
                    if (action.move.promote)
                        kifu << "成";
                    kifu << '(' << action.move.src / 9 + 1 << action.move.src % 9 + 1 << ')';   
                    break;
                case DROP:
                    kifu << std::string(FILE_CHAR + action.drop.dst / 9 * 3, 3)
                        << std::string(RANK_CHAR + action.drop.dst % 9 * 3, 3)
                        << std::string(DROP_CHAR + action.drop.graveIndex * 3, 3)
                        << "打";
                    break;
                case ILLEGAL:
                    kifu << "反則手";
                    break;
                case RESIGN:
                    kifu << "投了";
                    break;
                case SENNICHITE:
                    kifu << "千日手";
                    break;
                case JISHOGI:
                    kifu << "持将棋";
                    break;
            }

            kifu << std::put_time(localtime(&pmoveCount), "   (0:1/%H:%M:%S)");
        }

        kifu << std::flush;

        return kifu.str();
    }
};

#endif