/**
 * Splitting rules and feature test/scoring rules
 */
#ifndef YARF_RFSPLIT_HPP
#define YARF_RFSPLIT_HPP

#include <cmath>
#include <cassert>
#include <set>

#include "RFtypes.hpp"
#include "RFparameters.hpp"
#include "Dataset.hpp"


/**
 * Interface for a class providing feature split selection
 */
class SplitSelector
{
public:
    typedef RefCountPtr<SplitSelector> Ptr;
    typedef RefCountPtr<const SplitSelector> CPtr;

    /**
     * Find a split
     * fts: Array of feature values
     * ls: Array of target labels
     * ids: reference ids of the samples in fts and ls
     * counts: Array of counts (should sum to ls.size()), alternatively if
     *         empty then this will be automatically filled from ls
     */
    SplitSelector(const FtvalArray& fts, const LabelArray& ls,
                  const IdArray& ids, const DoubleArray& counts) {
    }

    virtual ~SplitSelector() { }

    /**
     * Return the best score
     */
    virtual double getScore() const = 0;

    /**
     * Returns true if a split should be performed, false if no split
     */
    virtual bool splitRequired() const = 0;

    /**
     * Split the sample ids at this node into two parts
     */
    virtual void splitSamples(IdArray& left, IdArray& right) const = 0;

    /**
     * Get the size of the two nodes after the split
     */
    //virtual void getSplitSizes(double& left, double& right) const = 0;

    /**
     * Decide whether a sample should go left or right, false:left true:right
     */
    virtual bool predict(const DataSample& d) const = 0;

    /**
     * Count the number of each class label
     * counts: Array to hold the counts of class labels
     * ls: Array of class labels
     * ncls: Number of classes
     */
    static void countLabels(DoubleArray& counts, const LabelArray& ls,
                            uint ncls) {
        counts.clear();
        counts.resize(ncls);
        for (LabelArray::const_iterator l = ls.begin(); l != ls.end(); ++l) {
            ++counts[*l];
        }
    }

    /**
     * Checks if the class counts are pure (only one class is present)
     * Return false if counts is all zero, or more than one element of counts
     * is non-zero
     */
    static bool isPure(const DoubleArray& counts) {
        uint n = 0;
        for (DoubleArray::const_iterator it = counts.begin();
             it != counts.end(); ++it) {
            n += *it > 0;
        }
        return n == 1;
    }

protected:
    SplitSelector() { }
};


/**
 * Test a single specified feature to find a binary split in values which leads
 * to the maximum information gain.
 * The split value is taken as the average of the feature values on either side
 * of the split.
 */
class MaxInfoGainSingleSplit
{
public:
    typedef RefCountPtr<MaxInfoGainSingleSplit> Ptr;

    /**
     * Minimum difference between floats
     */
    static const double EPSILON = 1e-15;

    /**
     * Find the split which leads to the maximum information gain
     * fts: Array of feature values
     * ftid: Feature id
     * ls: Array of target labels
     * ids: reference ids of the samples in fts and ls
     * counts: Array of counts (should sum to ls.size())
     */
    MaxInfoGainSingleSplit(const FtvalArray& fts, uint ftid,
                           const LabelArray& ls, const IdArray& ids,
                           const DoubleArray& counts):
        m_ids(ids), m_ftid(ftid), m_perm(ids.size()), m_counts(counts),
        m_ig(ids.size()), m_splitpos(0), m_splitval(NAN) {
        assert(ids.size() > 0);
        assert(fts.size() == ls.size() && fts.size() == ids.size());

        sortperm(fts);
        infogain(fts, ls);
    }

    /**
     * Get the class frequencies (unnormalised) at this node
     */
    const DoubleArray& getClassFreqs() const {
        return m_counts;
    }

    /**
     * Return the maximum information gain
     */
    double getInfoGain() const {
        return m_ig[m_splitpos];
    }

    /**
     * Return the value of the feature split
     */
    Ftval getSplitValue() const {
        return m_splitval;
    }

    /**
     * Split the sample ids at this node into two parts
     */
    void splitSamples(IdArray& left, IdArray& right) const {
        left.resize(m_splitpos);
        for (uint i = 0; i < m_splitpos; ++i)
        {
            left[i] = m_ids[m_perm[i]];
        }

        right.resize(m_perm.size() - m_splitpos);
        for (uint i = m_splitpos; i < m_perm.size(); ++i)
        {
            right[i - m_splitpos] = m_ids[m_perm[i]];
        }
    }

    /**
     * Get the feature id used for splitting
     */
    uint getFeatureId() const {
        return m_ftid;
    }

    // The next four functions are for debugging

    /**
     * Get the array of infomation gain values for all valid splits
     */
    const DoubleArray& getInfoGainArray() const {
        return m_ig;
    }

    /**
     * Permutation of samples in the two children in the form of iterators:
     * A[permLeft()..permMiddle()-1], and A[permMiddle()..permRight()-1]
     */
    UintArray::const_iterator permLeft() const {
        return m_perm.begin();
    }

    /**
     * Permutation of samples in the two children in the form of iterators:
     * A[permLeft()..permMiddle()-1], and A[permMiddle()..permRight()-1]
     */
    UintArray::const_iterator permMiddle() const {
        return m_perm.begin() + m_splitpos;
    }

    /**
     * Permutation of samples in the two children in the form of iterators:
     * A[permLeft()..permMiddle()-1], and A[permMiddle()..permRight()-1]
     */
    UintArray::const_iterator permRight() const {
        return m_perm.end();
    }

protected:
    /**
     * Find the permutation of indices which will sort the feature values
     * fts: Array of feature values
     */
    void sortperm(const FtvalArray& fts) {
        for(uint i = 0; i < m_perm.size(); ++i) {
            m_perm[i] = i;
        }

        qsort(fts, 0, m_perm.size());
    }

    /**
     * Calculate the information gain for all possible valid splits
     * fts: Array of feature values
     * ls: Array of target labels
     */
    void infogain(const FtvalArray& fts, const LabelArray& ls) {
        // IG(T,a) = h(T) - h(T|a) where a is the split (binary in this case)
        // ha(i): Entropy when A is split into A[0..i-1],A[i..n]

        // Iteratively move one sample from the right partition to the left,
        // and update the counts, calculate the entropy for the split, and IG.

        uint n = m_ids.size();

        double ht = entropy(m_counts, n);

        DoubleArray countsleft(m_counts.size());
        DoubleArray countsright(m_counts);
        DoubleArray hta(m_perm.size());
        hta[0] = ht;
        m_ig[0] = 0;

        for (uint i = 1; i < n; ++i) {
            Label shiftl = ls[m_perm[i - 1]];

            ++countsleft[shiftl];
            --countsright[shiftl];

            hta[i] = (i * entropy(countsleft, i) + (n - i) *
                      entropy(countsright, n - i)) / n;

            // In practice we can only split if feature values differ, otherwise
            // set to 0
            if (fequals(fts[m_perm[i - 1]], fts[m_perm[i]])) {
                m_ig[i] = 0;
            }
            else {
                m_ig[i] = ht - hta[i];
                if (m_ig[i] > m_ig[m_splitpos]) {
                    m_splitpos = i;
                    m_splitval = (fts[m_perm[i - 1]] + fts[m_perm[i]]) / 2;
                }
            }
        }
    }

    /**
     * Calculate the entropy from a set of label counts
     * counts: Frequency of each class label
     * total: Total number of samples
     */
    double entropy(const DoubleArray& counts, uint total) {
        static const double LOG2 = log(2);

        double h = 0;
        for (uint i = 0; i < counts.size(); ++i) {
            double p = counts[i] / total;
            // 0 * log 0 = 0
            h -= (counts[i] == 0)? 0 : p * std::log(p) / LOG2;
        }

        return h;
    }

    /**
     * Finds the permutation of indices which would sort the features using
     * quick sort
     */
    void qsort(const FtvalArray& fts, uint s, uint t) {
        // Sort A[s..t-1]
        if (t - s > 1) {
            uint q = qpart(fts, s, t);
            
            qsort(fts, s, q);
            qsort(fts, q, t);
        }
    }

    /**
     * Quick sort (in-place) partition, randomly selected pivot
     */
    uint qpart(const FtvalArray& fts, uint s, uint t) {
        // Partition A[s..t-1]
        // A[s..i] <= A[t-1] <= A[j..t-2]
        uint r = Utils::randint(s, t);
        qswap(m_perm[r], m_perm[t - 1]);

        Ftval p = fts[m_perm[t - 1]];
        uint i = s - 1;

        for (uint j = s; j < t - 1; ++j) {
            if (fts[m_perm[j]] <= p) {
                ++i;
                qswap(m_perm[j], m_perm[i]);
            }
        }

        ++i;
        qswap(m_perm[i], m_perm[t - 1]);
        return i;
    }

    /**
     * Swap two values
     */
    void qswap(uint& x, uint& y) const {
        uint t = x;
        x = y;
        y = t;
    }

    /**
     * Check if two floats are equal
     */
    bool fequals(double x, double y) const {
        return std::fabs(x - y) < EPSILON;
    }

    /**
     * Input sample ids
     */
    UintArray m_ids;

    /**
     * Feature identifier
     */
    uint m_ftid;

    /**
     * Permutation indices
     */
    UintArray m_perm;

    /**
     * Total numbers of each class label
     */
    DoubleArray m_counts;

    /**
     * Information gain for each split of sorted array
     */
    DoubleArray m_ig;

    /**
     * Index of the first element after the split with maximum infomation gain
     */
    uint m_splitpos;

    /**
     * Value of the feature split
     */
    Ftval m_splitval;
};


/**
 * Test a multiple randomly selected features to find a binary split in values
 * which leads to the maximum information gain.
 */
class MaxInfoGainSplit: public SplitSelector
{
public:
    /**
     * Minimum difference between floats
     */
    static const double EPSILON = 1e-15;

    /**
     * Find the split which leads to the maximum information gain.
     * If class counts are pure returns without testing any features.
     * fts: Array of feature values
     * data: Dataset
     * ls: Array of target labels
     * ids: reference ids of the samples in fts and ls
     * counts: Array of counts (should sum to ls.size())
     */
    MaxInfoGainSplit(const RFparameters& params, const Dataset& data,
                     const LabelArray& ls, const IdArray& ids,
                     const DoubleArray& counts):
        m_counts(counts), m_gotSplit(false), m_bestft(-1) {
        assert(ids.size() > 0);
        assert(ls.size() == ids.size());
        assert(params.numSplitFeatures <= data.numFeatures());

        if (m_counts.empty()) {
            countLabels(m_counts, ls, data.numClasses());
        }
        assert(m_counts.size() == data.numClasses());

        if (!SplitSelector::isPure(m_counts)) {
            testFeatures(params, data, ls, ids);
        }
    }

    virtual double getScore() const {
        assert(m_bestft >= 0);
        return m_splits[m_bestft]->getInfoGain();
    }

    virtual bool splitRequired() const {
        return m_gotSplit;
    }

    virtual void splitSamples(IdArray& left, IdArray& right) const {
        assert(splitRequired());
        m_splits[m_bestft]->splitSamples(left, right);
    }

    virtual bool predict(const DataSample& d) const {
        assert(splitRequired());
        bool goRight = d[m_splits[m_bestft]->getFeatureId()] >=
            m_splits[m_bestft]->getSplitValue();
        return goRight;
    }

    MaxInfoGainSingleSplit::Ptr getSplit() const {
        assert(m_bestft >= 0);
        return m_splits[m_bestft];
    }

protected:
    /**
     * Test multiple random features
     */
    void testFeatures(const RFparameters& params, const Dataset& data,
                      const LabelArray& ls, const IdArray& ids) {
        m_splits.reserve(params.numSplitFeatures);

        double bestig = 0;
        std::set<uint> selected;

        for (uint i = 0; i < params.numSplitFeatures; ++i) {
            // Only test each feature once
            uint r;
            do {
                r = Utils::randint(0, data.numFeatures());
            }
            while (selected.find(r) != selected.end());
            selected.insert(r);

            FtvalArray fts;
            data.getFeature(r)->select(fts, ids);

            MaxInfoGainSingleSplit::Ptr s = new MaxInfoGainSingleSplit(
                fts, r, ls, ids, m_counts);
            m_splits.push_back(s);

            double ig = s->getInfoGain();
            if (ig > bestig) {
                bestig = ig;
                m_bestft = i;
            }

            // TODO: depth indentation
            //print("  ", m_depth);
            //std::cout << "Tested feature: " << r << " IG: " << ig <<
            //    " split-val: " << s->getSplitValue() << std::endl;
        }

        m_gotSplit = bestig > params.minScore;
    }

private:
    /**
     * Total numbers of each class label
     */
    DoubleArray m_counts;

    /**
     * Whether a suitable split was found or not
     */
    bool m_gotSplit;

    /**
     * The best tested feature
     */
    int m_bestft;

    /**
     * Array of tested splits
     */
    std::vector<MaxInfoGainSingleSplit::Ptr> m_splits;
};


#endif // YARF_RFSPLIT_HPP
