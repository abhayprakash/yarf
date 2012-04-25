/**
 * Random forest trees
 */
#ifndef YARF_RFTREE_HPP
#define YARF_RFTREE_HPP

#include "Dataset.hpp"
#include "RFnode.hpp"
#include "RFutils.hpp"
#include "RFserialise.hpp"
#include <vector>
#include <algorithm>
#include <functional>


/**
 * A random forest tree
 */
class RFtree
{
public:
    typedef RefCountPtr<RFtree> Ptr;

    /**
     * A random forest tree
     * data: The underlying dataset, must remain in scope for the life of the
     *       tree
     * params: Random forest parameters
     */
    RFtree(const Dataset* data, RFparameters::Ptr params):
        m_data(data), m_params(params) {
        data->getIds(m_ids);
        buildTree();
    }

    ~RFtree() { }

    /**
     * Set the data source, needed if this is a deserialised tree
     */
    void setDataset(const Dataset* data) {
        m_data = data;
    }

    /**
     * Get the root node of the tree
     */
    RFnode::Ptr getRoot() const {
        return m_root;
    }

    /**
     * Test the tree using OOB samples
     * err: Array to hold the class error rates
     * Returns the overall class weighted error rate
     */
    double oobErrors(DoubleArray& err) const {
        ConfusionMatrix cm(m_data->numClasses());
        oobPredict(cm, *m_data);
        return cm.classErrorRates(err);
    }

    /**
     * Calculate variable importance for a feature using the OOB samples with a
     * permuted data set
     * permuted: The data set with the values of feature ftid permuted
     * ftid: The id of the permuted feature
     * Returns the feature importance
     */
    double varImp(const Dataset& permuted, uint ftid) const {
        // Variable importance:
        // Votes for correct class - Votes for correct class in permuted data
        // averaged over OOB samples
        ConfusionMatrix pcm(permuted.numClasses());
        oobPredict(pcm, permuted);

        ConfusionMatrix cm(m_data->numClasses());
        oobPredict(cm, *m_data);

        double imp = 0.0;
        for (Label i = 0; i < m_data->numClasses(); ++i) {
            imp += cm.score(i, i) - pcm.score(i, i);
        }
        assert(cm.total() == pcm.total());
        return imp / cm.total();
    }

    /**
     * Get a prediction
     * dist: Array to hold the class predictions
     */
    void predict(DoubleArray& dist, const DataSample& d) const {
        m_root->predict(dist, d);
    }

    /**
     * Save this object
     */
    void serialise(std::ostream& os, uint level, uint i) const {
        os << in(i) << "RFtree{\n"
           << in(i) << "data " << "[0]" << "\n"
           << in(i) << "ids " << arrayToString(m_ids) << "\n"
           << in(i) << "bag " << arrayToString(m_bag) << "\n"
           << in(i) << "oob " << arrayToString(m_oob) << "\n"
           << in(i) << "params\n";
        m_params->serialise(os, level, i + 1);
        os << in(i) << "root\n";
        m_root->serialise(os, level, i + 1);
        os << in(i) << "}RFtree\n";
    }

protected:
    /**
     * Random selection of ids with replacement, and calculation of OOB samples
     */
    void randomBagOob(IdArray& bag, IdArray& oob) const {
        std::vector<bool> selected(m_ids.size(), false);
        bag.resize(m_ids.size());
        oob.reserve(m_ids.size());

        for (IdArray::iterator it = bag.begin(); it != bag.end(); ++it) {
            uint r = Utils::randint(0, m_ids.size());
            *it = m_ids[r];
            selected[r] = true;
        }

        for (uint i = 0; i < selected.size(); ++i) {
            if (!selected[i]) {
                oob.push_back(m_ids[i]);
            }
        }
    }

    /**
     * Build the tree
     */
    void buildTree() {
        randomBagOob(m_bag, m_oob);
        m_root = new RFnode(*m_params, *m_data, m_bag);
    }

    /**
     * Test the tree using OOB samples
     * err: Array to hold the class error rates
     * data: The dataset to use for predictions
     */
    void oobPredict(ConfusionMatrix& cm, const Dataset& data) const {
        assert(m_data->numClasses() == data.numClasses());
        DoubleArray dist;

        for (IdArray::const_iterator it = m_oob.begin();
             it != m_oob.end(); ++it) {
            Dataset::DataSamplePtr d = data.getSample(*it);
            predict(dist, *d);
            assert(d->label() != Dataset::NoLabel);

            cm.inc(d->label(), dist);
        }
    }

private:
    /**
     * Default constructor for deserialisation only
     */
    RFtree() {
    }
    friend class RFbuilder;

    /**
     * The underlying dataset
     */
    const Dataset* m_data;

    /**
     * Parameters
     */
    RFparameters::Ptr m_params;

    /**
     * Dataset sample ids
     */
    IdArray m_ids;

    /**
     * Bagged sample ids
     */
    IdArray m_bag;

    /**
     * Out of bag sample ids
     */
    IdArray m_oob;

    /**
     * Root of the tree
     */
    RFnode::Ptr m_root;
};


/**
 * A forest of trees
 */
class RFforest
{
public:
    typedef RefCountPtr<RFforest> Ptr;

    /**
     * A random forest
     * data: The underlying dataset, must remain in scope for the life of the
     *       forest
     * params: Random forest parameters
     */
    RFforest(const Dataset* data, const RFparameters::Ptr params):
        m_data(data), m_params(params) {
        m_trees.reserve(m_params->numTrees);
        for (uint i = 0; i < m_params->numTrees; ++i) {
            LOG(Log::DEBUG1) << "Building tree " << i;
            m_trees.push_back(new RFtree(data, m_params));
        }
    }

    /**
     * Set the data source, needed if this is a deserialised forest
     */
    void setDataset(const Dataset* data) {
        m_data = data;
        for (std::vector<RFtree::Ptr>::const_iterator it = m_trees.begin();
             it != m_trees.end(); ++it) {
            (*it)->setDataset(data);
        }
    }

    /**
     * Prediction
     * dist: Array to hold the class predictions
     * treeDists: Array of arrays to hold the class predictions from each tree
     * d: Sample to be predicted
     */
    void predict(DoubleArray& dist, std::vector<DoubleArray>& treeDists,
                 const DataSample& d) const {
        treeDists.resize(m_trees.size());
        // Fill with 0
        dist.clear();
        dist.resize(m_data->numClasses());

        for (uint i = 0; i < m_trees.size(); ++i) {
            m_trees[i]->predict(treeDists[i], d);
            std::transform(dist.begin(), dist.end(), treeDists[i].begin(),
                           dist.begin(), std::plus<double>());
        }

        Utils::normalise<DoubleArray>(dist.begin(), dist.end());
    }

    /**
     * Prediction
     * dist: Array to hold the class predictions
     * d: Sample to be predicted
     */
    void predict(DoubleArray& dist, const DataSample& d) const {
        std::vector<DoubleArray> treeDists;
        predict(dist, treeDists, d);
    }

    /**
     * OOB class errors
     * err: Array to hold the class error rates
     * treeErrs: Array of arrays to hold the class error rates from each tree
     */
    void oobErrors(DoubleArray& err, std::vector<DoubleArray>& treeErrs)
        const {
        treeErrs.resize(numTrees());
        // Fill with 0
        err.clear();
        err.resize(m_data->numClasses());

        for (uint i = 0; i < numTrees(); ++i) {
            m_trees[i]->oobErrors(treeErrs[i]);
            std::transform(err.begin(), err.end(), treeErrs[i].begin(),
                           err.begin(), std::plus<double>());
        }

        Utils::normalise<DoubleArray>(err.begin(), err.end(), numTrees());
    }

    /**
     * OOB class errors
     * err: Array to hold the class error rates
     */
    void oobErrors(DoubleArray& err) const {
        std::vector<DoubleArray> treeErrs;
        oobErrors(err, treeErrs);
    }

    // TODO: varImp should also return oobErrors since it's calculated anyway

    /**
     * OOB variable importances
     * imp: Array to hold the feature importances
     * treeImps: Array of arrays to hold the feature importances from each tree
     */
    void varImp(DoubleArray& imp, std::vector<DoubleArray>& treeImps) const {
        treeImps.resize(numTrees());
        std::fill(treeImps.begin(), treeImps.end(),
                  DoubleArray(m_data->numFeatures()));

        // Fill with 0
        imp.resize(m_data->numFeatures());
        std::fill(imp.begin(), imp.end(), 0);

        for (uint ftid = 0; ftid < m_data->numFeatures(); ++ftid) {
            Dataset::Ptr permuted = new PermutedFeatureDataset(*m_data, ftid);

            for (uint i = 0; i < numTrees(); ++i) {
                treeImps[i][ftid] = m_trees[i]->varImp(*permuted, ftid);
                imp[ftid] += treeImps[i][ftid];
            }
        }

        Utils::normalise<DoubleArray>(imp.begin(), imp.end(), numTrees());
    }

    /**
     * OOB variable importances
     * imp: Array to hold the feature importances
     * treeImps: Array of arrays to hold the feature importances from each tree
     */
    void varImp(DoubleArray& imp) const {
        std::vector<DoubleArray> treeImps;
        varImp(imp, treeImps);
    }

    /**
     * Return the number of trees
     */
    uint numTrees() const {
        return m_trees.size();
    }

    /**
     * Get a tree in the forest
     * n: Index of the tree
     */
    RFtree::Ptr getTree(uint n) const {
        assert(n < m_trees.size());
        return m_trees[n];
    }

    /**
     * Save this object
     */
    void serialise(std::ostream& os, uint level, uint i) const {
        os << in(i) << "RFforest{\n"
           << in(i) << "data " << "[0]" << "\n"
           << in(i) << "params\n";
        m_params->serialise(os, level, i + 1);
        os << in(i) << "trees " << "[" << m_trees.size() << "]\n";
        for (std::vector<RFtree::Ptr>::const_iterator it =
                 m_trees.begin(); it != m_trees.end(); ++it) {
            (*it)->serialise(os, level, i + 1);
        }
        os << in(i) << "}RFforest\n";
    }

private:
    /**
     * Default constructor for deserialisation only
     */
    RFforest() {
    }
    friend class RFbuilder;

    /**
     * The underlying dataset
     */
    const Dataset* m_data;

    /**
     * Parameters
     */
    RFparameters::Ptr m_params;

    /**
     * The array of trees
     */
    std::vector<RFtree::Ptr> m_trees;
};


#endif // YARF_RFTREE_HPP

