/**
 * Classes to handle the passing of datasets to the random forest
 */
#ifndef YARF_DATASET_HPP
#define YARF_DATASET_HPP

#include <cassert>
#include <algorithm>
#include "RFtypes.hpp"
#include "RFutils.hpp"

/**
 * Interface to a feature
 */
class FeatureSet
{
public:
    FeatureSet() {};

    virtual ~FeatureSet() {};

    /**
     * Return the feature value for a specified sample
     * id: The sample id
     */
    virtual Ftval operator[](Id id) const = 0;

    /**
     * Return the values of a feature for a subset of samples
     * fts: The output array to hold the feature values
     * ids: Array of sample ids
     */
    virtual void select(FtvalArray& fts, const IdArray& ids) const = 0;

    /**
     * Return the number of samples
     */
    virtual uint size() const = 0;
};


/**
 * Interface to a sample
 */
class DataSample
{
public:
    DataSample() {};

    virtual ~DataSample() {};

    /**
     * Return the value of a feature
     * ftid: The feature id
     */
    virtual Ftval operator[](uint ftid) const = 0;

    /**
     * Return the sample id
     */
    virtual Id id() const = 0;

    /**
     * Return the sample label, or Dataset::NoLabel if unknown
     */
    virtual Label label() const = 0;

    /**
     * Return the number of features
     */
    virtual uint size() const = 0;
};


/**
 * Interface to a dataset
 */
class Dataset
{
public:
    typedef RefCountPtr<Dataset> Ptr;
    typedef RefCountPtr<const LabelArray> LabelArrayPtr;
    typedef RefCountPtr<const FeatureSet> FeatureSetPtr;
    typedef RefCountPtr<const DataSample> DataSamplePtr;
    //typedef RefCountPtr<const IdArray> IdArrayPtr;

    static const Label NoLabel = -1;

    Dataset() {};

    virtual ~Dataset() {};

    /**
     * Return the number of features
     */
    virtual uint numFeatures() const = 0;

    /**
     * Return the number of samples
     */
    virtual uint numSamples() const = 0;

    /**
     * Return a single feature for all samples
     */
    virtual FeatureSetPtr getFeature(uint n) const = 0;

    /**
     * Return a single sample
     */
    virtual DataSamplePtr getSample(Id id) const = 0;

    /**
     * Return the labels of all samples
     */
    virtual LabelArrayPtr getLabels() const = 0;

    /**
     * Get the labels of the specified sample ids
     * ls: The output array to hold the labels
     * ids: Array of sample ids
     */
    virtual void selectLabels(LabelArray& ls, const IdArray& ids) const = 0;

    /**
     * Return the ids of all samples
     */
    virtual void getIds(IdArray& ids) const = 0;

    /**
     * Return the number of classes
     */
    virtual uint numClasses() const = 0;
};


class SingleMatrixDataSample: public DataSample
{
public:
    SingleMatrixDataSample(Id id, const std::vector<FtvalArray>& xs,
                           Label y = Dataset::NoLabel):
        m_id(id), m_xs(xs), m_y(y) {
    }

    virtual Ftval operator[](uint ftid) const {
        assert(ftid < m_xs.size());
        return m_xs[ftid][m_id];
    }

    virtual Id id() const {
        return m_id;
    }

    virtual Label label() const {
        return m_y;
    }

    virtual uint size() const {
        return m_xs.size();
    }

private:
    /**
     * The id of this sample
     */
    const Id m_id;

    /**
     * Reference to the underlying dataset
     */
    const std::vector<FtvalArray>& m_xs;

    /**
     * The class label, Label(-1) if unknown
     */
    const Label m_y;
};


class SingleMatrixFeatureSet: public FeatureSet
{
public:
    SingleMatrixFeatureSet(const FtvalArray& x):
        m_x(x) {
    }

    virtual Ftval operator[](Id id) const {
        assert(id < m_x.size());
        return m_x[id];
    }

    virtual void select(FtvalArray& fts, const IdArray& ids) const {
        Utils::extract(fts, m_x, ids);
    }

    virtual uint size() const {
        return m_x.size();
    }
private:
    /**
     * Reference to the underlying feature vector
     */
    const FtvalArray& m_x;
};


class SingleMatrixDataset: public Dataset
{
public:
    SingleMatrixDataset(uint nr, uint nc):
        m_ids(nr), m_xs(nc), m_ys(nr), m_numClasses(0) {
        for (uint c = 0; c < nc; ++c) {
            m_xs[c].resize(nr);
        }

        for (uint r = 0; r < nr; ++r) {
            m_ids[r] = r;
        }
    }

    void setLabel(uint r, Label l) {
        assert(r < numSamples());
        m_ys[r] = l;
        if (l >= m_numClasses) {
            m_numClasses = l + 1;
        }
    }

    virtual uint numFeatures() const {
        return m_xs.size();
    }

    virtual uint numSamples() const {
        return m_xs[0].size();
    }

    virtual FeatureSetPtr getFeature(uint n) const {
        assert(n < numFeatures());
        return new SingleMatrixFeatureSet(m_xs[n]);
    }

    virtual DataSamplePtr getSample(Id id) const {
        assert(id < numSamples());
        return new SingleMatrixDataSample(id, m_xs, m_ys[id]);
    }

    virtual LabelArrayPtr getLabels() const {
        return new LabelArray(m_ys);
    }

    virtual void selectLabels(LabelArray& ls, const IdArray& ids) const {
        Utils::extract(ls, m_ys, ids);
    }

    virtual void getIds(IdArray& ids) const {
        ids = m_ids;
    }

    virtual uint numClasses() const {
        return m_numClasses;
    }

    // Additional methods specific to this class
    void setX(uint r, uint c, Ftval x) {
        assert(r < numSamples() && c < numFeatures());
        m_xs[c][r] = x;
    }

    Ftval getX(uint r, uint c) const {
        assert(r < numSamples() && c < numFeatures());
        return m_xs[c][r];
    }

private:
    /**
     * Array of sample ids
     */
    IdArray m_ids;

    /**
     * Matrix of samples, accessed in order m_xs[feature][sample]
     */
    std::vector<FtvalArray> m_xs;

    /**
     * Vector of class labels
     */
    LabelArray m_ys;

    /**
     * Number of class labels
     */
    uint m_numClasses;
};


/**
 * A class which permutes a feature, intended for use with variable importance
 * calculations
 */
class PermutedFeatureDataset: public Dataset
{
public:
    /**
     * Construct a data set in which one of the feature values are permuted
     * data: The underlying dataset
     * ftid: The feature id of the feature to be permuted
     */
    PermutedFeatureDataset(const Dataset& data, uint ftid):
        m_data(data), m_permute(ftid) {
        permuteFeature(ftid);
    }

    virtual uint numFeatures() const {
        return m_data.numFeatures();
    }

    virtual uint numSamples() const {
        return m_data.numSamples();
    }

    /**
     * Return a single feature for all samples, possibly permuted
     */
    virtual FeatureSetPtr getFeature(uint n) const {
        if (n == m_permute) {
            return new SingleMatrixFeatureSet(m_permutedValues);
        }

        return m_data.getFeature(n);
    }

    /**
     * Return a single sample, possibly with a permuted feature
     */
    virtual DataSamplePtr getSample(Id id) const {
        return new PermutedFeatureDataSample(
            m_data.getSample(id), m_permute, m_permutedValues[m_permute]);
    }

    virtual LabelArrayPtr getLabels() const {
        return m_data.getLabels();
    }

    virtual void selectLabels(LabelArray& ls, const IdArray& ids) const {
        return m_data.selectLabels(ls, ids);
    }

    virtual void getIds(IdArray& ids) const {
        return m_data.getIds(ids);
    }

    virtual uint numClasses() const {
        return m_data.numClasses();
    }

protected:
    class PermutedFeatureDataSample: public DataSample
    {
    public:
        PermutedFeatureDataSample(const DataSamplePtr sample, uint ftid,
                                  Ftval permutedValue):
            m_sample(sample), m_permute(ftid), m_permutedValue(permutedValue) {
        }

        virtual Ftval operator[](uint ftid) const {
            if (ftid == m_permute) {
                return m_permutedValue;
            }
            return (*m_sample)[ftid];
        }

        virtual Id id() const {
            return m_sample->id();
        }

        virtual Label label() const {
            return m_sample->label();
        }

        virtual uint size() const {
            return m_sample->size();
        }

    private:
        /**
         * Reference to the underlying data sample
         */
        const DataSamplePtr m_sample;

        /**
         * The id of the permuted feature
         */
        const uint m_permute;

        /**
         * The value of the permuted feature
         */
        const Ftval m_permutedValue;
    };

    void permuteFeature(uint ftid) {
        IdArray ids;
        m_data.getIds(ids);
        FeatureSetPtr ft = getFeature(ftid);

        // Get a copy of the original feature, and permute
        ft->select(m_permutedValues, ids);
        std::random_shuffle(m_permutedValues.begin(), m_permutedValues.end());
    }

private:
    /**
     * The underlying dataset
     */
    const Dataset& m_data;

    /**
     *  The id of the feature to be permuted
     */
    const uint m_permute;

    /**
     * The permuted feature
     */
    FtvalArray m_permutedValues;
};


#endif // YARF_DATASET_HPP
