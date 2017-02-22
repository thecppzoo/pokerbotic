#pragma once

#include "PokerTypes.h"

#include <istream>

namespace ep {

auto g_rankLetters = "AKQJT98765432!@#$%^&*()";
    
std::ostream &pRanks(std::ostream &out, unsigned ranks) {
    for(auto ndx = 32; ndx--; ) {
        if(ranks & (1 << ndx)) { out << g_rankLetters[ndx]; }
        else { out << '-'; }
    }
    return out;
}

}