#include "basics.h"
#include "bitboard.h"
#include "board.h"
#include "intrinsics.h"

#include <string>
#include <iostream>
#include <vector>

Board::operator std::string() {
    char board[FILE_NUMBER * RANK_NUMBER * 4 + RANK_NUMBER + 1] = {};
    char* ptr = board;
    
    for (int r = RANK_ONE; r <= RANK_NINE; r++) {
        for (int f = FILE_NINE; f >= FILE_ONE; f--) {
            
            for (int piece = 0; piece < PIECE_NUMBER; piece++)
                if (this->pieces[piece].test(toBBIndex(f, r))) {
                    const char* start = PIECE_CHAR + piece * 3;
                    std::copy(start, start + 3, ptr);
                    ptr += 3;
                    *(ptr++) = colourOf(piece) ? 'G': 'S';
                    goto filled;
                }
            std::copy(NOPIECE_CHAR, NOPIECE_CHAR + 3, ptr);
            ptr += 3;
            *(ptr++) = 'N';
            filled:
            ;
        }

        *(ptr++) = '\n';
    }

    std::string output(board);

    output += "SENTE:";

    for (int drop = 0; drop < DROP_NUMBER / 2; drop++) {
        output += " ";
        output += std::string(DROP_CHAR + drop * 3, 3);
        output += " * ";
        output += std::to_string(this->graveInfo[drop]);
    }

    output += "\nGOTE:";

    for (int drop = DROP_NUMBER / 2; drop < DROP_NUMBER; drop++) {
        output += " ";
        output += std::string(DROP_CHAR + drop * 3, 3);
        output += " * ";
        output += std::to_string(this->graveInfo[drop]);
    }
    
    return output;
}

BitBoard FILE_BB[FILE_NUMBER];
BitBoard RANK_BB[RANK_NUMBER];
BitBoard LINE[FILE_NUMBER * RANK_NUMBER][FILE_NUMBER * RANK_NUMBER];
BitBoard SEGMENT[FILE_NUMBER * RANK_NUMBER][FILE_NUMBER * RANK_NUMBER];
BitBoard SETUP[PIECE_NUMBER];
BitBoard PAWN_DROP_MASK[COLOUR_NUMBER][FILE_NUMBER];
BitBoard LANCE_DROP_MASK[COLOUR_NUMBER];
BitBoard KNIGHT_DROP_MASK[COLOUR_NUMBER];
BitBoard PAWN_FORCE_PROMOTION_MASK[COLOUR_NUMBER];
BitBoard LANCE_FORCE_PROMOTION_MASK[COLOUR_NUMBER];
BitBoard KNIGHT_FORCE_PROMOTION_MASK[COLOUR_NUMBER];
BitBoard STEP_MOVES[PIECE_NUMBER][FILE_NUMBER * RANK_NUMBER];
BitBoard ROOK_FILE_MOVES[FILE_NUMBER * RANK_NUMBER][ROOK_PEXT];
BitBoard ROOK_RANK_MOVES[FILE_NUMBER * RANK_NUMBER][ROOK_PEXT];
BitBoard LANCE_MOVES[COLOUR_NUMBER][FILE_NUMBER * RANK_NUMBER][ROOK_PEXT];
BitBoard BISHOP_MASK[FILE_NUMBER * RANK_NUMBER];
BitBoard BISHOP_MOVES[FILE_NUMBER * RANK_NUMBER][ROOK_PEXT * ROOK_PEXT];
BitBoard STEP_MOVES_INVERT[PIECE_NUMBER][FILE_NUMBER * RANK_NUMBER];
BitBoard ROOK_FILE_MOVES_INVERT[FILE_NUMBER * RANK_NUMBER][ROOK_PEXT];
BitBoard ROOK_RANK_MOVES_INVERT[FILE_NUMBER * RANK_NUMBER][ROOK_PEXT];
BitBoard LANCE_MOVES_INVERT[COLOUR_NUMBER][FILE_NUMBER * RANK_NUMBER][ROOK_PEXT];
BitBoard BISHOP_MOVES_INVERT[FILE_NUMBER * RANK_NUMBER][ROOK_PEXT * ROOK_PEXT];
BitBoard PROMOTE_MASK[COLOUR_NUMBER];

int DIAGONAL[] = { NE, NW, SE, SW };
int NS[] = { N, S };
int EW[] = { E, W };

void Board::init() {
    for (int f = 0; f < FILE_NUMBER; f++)
        for (int r = 0; r < RANK_NUMBER; r++) {
            FILE_BB[f].set(toBBIndex(f, r));
            RANK_BB[r].set(toBBIndex(f, r));
        }
    for (int f = 0; f < FILE_NUMBER; f++)
        for (int r = 0; r < RANK_NUMBER; r++) {
            for (int power : PIECE_POWER[S_KING]) {
                BitBoard line;
                BitBoard segment;

                for (int i = -8; i <= 8; i++) {
                    int dst[2];
                    calculateQueenDestination(f, r, power, dst, i);
                    if (isInBound(dst[0], dst[1]))
                        line.set(toBBIndex(dst[0], dst[1]));
                }

                for (int i = -8; i <= 8; i++) {
                    int dst[2];
                    calculateQueenDestination(f, r, power, dst, i);
                    if (isInBound(dst[0], dst[1]))
                        LINE[toBBIndex(f, r)][toBBIndex(dst[0], dst[1])] = line;
                }

                for (int i = 1; i <= 8; i++) {
                    int dst[2];
                    calculateQueenDestination(f, r, power, dst, i);
                    if (isInBound(dst[0], dst[1])) {
                        SEGMENT[toBBIndex(f, r)][toBBIndex(dst[0], dst[1])] = BitBoard(segment);
                        segment.set(toBBIndex(dst[0], dst[1]));
                    } else
                        break;
                }
            }

            for (int piece = 0; piece < PIECE_NUMBER; piece++) {
                for (int power : PIECE_POWER[piece])
                    if (power != STUB) {
                        int dst[2];

                        if (isKnightLike(piece))
                            calculateLionDestination(f, r, power, dst);
                        else
                            calculateQueenDestination(f, r, power, dst);
                        if (isInBound(dst[0], dst[1])) {
                            STEP_MOVES[piece][toBBIndex(f, r)].set(toBBIndex(dst[0], dst[1]));
                            STEP_MOVES_INVERT[piece][toBBIndex(dst[0], dst[1])].set(toBBIndex(f, r));
                        }
                    }
            }

            for (int df = FILE_TWO; df <= FILE_EIGHT; df++)
                for (int dr = RANK_TWO; dr <= RANK_EIGHT; dr++)
                    if (abs(f - df) == abs(r - dr) && f != df)
                        BISHOP_MASK[toBBIndex(f, r)].set(toBBIndex(df, dr));
            for (int index = 0; index < ROOK_PEXT * ROOK_PEXT; index++) {
                BitBoard situation = BitBoard::unpextBishop(index, BISHOP_MASK[toBBIndex(f, r)]);
                
                for (int power : DIAGONAL)
                    for (int i = 1; i <= 8; i++) {
                        int dst[2];
                        calculateQueenDestination(f, r, power, dst, i);
                        if (isInBound(dst[0], dst[1])) {
                            BISHOP_MOVES[toBBIndex(f, r)][index].set(toBBIndex(dst[0], dst[1]));
                            BISHOP_MOVES_INVERT[toBBIndex(dst[0], dst[1])][index] = toBBIndex(f, r);
                            if (situation.test(toBBIndex(dst[0], dst[1])))
                                break;
                        } else
                            break;
                    }
            }

            for (int index = 0; index < ROOK_PEXT; index++) {
                BitBoard fileMask = FILE_BB[f] & ~RANK_BB[RANK_ONE] & ~RANK_BB[RANK_NINE];
                BitBoard fileSituation = BitBoard::unpextRook(index, fileMask);
                for (int power : NS)
                    for (int i = 1; i <= 8; i++) {
                        int dst[2];
                        calculateQueenDestination(f, r, power, dst, i);
                        if (isInBound(dst[0], dst[1])) {
                            ROOK_FILE_MOVES[toBBIndex(f, r)][index].set(toBBIndex(dst[0], dst[1]));
                            ROOK_FILE_MOVES_INVERT[toBBIndex(dst[0], dst[1])][index].set(toBBIndex(f, r));
                            LANCE_MOVES[power == N ? 0 : 1][toBBIndex(f, r)][index].set(toBBIndex(dst[0], dst[1]));
                            LANCE_MOVES_INVERT[power == N ? 0 : 1][toBBIndex(dst[0], dst[1])][index].set(toBBIndex(f, r));
                            if (fileSituation.test(toBBIndex(dst[0], dst[1])))
                                break;
                        } else
                            break;    
                    }
                BitBoard rankMask = RANK_BB[r] & ~FILE_BB[FILE_ONE] & ~FILE_BB[FILE_NINE];
                BitBoard rankSituation = BitBoard::unpextRook(index, rankMask);
                for (int power : EW)
                    for (int i = 1; i <= 8; i++) {
                        int dst[2];
                        calculateQueenDestination(f, r, power, dst, i);
                        if (isInBound(dst[0], dst[1])) {
                            ROOK_RANK_MOVES[toBBIndex(f, r)][index].set(toBBIndex(dst[0], dst[1]));
                            ROOK_RANK_MOVES_INVERT[toBBIndex(dst[0], dst[1])][index].set(toBBIndex(f, r));
                            if (rankSituation.test(toBBIndex(dst[0], dst[1])))
                                break;
                        } else
                            break;
                    }
            }
        }
    for (int f = 0; f < FILE_NUMBER; f++)
        for (int r = 0; r < RANK_NUMBER; r++)
            if (NAIVE_SETUP[f][r] != NONE)
                SETUP[NAIVE_SETUP[f][r]].set(toBBIndex(f, r));
    for (int colour = 0; colour < COLOUR_NUMBER; colour++) {
        PROMOTE_MASK[colour] |= RANK_BB[opposingRank(colour, 0)];
        PROMOTE_MASK[colour] |= RANK_BB[opposingRank(colour, 1)];
        PROMOTE_MASK[colour] |= RANK_BB[opposingRank(colour, 2)];
        PAWN_FORCE_PROMOTION_MASK[colour] |= RANK_BB[opposingRank(colour, 0)];
        for (int f = 0; f < FILE_NUMBER; f++)
            PAWN_DROP_MASK[colour][f] |= FILE_BB[f] & ~PAWN_FORCE_PROMOTION_MASK[colour];
        LANCE_FORCE_PROMOTION_MASK[colour] |= RANK_BB[opposingRank(colour, 0)];
        LANCE_DROP_MASK[colour] = ~LANCE_FORCE_PROMOTION_MASK[colour];
        KNIGHT_FORCE_PROMOTION_MASK[colour] |= RANK_BB[opposingRank(colour, 0)];
        KNIGHT_FORCE_PROMOTION_MASK[colour] |= RANK_BB[opposingRank(colour, 0)];
        KNIGHT_DROP_MASK[colour] = ~KNIGHT_FORCE_PROMOTION_MASK[colour];
    }
}

std::vector<MoveAction> Board::getAvailableMoves(int colour) {
    std::vector<MoveAction> availables;
    BitBoard everyPieces = this->getEveryPieces();

    int start = colour == 1 ? PIECE_NUMBER / 2 : 0;
    int end = start + PIECE_NUMBER / 2;
    int opponent = !colour;
    
    int myKing = kingOf(colour);
    int myKingLoc = this->pieces[myKing].first();
    BitBoard myChecking = this->directChecks[opponent] | this->getRangeAttackers(opponent, myKingLoc);

    if (myChecking.count() > 1) {
        BitBoard evadable = getAttackingSquares(colour, myKingLoc, myKing);
        evadable.forEach([&](int dst) {
            if (!this->getAttackers(opponent, dst))
                availables.push_back(MoveAction{myKing, myKingLoc, dst, false});
        });
        return availables;
    }

    BitBoard pinned = this->getPinning(opponent, myKingLoc);


    for (int piece = start; piece < end; piece++) {
        this->pieces[piece].forEach([&](int src) {
            BitBoard attackable = this->getAttackingSquares(colour, src, piece);

            if (myChecking) {
                if (piece == myKing) {
                    BitBoard evadable = getAttackingSquares(colour, myKingLoc, myKing);
                    evadable.forEach([&](int dst) {
                        if (!this->getAttackers(opponent, dst))
                            availables.push_back(MoveAction{piece, src, dst, false});
                    });
                    return;
                } else
                    attackable &= (myChecking | SEGMENT[myChecking.first()][myKingLoc]);
            }


            if (pinned.test(src))
                attackable &= LINE[src][this->pieces[kingOf(colour)].first()];
            
            BitBoard attackablePromote = PROMOTE_MASK[colour].test(src) ? BitBoard(attackable) : attackable & PROMOTE_MASK[colour];
            
            if (isPawnLike(piece))
                attackable &= ~PAWN_FORCE_PROMOTION_MASK[colour];
            else if (isLanceLike(piece))
                attackable &= ~LANCE_FORCE_PROMOTION_MASK[colour];
            else if (isKnightLike(piece))
                attackable &= ~KNIGHT_FORCE_PROMOTION_MASK[colour];

            if (PROMOTABLE_PIECE[piece])
                attackablePromote.forEach([&](int dst) {
                    availables.push_back(MoveAction{piece, src, dst, true});
                });

            attackable.forEach([&](int dst) {
                availables.push_back(MoveAction{piece, src, dst, false});
            });
        });
    }
    
    return availables;
}

std::vector<DropAction> Board::getAvailableDrops(int colour) {
    std::vector<DropAction> availables;
    int start = colour == 1 ? DROP_NUMBER / 2 : 0;
    int end = start + DROP_NUMBER / 2;
    BitBoard notEveryPieces = ~this->getEveryPieces();

    int opponent = !colour;
    int myKing = kingOf(colour);
    int myKingLoc = this->pieces[myKing].first();
    BitBoard myChecking = this->directChecks[opponent] | this->getRangeAttackers(opponent, myKingLoc);

    if (myChecking) {
        if (myChecking.count() > 1)
            return availables;
        notEveryPieces &= myChecking & SEGMENT[myChecking.first()][myKingLoc];
    }

    int dropPawnMate = -1;
    int opponentKingLoc = pieces[kingOf(opponent)].first();
    int khead = colour == 0 ? opponentKingLoc + 1 : opponentKingLoc - 1;

    if (graveInfo[start] != 0 && this->pawnDropMask[colour].test(khead) && opponentKingLoc % RANK_NUMBER != opposingRank(opponent, 0)
        && notEveryPieces.test(khead)) {

        if (!getAttackers(colour, khead))
            goto noDropPawnMate;
        if(getAttackers(opponent, khead) & (getPinning(opponent, opponentKingLoc) | FILE_BB[khead / RANK_NUMBER]) & ~pieces[kingOf(opponent)])
            goto noDropPawnMate;
        
        BitBoard evadable = getAttackingSquares(opponent, opponentKingLoc, kingOf(opponent));
        dropPawnMate = khead;

        evadable.forEach([&](int dst) {
            if (!this->getAttackers(colour, dst)) {
                dropPawnMate = -1;
                return;
            }
        });
    }

    noDropPawnMate:
    ;

    notEveryPieces.forEach([&](int dst) {
        for (int i = start; i < end; i++) {
            if (graveInfo[i] == 0)
                continue;
            int piece = GRAVE_TO_PIECE[i];

            if (isPawnLike(piece)) {
                if (dst != dropPawnMate && this->pawnDropMask[colour].test(dst))
                    availables.push_back(DropAction{i, dst});
                continue;
            }
            if (isLanceLike(piece)) {
                if (LANCE_DROP_MASK[colour].test(dst))
                    availables.push_back(DropAction{i, dst});
                continue;
            }
            if (isKnightLike(piece)) {
                if (KNIGHT_DROP_MASK[colour].test(dst))
                    availables.push_back(DropAction{i, dst});
                continue;
            }
            availables.push_back(DropAction{i, dst});
        }
    });

    return availables;
}

int Board::inflict(int colour, MoveAction action) {
    std::cout << (std::string) action << std::endl;
    int opponent = !colour;

    int start = colour == 1 ? 0 : PIECE_NUMBER / 2;
    int end = start + PIECE_NUMBER / 2;
    int capture = NONE;

    for (int piece = start; piece < end; piece++)
        if (this->pieces[piece].test(action.dst)) {
            this->placement[!colour].unset(action.dst);
            this->pieces[piece].unset(action.dst);
            capture = piece;
            this->graveInfo[PIECE_TO_GRAVE[piece]] += 1;
            break;
        }
    
    this->pieces[action.piece].unset(action.src);
    this->placement[colour].unset(action.src);
    int resultingPiece = action.promote ? promote(action.piece) : action.piece;
    this->pieces[resultingPiece].set(action.dst);
    
    this->placement[colour].set(action.dst);

    if (this->getAttackingSquares(colour, action.dst, resultingPiece) & this->pieces[kingOf(opponent)])
        this->directChecks[colour].set(action.dst);

    if (isPawnLike(action.piece) && action.promote)
        pawnDropMask[colour] |= PAWN_DROP_MASK[colour][action.dst / RANK_NUMBER];
    if (isPawnLike(capture))
        pawnDropMask[opponent] |= PAWN_DROP_MASK[opponent][action.dst / RANK_NUMBER];

    return capture;
}

void Board::inflict(int colour, DropAction action) {
    std::cout << (std::string) action << std::endl;
    int opponent = !colour;
    this->graveInfo[action.graveIndex] -= 1;
    int resultingPiece = GRAVE_TO_PIECE[action.graveIndex];
    this->pieces[resultingPiece].set(action.dst);
    this->placement[colour].set(action.dst);

    if (this->getAttackingSquares(colour, action.dst, resultingPiece) & this->pieces[kingOf(opponent)])
        this->directChecks[colour].set(action.dst);

    if (isPawnLike(resultingPiece))
        pawnDropMask[colour] &= ~PAWN_DROP_MASK[colour][action.dst / RANK_NUMBER];
}