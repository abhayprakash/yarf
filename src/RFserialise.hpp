/**
 * Serialisation methods for the random forest
 */
#ifndef YARF_RFSERIALISE_HPP
#define YARF_RFSERIALISE_HPP

#include <sstream>
#include <iomanip>
#include <limits>

#include "RFtypes.hpp"
#include "DataIO.hpp"



inline std::string in(uint n) {
    return std::string(2 * n, ' ');
}

inline std::string indent(uint n, char c = ' ') {
    return std::string(n, c);
}


/**
 * Output the argument x with maximum precision
 */
template <typename T>
std::string strprecise(const T& x)
{
    std::ostringstream os;

    // The latest version of C++ should support the hexfloat modifier to
    // enable a floating point to be stored as a string which can be
    // recovered exactly
    // (Note check whether the precision needs to be explicitly set)
    // http://stackoverflow.com/a/4740044
    // http://en.cppreference.com/w/cpp/io/manip/fixed
    //os << std::hexfloat;
    // Alternatively specifying fixed and scientific formatting should give
    // hexfloat output:
    //os.setf(ios_base::fixed | ios_base::scientific, ios_base::floatfield);
    // However neither seem to work with g++ 4.6.1

    // Yet another alternative is to use the C99 printf %a specifier
    //std::printf("%a", x);
    // Or this *should* output enough decimal places to recover the
    // original, though may be dependent on the conversion routines
    os << std::scientific
       << std::setprecision(std::numeric_limits<T>::digits10 + 1)
       << x;

    return os.str();
}

template<typename ContainerT>
std::string arrayToString(const ContainerT& xs, bool printSize = true,
                          size_t p1 = 0, size_t p2 = -1)
{
    std::ostringstream oss;

    p2 = std::min<size_t>(xs.size(), p2);
    if (printSize) {
        oss << "[" << p2 - p1 << "] ";
    }

    for (size_t p = p1; p < p2; ++p)
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
