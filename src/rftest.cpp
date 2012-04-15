/**
 * Test program for the random forest
 */
#include "Dataset.hpp"
#include "RFparameters.hpp"
#include "RFsplit.hpp"
#include "RFnode.hpp"
#include "RFtree.hpp"

#include <iostream>
#include <ctime>
#include <cmath>

#include <algorithm>
#include <functional>

#include "DataIO.hpp"
#include "output.hpp"


void createTestData(FtvalArray& fts, LabelArray& ls, IdArray& ids, uint& ncls)
{
    using std::cout;
    using std::endl;

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

    cout << "fts\t";
    printArray(fts);
    cout << "ls\t";
    printArray(ls);
    cout << "ids\t";
    printArray(ids);
}


Dataset::Ptr createTestDataset(uint n, uint f)
{
    using std::cout;
    using std::endl;

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
        //y = (y + 20) / 20;
        y = (d->getX(r, 0) + 10) / 10;
        d->setLabel(r, y);
    }

    for (uint c = 0; c < d->numFeatures(); ++c)
    {
        cout << "ft " << c << "\t";
        printArray(*d->getFeature(c));
    }
    cout << "ls\t";
    printArray(*d->getLabels());

    //cout << "ids\t";
    //printArray(ids);

    return pd;
}


Dataset::Ptr openTestDataset(const char file[])
{
    using std::cout;
    using std::cerr;
    using std::endl;

    CsvReader csv;
    bool status = csv.parse(file);
    if (!status) {
        cerr << "Error parsing " << file << endl;
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
        cout << "ft " << c << "\t";
        printArray(*d->getFeature(c));
    }
    cout << "ls\t";
    printArray(*d->getLabels());

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

    printArray(s->getInfoGainArray());

    std::cout << "IG: " << ig << " split-val: " << sv << std::endl;

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
    print("  ", depth);
    cout << "counts: ";
    printArray(dist, false);

    t.getClassDistribution(dist, true);
    print("  ", depth);
    cout << "normalised: ";
    printArray(dist, false);

    if (!t.isleaf())
    {
        SplitSelector::CPtr splitter = t.getSplit();
        const MaxInfoGainSplit* s =
            dynamic_cast<const MaxInfoGainSplit*>(splitter.get());
        double ig = s->getSplit()->getInfoGain();
        Ftval sv = s->getSplit()->getSplitValue();
        uint ftid = s->getSplit()->getFeatureId();
        print("  ", depth);
        cout << "Feature: " << ftid << " split: " << sv << " IG: " << ig <<
            endl;
    }

    if (t.left() && t.right())
    {
        print("  ", depth);
        cout << "Left" << endl;
        printTree(*t.left(), depth + 1);

        print("  ", depth);
        cout << "Right" << endl;
        printTree(*t.right(), depth + 1);
    }
}

bool testRFnode(const Dataset::Ptr data)
{
    RFparameters params;
    params.numTrees = 1;
    params.numSplitFeatures = std::ceil(std::sqrt(data->numFeatures()));
    params.minScore = 1e-6;

    RFtree::Ptr tree = new RFtree(*data, params);

    print("*", 80);
    print('\n');
    printTree(*tree->getRoot());

    DoubleArray oobErr;
    tree->oobErrors(oobErr);
    print("\nOOB error\t");
    printArray(oobErr);

    return true;
}

bool testForest(const Dataset::Ptr data)
{
    RFparameters params;
    params.numTrees = 50;
    params.numSplitFeatures = std::ceil(std::sqrt(data->numFeatures()));
    params.minScore = 1e-6;

    RFforest forest(*data, params);

    print("*", 80);
    for (uint i = 0; i < forest.numTrees(); ++i)
    {
        std::cout << "\nTree " << i << "\n";
        printTree(*forest.getTree(i)->getRoot());
    }

    std::vector<DoubleArray> treeErrs;
    DoubleArray oobErr;
    forest.oobErrors(oobErr, treeErrs);
    print('\n');
    for (uint i = 0; i < treeErrs.size(); ++i)
    {
        std::cout << "OOB error " << i << "\t";
        printArray(treeErrs[i]);
    }
    print("\nOOB error\t");
    printArray(oobErr);

    std::vector<DoubleArray> treeImps;
    DoubleArray imp;
    forest.varImp(imp, treeImps);
    print('\n');
    for (uint i = 0; i < treeImps.size(); ++i)
    {
        std::cout << "Feature importance " << i << "\t";
        printArray(treeImps[i]);
    }
    print("\nFeature importance\t");
    printArray(imp);
    std::cout << imp;

    return true;
}

int main(int argc, char* argv[])
{
    std::srand(std::time(NULL));

    //testRFsplit(createTestDataset(10, 1));
    //testRFnode(createTestDataset(100, 4));
    //testForest(createTestDataset(100, 4));
    //testForest(openTestDataset("ionosphere.csv"));
    testForest(openTestDataset("iris.csv"));
    return 0;
}


