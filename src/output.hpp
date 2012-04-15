/**
 * Quick and dirty debug output
 */
#ifndef YARF_OUTPUT_HPP
#define YARF_OUTPUT_HPP

#include <iostream>
#include <sstream>
#include "RFtypes.hpp"



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
