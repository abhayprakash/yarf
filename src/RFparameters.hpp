/**
 * Parameters for the random forest 
 */
#ifndef YARF_RFPARAMETERS_HPP
#define YARF_RFPARAMETERS_HPP

#include "RFtypes.hpp"

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
};


#endif // YARF_RFPARAMETERS_HPP
