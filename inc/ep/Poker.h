#pragma once

#include "PokerTypes.h"
#include "ep/core/metaLog.h"

namespace ep {

constexpr int positiveIndex1Better(int index1, int index2) {
    return index2 - index1;
}

int bestBit(uint64_t val) {
    return __builtin_ctzll(val);
}

template<int Size, typename T>
struct Counted {
    Counted() = default;
    constexpr Counted(core::SWAR<Size, T> bits):
        m_counts(
            core::popcount<core::metaLogCeiling(Size - 1) - 1>(bits.value())
        )
    {}

    constexpr core::SWAR<Size, T> counts() { return m_counts; }

    template<int N>
    constexpr Counted greaterEqual() {
        return core::greaterEqualSWAR<N>(m_counts);
    }
    constexpr operator bool() { return m_counts.value(); }

    // \note Best is smaller number
    constexpr int bestIndex() {
        return bestBit(m_counts.value()) / Size;
    }

    constexpr Counted clearAt(int index) { return m_counts.clear(index); }

    protected:
    core::SWAR<Size, T> m_counts;
};

template<int Size, typename T>
constexpr Counted<Size, T>
makeCounted(core::SWAR<Size, T> bits) { return bits; }

struct CSet {
    SWARSuit m_bySuit;
    SWARRank m_byNumber;

    constexpr CSet operator|(CSet o) {
        return { m_bySuit | o.m_bySuit, m_byNumber | m_byNumber };
    }

    constexpr CSet operator&(CSet o) {
        return { m_bySuit & o.m_bySuit, m_byNumber & m_byNumber };
    }

    constexpr CSet operator^(CSet o) {
        return { m_bySuit ^ o.m_bySuit, m_byNumber ^ m_byNumber };
    }

    constexpr auto suitCounts() { return makeCounted(m_bySuit); }
    constexpr auto numberCounts() { return makeCounted(m_byNumber); }

    constexpr static unsigned numberSet(uint64_t orig) {
        auto rv = orig;
        for(auto ndx = NSuits; --ndx;) {
            orig = orig >> SuitBits;
            rv |= orig;
        }
        constexpr auto isolateMask = (uint64_t(1) << NRanks) - 1;
        return rv & isolateMask;
    }

    constexpr unsigned numberSet() { return numberSet(m_bySuit.value()); }

    constexpr CSet include(int rank, int suit) {
        return { m_bySuit.set(suit, rank), m_byNumber.set(rank, suit) };
    }
};

constexpr Counted<SuitBits, uint64_t> flushes(Counted<SuitBits, uint64_t>  ss) {
    return ss.greaterEqual<5>();
}

#define RARE(v) if(__builtin_expect(bool(v), false))

inline unsigned straights(unsigned rv) {
    // xoptx is it better hasAce = (1 & rv) << (NRanks - 4) ?
    // xoptx what about rv |= ... ?
    auto hasAce = (1 & rv) ? (1 << (NRanks - 4)) : 0;
    // 2 1 0 9 8 7 6 5 4 3 2 1 0 bit index
    // =========================
    // 2 3 4 5 6 7 8 9 T J Q K A
    // - 2 3 4 5 6 7 8 9 T J Q K &
    // - - 2 3 4 6 5 6 7 8 9 T J &
    // - - - 2 3 4 5 6 7 8 9 T J &
    // - - - A - - - - - - - - - |
    // - - - A 2 3 4 5 6 7 8 9 T &
    
    
    rv &= (rv >> 1);  // two
    rv &= (rv >> 1); // three in sequence
    rv &= (rv >> 1); // four in sequence
    rv |= hasAce;
    rv &= (rv >> 1);
    return rv;
}

struct ComparisonResult {
    int ifDecided;
    bool decided;
};

inline int bestKicker(
    Counted<NSuits, uint64_t> p1s, Counted<NSuits, uint64_t> p2s
) {
    return positiveIndex1Better(p1s.bestIndex(), p2s.bestIndex());
}

inline int bestFourOfAKind(
    Counted<NSuits, uint64_t> p1s, Counted<NSuits, uint64_t> p2s,
    Counted<NSuits, uint64_t> p1foaks, Counted<NSuits, uint64_t> p2foaks
) {
    auto p1BestNdx = p1foaks.bestIndex();
    auto p2BestNdx = p2foaks.bestIndex();
    auto diff = positiveIndex1Better(p1BestNdx, p2BestNdx);
    if(diff) { return diff; }
    return bestKicker(p1s.clearAt(p1BestNdx), p2s.clearAt(p2BestNdx));
}

inline int bestFlush(unsigned p1, unsigned p2) {
    for(auto count = 5; count--; ) {
        auto
            bestP1 = bestBit(p1),
            bestP2 = bestBit(p2);
        auto diff = positiveIndex1Better(bestP1, bestP2);
        if(diff) { return diff; }
    }
    return 0;
}

inline int bestStraight(unsigned s1, unsigned s2) {
    return positiveIndex1Better(bestBit(s1), bestBit(s2));
}

struct FullHouseResult {
    bool isFullHouse;
    int bestThreeOfAKind, bestPair;
};

inline FullHouseResult isFullHouse(Counted<NSuits, uint64_t> counts) {
    auto toaks = counts.greaterEqual<3>();
    RARE(toaks) {
        auto bestTOAK = toaks.bestIndex();
        auto withoutBestTOAK = counts.clearAt(bestTOAK);
        auto fullPairs = withoutBestTOAK.greaterEqual<2>();
        RARE(fullPairs) { return { true, bestTOAK, fullPairs.bestIndex() }; }
    }
    return { false, 0, 0 };
}

template<int N>
inline int bestKickers(
    Counted<NSuits, uint64_t> p1s, Counted<NSuits, uint64_t> p2s
) {
    for(auto counter = N;;) {
        auto p1Best = p1s.bestIndex();
        auto rv = positiveIndex1Better(p1Best, p2s.bestIndex());
        if(rv) { return rv; }
        if(!--counter) { break; }
        // xoptx
        p1s = p1s.clearAt(p1Best);
        p2s = p2s.clearAt(p1Best);
    }
    return 0;
}

}
