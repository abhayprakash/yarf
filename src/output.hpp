/**
 * Quick and dirty debug output
 */
#ifndef YARF_OUTPUT_HPP
#define YARF_OUTPUT_HPP

#include <iostream>
#include "RFtypes.hpp"


template<typename T>
void print(const T& c, uint n = 1) {
    for (uint i = 0; i < n;++i) {
        std::cout << c;
    }
}

template<typename ContainerT>
void printArray(const ContainerT& xs, bool printSize = true,
                uint p1 = 0, uint p2 = -1)
{
    using std::cout;
    using std::endl;

    //for (typename ContainerT::const_iterator it = xs.begin();
    //     it != xs.end(); ++it)
    p2 = std::min(xs.size(), p2);
    cout << "size: " << p2 - p1 << " \t";

    for (uint p = p1; p < p2; ++p)
    {
        if (p > p1)
        {
            cout << ",";
        }
        cout << xs[p];
    }
    cout << endl;
}

template<typename T>
std::ostream& operator<< (std::ostream& os, const std::vector<T>& xs) {
    os << "size: " << xs.size() << " \t";
    for (typename std::vector<T>::const_iterator it = xs.begin();
         it != xs.end(); ++it) {
        if (it != xs.begin())
        {
            os << ",";
        }
        os << *it;
    }
    os << "\n";
    return os;
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
