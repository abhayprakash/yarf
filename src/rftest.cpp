/**
 * Test program for the random forest
 */
#include "DataIO.hpp"
#include "Dataset.hpp"

#include "RFparameters.hpp"
#include "RFnode.hpp"
#include "RFtree.hpp"

#include "RFserialise.hpp"
#include "RFdeserialise.hpp"

#include "Logger.hpp"
#include "ClockTimer.hpp"


#include <iostream>
#include <ctime>
#include <cmath>

#include <algorithm>
#include <functional>



void createTestData(FtvalArray& fts, LabelArray& ls, IdArray& ids, uint& ncls)
{
    static const Ftval ftsa[] = {10, 2, 65, 176, 121, 65, 36, 65, 10};
    static const Label lsa[] = {0, 0, 1, 1, 1, 1, 0, 0, 0};
    //static const Ftval ftsa[] = {10, 2, 65, 17};
    //static const Label lsa[] = {0, 0, 1, 1};

    uint n = sizeof(ftsa) / sizeof(ftsa[0]);
    fts.assign(ftsa, ftsa + n);
    ls.assign(lsa, lsa + n);

    n = 10;

    fts.resize(n);
    std::generate(fts.begin(), fts.end(), std::rand);
    std::transform(fts.begin(), fts.end(), fts.begin(),
                   std::bind2nd(std::modulus<uint>(), 100));
    ls.resize(n);
    std::generate(ls.begin(), ls.end(), std::rand);
    //std::transform(ls.begin(), ls.end(), ls.begin(),
    //               std::bind2nd(std::modulus<uint>(), ncls));
    std::transform(fts.begin(), fts.end(), ls.begin(),
                   std::bind2nd(std::divides<uint>(), 25));

    ncls = *std::max_element(ls.begin(), ls.end()) + 1;
    ids.resize(n);

    for (uint i = 0; i < n; ++i)
    {
        ids[i] = i;
    }

    LOG(Log::DEBUG1) << "fts " << arrayToString(fts);
    LOG(Log::DEBUG1) << "ls " << arrayToString(ls);
    LOG(Log::DEBUG1) << "ids " << arrayToString(ids);
}


Dataset::Ptr createTestDataset(uint n, uint f)
{
    SingleMatrixDataset* d = new SingleMatrixDataset(n, f);
    Dataset::Ptr pd(d);

    for (uint r = 0; r < n; ++r)
    {
        int y = 0;
        for (uint c = 0; c < f; ++c)
        {
            double x = 0.1 * Utils::randint(-100, 100);
            d->setX(r, c, x);

            y += x;
        }
        y = (d->getX(r, 0) + 10) / 10;
        d->setLabel(r, y);
    }

    for (uint c = 0; c < d->numFeatures(); ++c)
    {
        LOG(Log::DEBUG1) << "ft " << c
                         << arrayToString(*d->getFeature(c));
    }
    LOG(Log::DEBUG1) << "ls " << arrayToString(*d->getLabels());
    //LOG(Log::DEBUG1) << "ids " << arrayToString(ids);

    return pd;
}


Dataset::Ptr openTestDataset(const char fname[])
{
    CsvReader csv;
    bool status = csv.parse(fname);
    if (!status) {
        LOG(Log::ERROR) << "Error parsing " << fname;
        exit(1);
    }

    // Class label(ground truth) should be in the last column
    SingleMatrixDataset* d = new SingleMatrixDataset(
        csv.rows(), csv.cols() - 1);
    Dataset::Ptr pd(d);
    uint colLabel = csv.cols() - 1;

    for (uint r = 0; r < csv.rows(); ++r)
    {
        for (uint c = 0; c < csv.cols() - 1; ++c)
        {
            d->setX(r, c, csv(r, c));
        }
        d->setLabel(r, csv(r, colLabel));
    }

    for (uint c = 0; c < d->numFeatures(); ++c)
    {
        LOG(Log::DEBUG2) << "ft " << c << "\t"
                         << arrayToString(*d->getFeature(c));
    }
    LOG(Log::DEBUG2) << "ls " << arrayToString(*d->getLabels());

    return pd;
}


template<typename ContainerT1, typename ContainerT2>
void printPermutedArray(const ContainerT1& xs,
                        typename ContainerT2::const_iterator a,
                        typename ContainerT2::const_iterator b)
{
    using std::cout;
    using std::endl;

    assert(b >= a && xs.size() >= uint(b - a));
    cout << "[" << b - a << "] ";

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

bool testRFsplit(const Dataset::Ptr data)
{
    using std::cout;
    using std::endl;

    FtvalArray fts;
    LabelArray ls;
    IdArray ids;

    data->getIds(ids);
    data->selectLabels(ls, ids);

    RFparameters params;
    params.numTrees = 1;
    params.numSplitFeatures = data->numFeatures();
    params.minScore = 0.0;

    MaxInfoGainSplit splitter(params, *data, ls, ids, DoubleArray());
    MaxInfoGainSingleSplit::Ptr s = splitter.getSplit();

    data->getFeature(s->getFeatureId())->select(fts, ids);
    double ig = s->getInfoGain();
    Ftval sv = s->getSplitValue();

    cout << "fts\t";
    printPermutedArray<FtvalArray, UintArray>(fts, s->permLeft(), s->permRight());
    cout << "ls\t";
    printPermutedArray<LabelArray, UintArray>(ls, s->permLeft(), s->permRight());
    cout << "ids\t";
    printPermutedArray<IdArray, UintArray>(ids, s->permLeft(), s->permRight());

    cout << "IGs\t" << arrayToString(s->getInfoGainArray())
              << "IG: " << ig << " split-val: " << sv << endl;

    cout << "fts (left)\t";
    printPermutedArray<FtvalArray, UintArray>(fts, s->permLeft(), s->permMiddle());
    cout << "fts (right)\t";
    printPermutedArray<FtvalArray, UintArray>(fts, s->permMiddle(), s->permRight());

    return true;
};

void printTree(const RFnode& t, uint depth = 0)
{
    using std::cout;
    using std::endl;

    DoubleArray dist;
    t.getClassDistribution(dist, false);
    cout << indent(depth * 2)
         << "counts: " << arrayToString(dist, false) << endl;

    t.getClassDistribution(dist, true);
    cout << indent(depth * 2)
         << "normalised: " << arrayToString(dist, false) << endl;

    if (!t.isleaf())
    {
        SplitSelector::CPtr splitter = t.getSplit();
        const MaxInfoGainSplit* s =
            dynamic_cast<const MaxInfoGainSplit*>(splitter.get());
        double ig = s->getSplit()->getInfoGain();
        Ftval sv = s->getSplit()->getSplitValue();
        uint ftid = s->getSplit()->getFeatureId();

        cout << indent(depth * 2) << "Feature: " << ftid
             << " split: " << sv << " IG: " << ig << endl;
    }

    if (t.left() && t.right())
    {
        cout << indent(depth * 2) << "Left" << endl;
        printTree(*t.left(), depth + 1);

        cout << indent(depth * 2) << "Right" << endl;
        printTree(*t.right(), depth + 1);
    }
}

bool testRFnode(const Dataset::Ptr data)
{
    using std::cout;

    RFparameters::Ptr params = new RFparameters;
    params->numTrees = 1;
    params->numSplitFeatures = std::ceil(std::sqrt(data->numFeatures()));
    params->minScore = 1e-6;

    RFtree::Ptr tree = new RFtree(data.get(), params);

    cout << indent(80, '*') << "\n";
    printTree(*tree->getRoot());

    DoubleArray oobErr;
    tree->oobErrors(oobErr);
    cout << "\nOOB error: " << arrayToString(oobErr) << "\n";

    return true;
}

RFforest::Ptr testForest(const Dataset::Ptr data, bool show, int NUMTREE)
{
    using std::cout;
    using std::endl;

    RFparameters::Ptr params = new RFparameters;
    params->numTrees = NUMTREE;
    params->numSplitFeatures = std::ceil(std::sqrt(data->numFeatures()));
    params->minScore = 1e-6;

    RFforest::Ptr forest = new RFforest(data.get(), params);

    if (show)
    {
        for (uint i = 0; i < forest->numTrees(); ++i)
        {
            cout << "\nTree " << i << "\n";
            printTree(*forest->getTree(i)->getRoot());
        }

        std::vector<DoubleArray> treeErrs;
        DoubleArray oobErr;
        forest->oobErrors(oobErr, treeErrs);
        cout << '\n';
        for (uint i = 0; i < treeErrs.size(); ++i)
        {
            cout << "OOB error tree " << i << ":\t"
                 << arrayToString(treeErrs[i]) << "\n";
        }
        cout << "\nOOB error: " << arrayToString(oobErr) << endl;

        std::vector<DoubleArray> treeImps;
        DoubleArray imp;
        forest->varImp(imp, treeImps);
        for (uint i = 0; i < treeImps.size(); ++i)
        {
            cout << "Feature importance tree " << i << ":\t"
                 << arrayToString(treeImps[i]) << "\n";
        }
        cout << "\nFeature importance: " << arrayToString(imp) << endl;
    }
    return forest;
}

void printTimes(const ClockTimer& timer)
{
    using std::cout;
    using std::endl;

    cout << "\nTimings:" << endl;
    ClockTimer::ClockTimes::const_iterator it = timer.getTimes().begin();

    while (true)
    {
        cout << it->first << ":\t" << it->second;
        ClockTimer::ClockTimes::const_iterator prev = it++;

        if (it == timer.getTimes().end())
        {
            break;
        }
        else
        {
            cout << "\t+" << it->second - prev->second;
        }
        cout << endl;
    }

    cout << endl;
}

void predictClass(const Dataset::Ptr data, const RFforest::Ptr f)
{
    using std::cout;
    using std::endl;

    IdArray ids;
    data->getIds(ids);

    std::vector<DoubleArray> preds1(ids.size());
    f->setDataset(data.get());

    int result;
    double mx = -1;

    for (uint i = 0; i < ids.size(); ++i)
    {
        f->predict(preds1[i], *data->getSample(ids[i]));

        cout << getClass_MaxProb(preds1[i]) << "\n";
    }
}

int main(int argc, char* argv[])
{
    ClockTimer timer;

    uint seed = 25; // not time(0) to make results reproducible
    Utils::srand(seed);
    //Log::reportingLevel() = Log::INFO;
    Log::reportingLevel() = Log::DEBUG1;
    //Log::reportingLevel() = Log::DEBUG2;

    RFforest::Ptr f;
    Dataset::Ptr ds;

    timer.time("Getting dataset");

    int numTree;
    if (argc > 1)
    {
        ds = openTestDataset(argv[1]);
    }
    else
    {
        //ds = openTestDataset("../data/ionosphere.csv");
        ds = openTestDataset("../data/iris.csv");
        //ds = openTestDataset("../data/feature_train.csv");
    }

    if(argc > 2)
    {
        numTree = atoi(argv[2]);
    }
    else
    {
        numTree = 10;
    }

    timer.time("Creating forest");
    f = testForest(ds, false, numTree);

    timer.time("Prediction");
    predictClass(ds, f);

    timer.time("Finished");
    printTimes(timer);
    return 0;
}


