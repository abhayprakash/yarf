/**
 * Test program for the random forest
 */
#include "Dataset.hpp"
#include "RFparameters.hpp"
#include "RFsplit.hpp"
#include "RFnode.hpp"
#include "RFtree.hpp"

#include "ClockTimer.hpp"


#include <iostream>
#include <ctime>
#include <cmath>

#include <algorithm>
#include <functional>

#include "DataIO.hpp"
#include "output.hpp"
#include "Logger.hpp"
#include "RFserialise.hpp"
#include "RFdeserialise.hpp"



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

    // Class label should be in the last column
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
        LOG(Log::DEBUG1) << "ft " << c << "\t"
                         << arrayToString(*d->getFeature(c));
    }
    LOG(Log::DEBUG1) << "ls " << arrayToString(*d->getLabels());

    return pd;
}


bool testRFsplit(const Dataset::Ptr data)
{
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

    printPermutedArray<FtvalArray, UintArray>(fts, s->permLeft(), s->permRight());
    printPermutedArray<LabelArray, UintArray>(ls, s->permLeft(), s->permRight());
    printPermutedArray<IdArray, UintArray>(ids, s->permLeft(), s->permRight());

    std::cout << arrayToString(s->getInfoGainArray())
              << "IG: " << ig << " split-val: " << sv << std::endl;

    printPermutedArray<FtvalArray, UintArray>(fts, s->permLeft(), s->permMiddle());
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

RFforest::Ptr testForest(const Dataset::Ptr data)
{
    using std::cout;
    using std::endl;

    RFparameters::Ptr params = new RFparameters;
    params->numTrees = 10;
    params->numSplitFeatures = std::ceil(std::sqrt(data->numFeatures()));
    params->minScore = 1e-6;

    RFforest::Ptr forest = new RFforest(data.get(), params);

    cout << indent(80, '*');
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

    return forest;
}


void testPredict(const Dataset::Ptr data, const RFforest::Ptr f1,
                 const RFforest::Ptr f2)
{
    // Note: To check floating point saving/loading test ids[77] tree 8
    // and ids[106] 
    using std::cout;
    using std::endl;

    IdArray ids;
    data->getIds(ids);
    //ids.push_back(77);
    //ids.push_back(106);

    if (!f2)
    {
        std::vector<DoubleArray> preds1(ids.size());
        f1->setDataset(data.get());

        cout << '\n';
        for (uint i = 0; i < ids.size(); ++i)
        {
            f1->predict(preds1[i], *data->getSample(ids[i]));
            cout << "Prediction " << i << ":\t"
                 << arrayToString(preds1[i]) << "\n";
        }
    }

    if (f1 && f2)
    {
        std::vector<DoubleArray> preds1(ids.size()), preds2(ids.size());
        f1->setDataset(data.get());
        f2->setDataset(data.get());

        cout << '\n';
        for (uint i = 0; i < ids.size(); ++i)
        {
            f1->predict(preds1[i], *data->getSample(ids[i]));
            f2->predict(preds2[i], *data->getSample(ids[i]));

            bool b = Utils::equals<DoubleArray>(
                preds1[i].begin(), preds1[i].end(),
                preds2[i].begin(), preds2[i].end());

            cout << "Predictions " << i << ":\t"
                 << arrayToString(preds1[i]) << "\t"
                 << arrayToString(preds2[i]) << "\t"
                 << (b? "==" : "!=") << "\n";
        }

        bool b = Utils::array2equals<std::vector<DoubleArray> >(
            preds1.begin(), preds1.end(),
            preds2.begin(), preds2.end());
        cout << "\npreds1 " << (b? "==" : "!=") << " preds2" << endl;
    }

}

void testSerialise(RFforest::Ptr forest, const char fname[])
{
    //std::cout << indent(80, '*') << std::endl;
    std::ofstream fout(fname);

    //forest->getTree(0)->serialise(fout, 2, 0);
    forest->serialise(fout, 2, 0);
}


RFforest::Ptr testDeserialise(const char fname[])
{
    using std::cout;
    //using std::cerr;
    using std::endl;

    //cout << indent(80, '*') << endl;
    std::ifstream fin(fname);

    Deserialiser deserial(fin);
    if (!fin)
    {
        LOG(Log::ERROR) << "Failed to open " << fname << endl;
        return NULL;
    }

    RFbuilder builder(deserial);
    RFforest::Ptr forest = builder.dRFforest(builder.nextToken());
    //RFtree::Ptr tree = builder.dRFtree(builder.nextToken());

    Deserialiser::Token token = deserial.next();
    while (token.type != Deserialiser::ParseError) {
        cout << Deserialiser::toString(token) << endl;
        token = deserial.next();
    }
    LOG(Log::WARNING) << Deserialiser::toString(token) << endl;

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


int main(int argc, char* argv[])
{
    ClockTimer timer;

    uint seed = 123;
    Utils::srand(seed);
    //Log::reportingLevel() = Log::INFO;
    Log::reportingLevel() = Log::DEBUG2;

    RFforest::Ptr f;
    Dataset::Ptr ds;

    timer.time("Getting dataset");
    //ds = createTestDataset(10, 1);
    //testRFsplit(ds);
    //ds = createTestDataset(100, 4);
    //testRFnode(ds);
    //testForest(ds);
    //ds = openTestDataset("../data/ionosphere.csv");
    ds = openTestDataset("../data/iris.csv");

    timer.time("Creating forest");
    f = testForest(ds);

    timer.time("Serialising forest");
    const char fname[] = "serialise2.out";
    testSerialise(f, fname);

    timer.time("Deserialising forest");
    RFforest::Ptr df = testDeserialise(fname);

    timer.time("Serialising forest");
    // Serialise the deserialised tree, so that a diff can be run manually
    const char fnamedf[] = "serialise3.out";
    testSerialise(df, fnamedf);

    timer.time("Comparing predictions");
    // Both forests should give the same predictions
    testPredict(ds, f, df);

    timer.time("Finished");

    printTimes(timer);
    return 0;
}


