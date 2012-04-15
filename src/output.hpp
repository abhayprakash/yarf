/**
 * Quick and dirty debug output
 */
#ifndef YARF_OUTPUT_HPP
#define YARF_OUTPUT_HPP

#include <iostream>
#include <sstream>
#include "RFtypes.hpp"


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

template<typename ContainerT1, typename ContainerT2>
void printPermutedArray(const ContainerT1& xs,
                        typename ContainerT2::const_iterator a,
                        typename ContainerT2::const_iterator b)
{
    using std::cout;
    using std::endl;

    assert(b >= a && xs.size() >= uint(b - a));
    cout << "size: " << b - a << " \t";

    for (typename ContainerT2::const_iterator p = a; p != b; ++p)
    {
        if (p != a)
        {
            cout << ",";
        }
        cout << xs[*p];
    }
    cout << endl;
}


#endif // YARF_OUTPUT_HPP
