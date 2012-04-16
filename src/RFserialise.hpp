/**
 * Serialisation methods for the random forest
 */
#ifndef YARF_RFSERIALISE_HPP
#define YARF_RFSERIALISE_HPP

#include <sstream>
#include "RFtypes.hpp"
#include "DataIO.hpp"



inline std::string in(uint n) {
    return std::string(2 * n, ' ');
}

inline std::string indent(uint n, char c = ' ') {
    return std::string(n, c);
}

template<typename ContainerT>
std::string arrayToString(const ContainerT& xs, bool printSize = true,
                          uint p1 = 0, uint p2 = -1)
{
    std::ostringstream oss;

    p2 = std::min(xs.size(), p2);
    if (printSize) {
        oss << "[" << p2 - p1 << "] ";
    }

    for (uint p = p1; p < p2; ++p)
    {
        if (p > p1)
        {
            oss << ",";
        }
        oss << xs[p];
    }

    return oss.str();
}



#endif // YARF_RFSERIALISE_HPP
