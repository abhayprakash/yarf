/**
 * Parameters for the random forest 
 */
#ifndef YARF_RFPARAMETERS_HPP
#define YARF_RFPARAMETERS_HPP

#include "RFtypes.hpp"
#include "RFserialise.hpp"

/**
 * Parameters for the random forest
 */
struct RFparameters
{
    /**
     * Number of trees in the forest
     */
    uint numTrees;

    /**
     * Number of features to randomly choose at each split
     */
    uint numSplitFeatures;

    /**
     * Minimum score before splitting
     */
    double minScore;

    void serialise(std::ostream& os, uint level, uint i) const {
        os << in(i) << "RFparameters{\n"
           << in(i) << "numTrees " << numTrees << "\n"
           << in(i) << "numSplitFeatures " << numSplitFeatures << "\n"
           << in(i) << "minScore " << minScore << "\n"
           << in(i) << "}RFparameters\n";
    }
};


#endif // YARF_RFPARAMETERS_HPP
