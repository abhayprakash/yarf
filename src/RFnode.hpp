/**
 * A random forest node
 */
#ifndef YARF_RFNODE_HPP
#define YARF_RFNODE_HPP

#include "RFtypes.hpp"
#include "RFparameters.hpp"
#include "RFsplit.hpp"
#include "RFutils.hpp"
#include "RFserialise.hpp"
#include "Logger.hpp"



/**
 * A node in the RF tree
 */
class RFnode
{
public:
    typedef RefCountPtr<RFnode> Ptr;

    /**
     * Create a node
     * params: Parameters for the RF algorithm
     * data: Dataset
     * ids: Array of sample ids to use
     * depth: Current node depth
     */
    RFnode(const RFparameters& params, const Dataset& data, const IdArray& ids,
           uint depth = 0):
        m_n(ids.size()), m_depth(depth) {

        LOG(Log::DEBUG2) << indent(m_depth * 2)
                         << "ids: " << arrayToString(ids);

        LabelArray ls;
        data.selectLabels(ls, ids);
        SplitSelector::countLabels(m_counts, ls, data.numClasses());

        // TODO: Should call a factory method instead of hardcoding
        // the SplitSelector implementation
        m_split = new MaxInfoGainSplit(params, data, ls, ids, m_counts);

        DoubleArray dist;
        getClassDistribution(dist, false);

        LOG(Log::DEBUG2) << indent(m_depth * 2)
                         << "counts: " << arrayToString(dist, false);

        if (m_split->splitRequired()) {
            splitNode(params, data);
        }
    }

    ~RFnode() { };

    /**
     * Get the class frequencies
     * dist: Array to hold the class frequencies
     * norm: If true normalise by the total number of samples to obtain
     *       relative frequencies
     */
    void getClassDistribution(DoubleArray& dist, bool norm) const {
        dist = m_counts;
        if (norm) {
            Utils::normalise<DoubleArray>(dist.begin(), dist.end());
        }
    }

    /**
     * Get the split handler
     */
    SplitSelector::CPtr getSplit() const {
        return m_split;
    }

    /**
     * Predict the class for a test sample
     * dist: Array to hold the normalised class predictions
     * d: Data sample to be predicted
     */
    void predict(DoubleArray& dist, const DataSample& d) const {
        if (!isleaf()) {
            bool goRight = m_split->predict(d);
            if (goRight) {
                m_right->predict(dist, d);
            }
            else {
                m_left->predict(dist, d);
            }
        }
        else {
            getClassDistribution(dist, true);
        }
    }

    /**
     * Is this a leaf (terminal) node?
     */
    bool isleaf() const {
        return !m_left && !m_right;
    }

    /**
     * Get the left child
     */
    RFnode::Ptr left() const {
        return m_left;
    }

    /**
     * Get the right child
     */
    RFnode::Ptr right() const {
        return m_right;
    }

    /**
     * Save this object
     */
    void serialise(std::ostream& os, uint level, uint i) const {
        os << in(i) << "RFnode{\n"
           << in(i) << "counts " << arrayToString(m_counts) << "\n"
           << in(i) << "n " << m_n << "\n"
           << in(i) << "depth " << m_depth << "\n"
           << in(i) << "split\n";
        m_split->serialise(os, level, i + 1);
        if (!isleaf()) {
            os << in(i) << "Left\n";
            m_left->serialise(os, level, i + 1);
            os << in(i) << "Right\n";
            m_right->serialise(os, level, i + 1);
        }
        os << in(i) << "}RFnode\n";
    }

protected:

    /**
     * Create two children by splitting the samples at this node using the
     * best split
     */
    void splitNode(const RFparameters& params, const Dataset& data) {
        assert(m_split->splitRequired());

        IdArray left, right;
        m_split->splitSamples(left, right);

        LOG(Log::DEBUG2) << indent(m_depth * 2) << "Left";
        m_left = new RFnode(params, data, left, m_depth + 1);

        LOG(Log::DEBUG2) << indent(m_depth * 2) << "Right";
        m_right = new RFnode(params, data, right, m_depth + 1);
    }

protected:
    /**
     * Left child
     */
    RFnode::Ptr m_left;

    /**
     * Right child
     */
    RFnode::Ptr m_right;

    /**
     * Number of each class
     */
    DoubleArray m_counts;

    /**
     * Number of samples at this node
     */
    uint m_n;

    /**
     * Feature split handler
     */
    SplitSelector::CPtr m_split;

    /**
     * Tree depth of this node
     */
    uint m_depth;

private:
    /**
     * Default constructor for deserialisation only
     */
    RFnode() {
    }
    friend class RFbuilder;
};


#endif // YARF_RFNODE_HPP
