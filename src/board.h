#ifndef BOARD_H
#define BOARD_H

#include "action.h"
#include "basics.h"
#include "bitboard.h"

#include <string>
#include <vector>

constexpr int ROOK_PEXT = 128;

extern BitBoard FILE_BB[FILE_NUMBER];
extern BitBoard RANK_BB[RANK_NUMBER];
extern BitBoard LINE[FILE_NUMBER * RANK_NUMBER][FILE_NUMBER * RANK_NUMBER];
extern BitBoard SEGMENT[FILE_NUMBER * RANK_NUMBER][FILE_NUMBER * RANK_NUMBER];
extern BitBoard SETUP[PIECE_NUMBER];
extern BitBoard PAWN_DROP_MASK[COLOUR_NUMBER][FILE_NUMBER];
extern BitBoard LANCE_DROP_MASK[COLOUR_NUMBER];
extern BitBoard KNIGHT_DROP_MASK[COLOUR_NUMBER];
extern BitBoard PAWN_FORCE_PROMOTION_MASK[COLOUR_NUMBER];
extern BitBoard LANCE_FORCE_PROMOTION_MASK[COLOUR_NUMBER];
extern BitBoard KNIGHT_FORCE_PROMOTION_MASK[COLOUR_NUMBER];
extern BitBoard STEP_MOVES[PIECE_NUMBER][FILE_NUMBER * RANK_NUMBER];
extern BitBoard ROOK_FILE_MOVES[FILE_NUMBER * RANK_NUMBER][ROOK_PEXT];
extern BitBoard ROOK_RANK_MOVES[FILE_NUMBER * RANK_NUMBER][ROOK_PEXT];
extern BitBoard LANCE_MOVES[COLOUR_NUMBER][FILE_NUMBER * RANK_NUMBER][ROOK_PEXT];
extern BitBoard BISHOP_MASK[FILE_NUMBER * RANK_NUMBER];
extern BitBoard BISHOP_MOVES[FILE_NUMBER * RANK_NUMBER][ROOK_PEXT * ROOK_PEXT];
extern BitBoard STEP_MOVES_INVERT[PIECE_NUMBER][FILE_NUMBER * RANK_NUMBER];
extern BitBoard ROOK_FILE_MOVES_INVERT[FILE_NUMBER * RANK_NUMBER][ROOK_PEXT];
extern BitBoard ROOK_RANK_MOVES_INVERT[FILE_NUMBER * RANK_NUMBER][ROOK_PEXT];
extern BitBoard LANCE_MOVES_INVERT[COLOUR_NUMBER][FILE_NUMBER * RANK_NUMBER][ROOK_PEXT];
extern BitBoard BISHOP_MOVES_INVERT[FILE_NUMBER * RANK_NUMBER][ROOK_PEXT * ROOK_PEXT];
extern BitBoard PROMOTE_MASK[COLOUR_NUMBER];
extern int MODEL_OUTPUT[FILE_NUMBER * RANK_NUMBER][FILE_NUMBER * RANK_NUMBER];

class Board {
    public:
    BitBoard pieces[PIECE_NUMBER];
    BitBoard placement[COLOUR_NUMBER];
    unsigned char graveInfo[DROP_NUMBER] = {};
    int currentColour = 0;

    // calculated
    BitBoard pawnDropMask[COLOUR_NUMBER];
    BitBoard directChecks[COLOUR_NUMBER];

    Board() {
        for (int i = 0; i < PIECE_NUMBER; i++) {
            this->pieces[i] = SETUP[i];
            this->placement[colourOf(i)] |= SETUP[i];
        }
    }

    Board(const int naiveSetup[FILE_NUMBER][RANK_NUMBER], const char setupGraveInfo[DROP_NUMBER]) {
        for (int f = 0; f < FILE_NUMBER; f++)
            for (int r = 0; r < RANK_NUMBER; r++) {
                if (naiveSetup[f][r] != NONE) {
                    this->pieces[naiveSetup[f][r]].set(toBBIndex(f, r));
                    this->placement[colourOf(naiveSetup[f][r])].set(toBBIndex(f, r));
                }
            }
        
        for (int i = 0; i < DROP_NUMBER; i++)
            this->graveInfo[i] = setupGraveInfo[i];

        this->calibrate();        
    }

    BitBoard getEveryPieces() const {
        return placement[0] | placement[1];
    }

    operator std::string() const;

    bool operator==(const Board& other) const {
        if (this->currentColour != other.currentColour)
            return false;
        for (int i = 0; i < PIECE_NUMBER; i++)
            if (this->pieces[i] != other.pieces[i])
                return false;
        for (int i = 0; i < DROP_NUMBER; i++)
            if (this->graveInfo[i] != other.graveInfo[i])
                return false;
        return true;
    }

    bool operator!=(const Board& other) const {
        return *this == other;
    }

    static void init();

    void calibrate() {
        for (int i = 0; i < COLOUR_NUMBER; i++) {
            this->directChecks[i] = getStepAttackers(i, this->pieces[kingOf(!i)].first());
            
            for (int f = 0; f < FILE_NUMBER; f++)
                if (!(FILE_BB[f] & this->pieces[pawnOf(i)]))
                    this->pawnDropMask[i] |= PAWN_DROP_MASK[i][f];
                else
                    this->pawnDropMask[i] &= ~PAWN_DROP_MASK[i][f];
        }
    }

    int getPieceAt(int src) {
        for (int p = 0; p < PIECE_NUMBER; p++)
            if (this->pieces[p].test(src))
                return p;
        return NONE;
    }

    BitBoard getPenetratingSquares(int colour, int penetratingColour, int src, int piece) const {
        BitBoard everyPieces = this->placement[!penetratingColour];
        BitBoard attackable;
        int f = src / RANK_NUMBER;
        int r = src % RANK_NUMBER;

        if (isBishopLike(piece))
            attackable |= BISHOP_MOVES[src][_pext_u64((everyPieces & BISHOP_MASK[src]).crushBishop(), BISHOP_MASK[src].crushBishop())];
        else if (isRookLike(piece)) {
            if (f >= FILE_EIGHT)
                attackable |= ROOK_FILE_MOVES[src][everyPieces.crushRookFile() >> ((f - FILE_EIGHT) * 9 + 1) & 0b1111111ULL];
            else
                attackable |= ROOK_FILE_MOVES[src][everyPieces.parts[1] >> (f * 9 + 1) & 0b1111111ULL];
            attackable |= ROOK_RANK_MOVES[src][_pext_u64(everyPieces.crushRookRank(), 0x40201008040201ULL << r)];
        } else if (isLanceLike(piece)) {
            if (f >= FILE_EIGHT)
                attackable |= LANCE_MOVES[colour][src][everyPieces.crushRookFile() >> ((f - FILE_EIGHT) * 9 + 1) & 0b1111111ULL];
            else
                attackable |= LANCE_MOVES[colour][src][everyPieces.parts[1] >> (f * 9 + 1) & 0b1111111ULL];
        }

        attackable &= ~this->placement[colour];

        return attackable;
    }

    BitBoard getAttackingSquares(int colour, int src, int piece) const {
        BitBoard everyPieces = this->getEveryPieces();
        BitBoard attackable = STEP_MOVES[piece][src];
        int f = src / RANK_NUMBER;
        int r = src % RANK_NUMBER;

        if (isBishopLike(piece))
            attackable |= BISHOP_MOVES[src][_pext_u64((everyPieces & BISHOP_MASK[src]).crushBishop(), BISHOP_MASK[src].crushBishop())];
        else if (isRookLike(piece)) {
            if (f >= FILE_EIGHT)
                attackable |= ROOK_FILE_MOVES[src][everyPieces.crushRookFile() >> ((f - FILE_EIGHT) * 9 + 1) & 0b1111111ULL];
            else
                attackable |= ROOK_FILE_MOVES[src][everyPieces.parts[1] >> (f * 9 + 1) & 0b1111111ULL];
            attackable |= ROOK_RANK_MOVES[src][_pext_u64(everyPieces.crushRookRank(), 0x40201008040201ULL << r)];
        } else if (isLanceLike(piece)) {
            if (f >= FILE_EIGHT)
                attackable |= LANCE_MOVES[colour][src][everyPieces.crushRookFile() >> ((f - FILE_EIGHT) * 9 + 1) & 0b1111111ULL];
            else
                attackable |= LANCE_MOVES[colour][src][everyPieces.parts[1] >> (f * 9 + 1) & 0b1111111ULL];
        }

        attackable &= ~this->placement[colour];

        return attackable;
    }

    BitBoard getStepAttackers(int colour, int dst) const {
        int start = colour == 0 ? 0 : PIECE_NUMBER / 2;
        int end = start + PIECE_NUMBER / 2;
        BitBoard everyPieces = this->getEveryPieces();
        BitBoard attacking;

        for (int piece = start; piece < end; piece++)
            attacking |= this->pieces[piece] & STEP_MOVES_INVERT[piece][dst];
        return attacking;        
    }

    BitBoard getRangeAttackers(int colour, int dst) const {
        BitBoard everyPieces = this->getEveryPieces();
        BitBoard bb;
        int f = dst / RANK_NUMBER;
        int r = dst % RANK_NUMBER;
        int opponent = !colour;
        bb |= getAttackingSquares(opponent, dst, bishopOf(opponent)) & (this->pieces[bishopOf(colour)] | this->pieces[horseOf(colour)]);
        bb |= getAttackingSquares(opponent, dst, rookOf(opponent)) & (this->pieces[rookOf(colour)] | this->pieces[dragonOf(colour)]);
        bb |= getAttackingSquares(opponent, dst, lanceOf(opponent)) & this->pieces[lanceOf(colour)];
        return bb;
    }

    BitBoard getAttackers(int colour, int dst) const {
        return getStepAttackers(colour, dst) | getRangeAttackers(colour, dst);
    }

    BitBoard getPinning(int colour, int dst) const {
        BitBoard everyPieces = this->getEveryPieces();
        BitBoard bb;
        BitBoard pinning;
        int f = dst / RANK_NUMBER;
        int r = dst % RANK_NUMBER;
        int opponent = !colour;
        bb |= getPenetratingSquares(opponent, opponent, dst, bishopOf(opponent)) & (this->pieces[bishopOf(colour)] | this->pieces[horseOf(colour)]);
        bb |= getPenetratingSquares(opponent, opponent, dst, rookOf(opponent)) & (this->pieces[rookOf(colour)] | this->pieces[dragonOf(colour)]);
        bb |= getPenetratingSquares(opponent, opponent, dst, lanceOf(opponent)) & this->pieces[lanceOf(colour)];

        bb.forEach([&] (int src) {
            BitBoard between = SEGMENT[src][dst] & this->placement[opponent];
            int pin = between.pop();
            if (!between)
                pinning.set(pin);
        });
        return pinning;
    }

    void changeTurn() {
        this->currentColour = !this->currentColour;
    }

    void toInput(float binput[PIECE_NUMBER][FILE_NUMBER][RANK_NUMBER], float ninput[DROP_NUMBER]) const {
        for (int p = 0; p < PIECE_NUMBER; p++)
            this->pieces[p].toInput(binput[p]);
        for (int d = 0; d < DROP_NUMBER; d++)
            ninput[d] = this->graveInfo[d];
    }

    static void toInputs(int n, float (*binput)[FILE_NUMBER][RANK_NUMBER]
        , float* ninput, Board* states) {
        for (int i = 0; i < n; i++)
            states[n - i - 1].toInput(&binput[PIECE_NUMBER * i], &ninput[DROP_NUMBER * i]);
        states[n - 1].toFeatureInput(&binput[PIECE_NUMBER * n], &ninput[DROP_NUMBER * n]);
    }

    static void toInputs(int n, float (*binput)[FILE_NUMBER][RANK_NUMBER]
        , float* ninput, std::vector<Board> states) {
        for (int i = 0; i < n; i++)
            states[n - i - 1].toInput(&binput[PIECE_NUMBER * i], &ninput[DROP_NUMBER * i]);
        states[n - 1].toFeatureInput(&binput[PIECE_NUMBER * n], &ninput[DROP_NUMBER * n]);
    }


    void toFeatureInput(float binput[COLOUR_NUMBER * 3 + 1][FILE_NUMBER][RANK_NUMBER], float* ninput) {
        int ctr = 0;

        for (int c = 0; c < COLOUR_NUMBER; c++) {
            this->placement[c].toInput(binput[ctr++]);
            this->pawnDropMask[c].toInput(binput[ctr++]);
            this->getPinning(!c, this->pieces[kingOf(c)].first()).toInput(binput[ctr++]);
        }

        this->getAttackers(!this->currentColour, this->pieces[kingOf(this->currentColour)].first()).toInput(binput[ctr++]);
        *ninput = this->currentColour;
    }

    std::vector<MoveAction> getNKMoves(int colour);
    std::vector<MoveAction> getKMoves(int colour);
    std::vector<DropAction> getNKDrops(int colour);
    std::vector<DropAction> getKDrops(int colour);
    std::vector<Action> getNKActions(int colour);
    std::vector<Action> getKActions(int colour);

    int inflict(int colour, MoveAction move);
    void inflict(int colour, DropAction drop);
    void inflict(int colour, Action action);
};

extern Board BEMPTY;

#endif