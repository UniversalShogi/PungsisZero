#ifndef BITBOARD_H
#define BITBOARD_H

#include "basics.h"
#include "intrinsics.h"

#include <string>
#include <iostream>

inline int toBBIndex(int file, int rank) {
    return file * RANK_NUMBER + rank;
}

class BitBoard;

extern BitBoard BB_ALL;
extern BitBoard BB_NONE;

class alignas(16) BitBoard {
    public:
    union {
        uint64_t parts[2];
        __m128i simd;
    };

    BitBoard() : BitBoard(0ULL, 0ULL) {}

    BitBoard(uint64_t part2) : BitBoard(0ULL, part2) {}
    
    BitBoard(uint64_t part1, uint64_t part2) {
        this->parts[0] = part1;
        this->parts[1] = part2;
    }

    BitBoard(__m128i simd) : simd(simd) {}

    BitBoard(const BitBoard& other) {
        _mm_store_si128(&this->simd, other.simd);
    }

    BitBoard& operator=(const BitBoard& other) {
        _mm_store_si128(&this->simd, other.simd);
        return *this;
    }

    BitBoard& operator+=(const BitBoard& other) {
        this->simd = _mm_add_epi64(this->simd, other.simd);
        return *this;
    }

    BitBoard& operator-=(const BitBoard& other) {
        this->simd = _mm_sub_epi64(this->simd, other.simd);
        return *this;
    }

    BitBoard& operator&=(const BitBoard& other) {
        this->simd = _mm_and_si128(this->simd, other.simd);
        return *this;
    }

    BitBoard& operator|=(const BitBoard& other) {
        this->simd = _mm_or_si128(this->simd, other.simd);
        return *this;
    }

    BitBoard& operator^=(const BitBoard& other) {
        this->simd = _mm_xor_si128(this->simd, other.simd);
        return *this;
    }

    BitBoard& operator<<=(int shift) {
        this->simd = _mm_slli_epi64(this->simd, shift);
        return *this;
    }

    BitBoard& operator>>=(int shift) {
        this->simd = _mm_srli_epi64(this->simd, shift);
        return *this;
    }

    BitBoard operator+(const BitBoard& other) {
        return BitBoard(*this) += other;
    }

    BitBoard operator-(const BitBoard& other) {
        return BitBoard(*this) -= other;
    }

    BitBoard operator&(const BitBoard& other) {
        return BitBoard(*this) &= other;
    }

    BitBoard operator|(const BitBoard& other) {
        return BitBoard(*this) |= other;
    }

    BitBoard operator^(const BitBoard& other) {
        return BitBoard(*this) ^= other;
    }

    BitBoard operator~() {
        return *this ^ BB_ALL;
    }

    BitBoard operator<<(int shift) {
        return BitBoard(*this) <<= shift;
    }

    BitBoard operator>>(int shift) {
        return BitBoard(*this) >>= shift;
    }

    bool operator==(const BitBoard& other) {
        __m128i result = _mm_xor_si128(this->simd, other.simd);
        return _mm_testz_si128(result, result);
    }

    bool operator!=(const BitBoard& other) {
        return !(*this == other);
    }

    template<int pos> uint64_t chunk() {
        return _mm_extract_epi64(this->simd, pos);
    }

    operator bool() {
        return !_mm_testz_si128(this->simd, this->simd);
    }

    static BitBoard one_one(int pos) {
        BitBoard bb;
        bb.set(pos);
        return bb;
    }

    int count() {
        return _mm_popcnt_u64(this->parts[0]) + _mm_popcnt_u64(this->parts[1]);
    }

    int first() {
        int index;

        if (this->parts[1] != 0) {
            index = _tzcnt_u64(this->parts[1]);
            return index;
        } else if (this->parts[0] != 0) {
            index = _tzcnt_u64(this->parts[0]);
            return index + 64;
        }

        return -1;
    }

    int pop() {
        int index;

        if (this->parts[1] != 0) {
            index = _tzcnt_u64(this->parts[1]);
            this->parts[1] = _blsr_u64(this->parts[1]);
            return index;
        } else if (this->parts[0] != 0) {
            index = _tzcnt_u64(this->parts[0]);
            this->parts[0] = _blsr_u64(this->parts[0]);
            return index + 64;
        }

        return -1;
    }

    void forEach(auto callback) {
        BitBoard copy(*this);

        int square;

        while ((square = copy.pop()) != -1)
            callback(square);
    }

    void set(int pos) {
        if (pos < 64)
            this->parts[1] |= 1ULL << pos;
        else
            this->parts[0] |= 1ULL << pos - 64;
    }

    void unset(int pos) {
        if (pos < 64)
            this->parts[1] &= ~(1ULL << pos);
        else
            this->parts[0] &= ~(1ULL << pos - 64);
    }

    void nullify() {
        this->simd = _mm_setzero_si128();
    }

    bool test(int pos) {
        if (pos < 64)
            return this->parts[1] & 1ULL << pos;
        else
            return this->parts[0] & 1ULL << pos - 64;
    }

    uint64_t crushBishop() {
        return parts[0] << 1 | parts[1];
    }

    static BitBoard unpextBishop(int index, BitBoard& mask) {
        BitBoard bb;
        BitBoard crushedMask = mask.crushBishop();
        int count = 0;
        crushedMask.forEach([&](int square) {
            if (index & 1 << count++)
                if (mask.test(square))
                    bb.set(square);
                else
                    bb.set(square + 63); 
                
        });
        return bb;
    }

    uint64_t crushRookRank() {
        return parts[1] >> 9 | parts[0] << 55;
    }

    uint64_t crushRookFile() {
        return parts[1] >> 63 | parts[0] << 1;
    }

    static BitBoard unpextRook(int index, BitBoard& mask) {
        BitBoard bb;
        int count = 0;
        mask.forEach([&](int square) {
            if (index & 1 << count++)
                bb.set(square);
        });
        return bb;
    }

    operator std::string();
};

#endif