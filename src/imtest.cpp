/**
 *
 */

#include "ImageDataset.hpp"
#include "ImagePGM.hpp"
#include "HaarFeature.hpp"
#include "RFserialise.hpp"
#include "RFtree.hpp"

#include <iostream>
#include <sstream>

typedef unsigned short ImageT;
typedef Image2D<ImageT> Image;
typedef Image2D<bool> BinImage;
typedef uint IntegralT;
typedef IntegralImage<IntegralT> Integral;


void testRead(const char file[])
{
    Image::Ptr im = ImagePGM::read<ImageT>(file);
    assert(im);
    ImagePGM::write(*im, std::cout, false);
    //ImagePGM::write(*im, "foo.out", true);
}

template <typename IT>
void printImage(std::ostream& os, const IT& im,
                uint x0, uint y0, uint x1, uint y1)
{
    os << im.xsize() << "x" << im.ysize()
       << " (" << x1 - x0 << "x" << y1 - y0 << ")\n";
    for (uint y = y0; y < y1; ++y)
    {
        for (uint x = x0; x < x1; ++x)
        {
            if (x > x0)
            {
                os << " ";
            }
            os << im(x, y);
        }
        os << "\n";
    }
}

void testHaarFeature(const char imageFile[], const char labelFile[])
{
    using std::cout;
    using std::endl;

    Image::Ptr im = ImagePGM::read<ImageT>(imageFile);
    assert(im);
    Integral integral(*im);
    BinImage::Ptr label = ImagePGM::read<bool>(labelFile);
    assert(label);

    cout << "Image\n";
    printImage(cout, *im, 0, 0, 5, 5);
    cout << "Integral\n";
    printImage(cout, integral, 0, 0, 5, 5);

    StringArray ftnames;
    ftnames.push_back("Haar2DRect2_N_1_2_2");
    ftnames.push_back("Haar2DRect2_S_2_1_2");
    ftnames.push_back("Haar2DRect2_E_1_2_2");
    ftnames.push_back("Haar2DRect2_W_2_1_2");

    ftnames.push_back("Haar2DRect4_N_1_2_2_0");
    ftnames.push_back("Haar2DRect4_S_2_1_2_0");
    ftnames.push_back("Haar2DRect4_N_1_2_0_2");
    ftnames.push_back("Haar2DRect4_S_2_1_0_2");

    ftnames.push_back("Haar2DRect4_E_1_2_2_0");
    ftnames.push_back("Haar2DRect4_W_2_1_2_0");
    ftnames.push_back("Haar2DRect4_E_1_2_0_2");
    ftnames.push_back("Haar2DRect4_W_2_1_0_2");
    HaarFeatureManager<ImageT> ftman(ftnames);

    cout << "Border width: " << ftman.borderWidth() << endl;

    Ftval d2n = ftman.getFeature(integral, 0, 2, 2);
    Ftval d2s = ftman.getFeature(integral, 1, 2, 2);
    Ftval d2e = ftman.getFeature(integral, 2, 2, 2);
    Ftval d2w = ftman.getFeature(integral, 3, 2, 2);
    cout << "fts: " << d2n << " " << d2s << " " << d2e << " " << d2w << endl;

    Ftval d4n1 = ftman.getFeature(integral, 4, 2, 2);
    Ftval d4s1 = ftman.getFeature(integral, 5, 2, 2);
    Ftval d4n2 = ftman.getFeature(integral, 6, 2, 2);
    Ftval d4s2 = ftman.getFeature(integral, 7, 2, 2);
    cout << "fts: " << d4n1 << " " << d4s1 << " "
         << d4n2 << " " << d4s2 << endl;

    Ftval d4e1 = ftman.getFeature(integral, 8, 2, 2);
    Ftval d4w1 = ftman.getFeature(integral, 9, 2, 2);
    Ftval d4e2 = ftman.getFeature(integral, 10, 2, 2);
    Ftval d4w2 = ftman.getFeature(integral, 11, 2, 2);
    cout << "fts: " << d4e1 << " " << d4w1 << " "
         << d4e2 << " " << d4w2 << endl;
}


void getFeatureNames(StringArray& ftnames)
{
    const int e[] = {4, 8, 12, 16};
    for (uint i = 0; i < 4; ++i)
    {
        std::ostringstream os;
        os << "Haar2DRect2_" << "N_"
           << e[i] / 2 << '_' << e[i] / 2 << '_' << e[i];
        ftnames.push_back(os.str());

        os.clear();
        os.str("");
        os << "Haar2DRect2_" << "W_"
           << e[i] / 2 << '_' << e[i] / 2 << '_' << e[i];
        ftnames.push_back(os.str());

        os.clear();
        os.str("");
        os << "Haar2DRect4_" << "N_"
           << e[i] / 2 << '_' << e[i] / 2 << '_' << e[i] / 2 << '_' << e[i] / 2;
        ftnames.push_back(os.str());
    }
}


Dataset::Ptr getDataset(const char imageFile[], const char labelFile[])
{
    using std::cout;
    using std::endl;

    Image::Ptr im = ImagePGM::read<ImageT>(imageFile);
    assert(im);
    BinImage::Ptr label = ImagePGM::read<bool>(labelFile);
    assert(label);

    cout << "Image\n";
    printImage(cout, *im, 0, 0, 5, 5);
    //cout << "Integral\n";
    //printImage(cout, integral, 0, 0, 5, 5);

    StringArray ftnames;
    getFeatureNames(ftnames);

    Dataset::Ptr data = new SingleImageHaarDataset<IntegralT, ImageT>(
        ftnames, im, label);

    //cout << "Sample: " << arrayToString(*data->getSample(0)) << endl;
    //cout << "Feature 0: " << arrayToString(*data->getFeature(0)) << endl;
    return data;
}


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


RFforest::Ptr testForest(const Dataset::Ptr data, bool show)
{
    using std::cout;
    using std::endl;

    RFparameters::Ptr params = new RFparameters;
    params->numTrees = 10;
    params->numSplitFeatures = std::ceil(std::sqrt(data->numFeatures()));
    params->minScore = 1e-6;

    RFforest::Ptr forest = new RFforest(data.get(), params);

    cout << indent(80, '*');
    if (show)
    {
        for (uint i = 0; i < forest->numTrees(); ++i)
        {
            cout << "\nTree " << i << "\n";
            printTree(*forest->getTree(i)->getRoot());
        }
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


int main(int argc, char* argv[])
{
    Log::reportingLevel() = Log::DEBUG2;

    //testRead("foo.pgm");
    //testRead("segtest-input-sm2.pgm");
    //testRead("segtest-label-sm2.pbm");
    testHaarFeature("segtest-input-sm2.pgm", "segtest-label-sm2.pbm");
    Dataset::Ptr data = getDataset(
        "segtest-input-sm2.pgm", "segtest-label-sm2.pbm");
    RFforest::Ptr forest = testForest(data, true);
    return 0;
}

