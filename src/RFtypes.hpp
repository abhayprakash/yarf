/**
 * Basic type definitions for the random forest feature values and class labels
 */
#ifndef YARF_RFTYPES_HPP
#define YARF_RFTYPES_HPP

#include <string>
#include <vector>
#include "refcountptr.hpp"

typedef unsigned int uint;
typedef double Ftval;
typedef unsigned short Label;
typedef uint Id;

typedef std::vector<Ftval> FtvalArray;
typedef std::vector<Label> LabelArray;
typedef std::vector<uint> UintArray;
typedef std::vector<double> DoubleArray;
typedef std::vector<Id> IdArray;

typedef std::vector<std::string> StringArray;

#endif // YARF_RFTYPES_HPP
