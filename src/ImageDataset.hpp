/**
 * Classes to handle one or more images as the input/output to the random forest
 */
#ifndef YARF_IMAGEDATASET_HPP
#define YARF_IMAGEDATASET_HPP

#include "Dataset.hpp"
#include "Image2D.hpp"
#include "HaarFeature.hpp"



/**
 * An example dataset using Haar-like features on a single image.
 * Note that border regions are omitted from the list of considered points.
 */
template <typename IntegralT>
class HaarFeatureSet: public FeatureSet
{
public:
    HaarFeatureSet(const HaarFeatureManager<IntegralT>& haar,
                   const IntegralImage<IntegralT>& im, uint ftid,
                   bool omitBorders = true):
        m_haar(haar), m_im(im), m_ftid(ftid) {
        assert(omitBorders);
        // TODO: Implement handling of points close to the edge of the image
    }

    virtual Ftval operator[](Id id) const {
        // First calculate the x-y coordinate in the region excluding borders
        // Then add on the border to find the coordinates in the full image
        uint xsz = m_im.xsize() - 2 * m_haar.borderWidth();
        uint ysz = m_im.ysize() - 2 * m_haar.borderWidth();
        assert(id < xsz * ysz);

        uint x = id % xsz + m_haar.borderWidth();
        uint y = id / xsz + m_haar.borderWidth();

        return m_haar.getFeature(m_im, m_ftid, x, y);
    }

    virtual void select(FtvalArray& fts, const IdArray& ids) const {
        fts.resize(ids.size());
        for (uint i = 0; i < ids.size(); ++i) {
            fts[i] = operator[](ids[i]);
        }
    }

    virtual uint size() const {
        return (m_im.xsize() - 2 * m_haar.borderWidth()) *
            (m_im.ysize() - 2 * m_haar.borderWidth());
    }

private:
    /**
     * Reference to the underlying features
     */
    const HaarFeatureManager<IntegralT>& m_haar;

    /**
     * The integral image
     * TODO: Should be a list
     */
    const IntegralImage<IntegralT>& m_im;

    /**
     * The feature index
     */
    const uint m_ftid;
};


template <typename IntegralT>
class HaarDataSample: public DataSample
{
public:
    HaarDataSample(Id id, uint xpos, uint ypos,
                   const HaarFeatureManager<IntegralT>& haar,
                   const IntegralImage<IntegralT>& im,
                   Label y = Dataset::NoLabel):
        m_id(id), m_xpos(xpos), m_ypos(ypos), m_haar(haar), m_im(im), m_y(y) {
    }

    virtual Ftval operator[](uint ftid) const {
        assert(ftid < m_haar.numFeatures());
        return m_haar.getFeature(m_im, ftid, m_xpos, m_ypos);
    }

    virtual Id id() const {
        return m_id;
    }

    virtual Label label() const {
        return m_y;
    }

    virtual uint size() const {
        return m_haar.numFeatures();
    }

private:
    /**
     * The id of this sample
     */
    const Id m_id;

    /**
     * The corresponding pixel X co-ordinate
     */
    uint m_xpos;

    /**
     * The corresponding pixel Y co-ordinate
     */
    uint m_ypos;

    /**
     * Reference to the feature calculator
     */
    const HaarFeatureManager<IntegralT>& m_haar;

    /**
     * The integral image corresponding to this sample
     */
    const IntegralImage<IntegralT>& m_im;

    /**
     * The class label, Label(-1) if unknown
     */
    const Label m_y;
};


/**
 * A dataset consisting of Haar-like features at the pixels of a single image
 */
template <typename IntegralT, typename ImageT>
class SingleImageHaarDataset: public Dataset
{
public:
    SingleImageHaarDataset(const StringArray& features,
                           typename Image2D<ImageT>::Ptr im,
                           Image2D<bool>::Ptr label):
        m_im(im), m_integral(*im), m_label(label), m_features(features) {
        assert(!features.empty());
        assert(im->xsize() == label->xsize() && im->ysize() == label->ysize());

        uint bw = m_features.borderWidth();
        assert(im->xsize() > 2 * bw && im->ysize() > 2 * bw);

        m_ids.resize(numSamples());
        m_ys.resize(numSamples());

        uint x = bw;
        uint y = bw;
        uint xmax = m_label->xsize() - bw;

        for (uint i = 0; i < numSamples(); ++i) {
            m_ids[i] = i;

            m_ys[i] = m_label->at(x, y);
            if (++x == xmax) {
                x = bw;
                ++y;
            }
        }
    }

    virtual uint numFeatures() const {
        return m_features.numFeatures();
    }

    virtual uint numSamples() const {
        return (m_im->xsize() - 2 * m_features.borderWidth()) *
            (m_im->ysize() - 2 * m_features.borderWidth());
    }

    virtual FeatureSetPtr getFeature(uint n) const {
        assert(n < numFeatures());
        return new HaarFeatureSet<IntegralT>(m_features, m_integral, n);
    }

    virtual DataSamplePtr getSample(Id id) const {
        assert(id < numSamples());
        uint xpos, ypos;
        idToImageLocation(id, xpos, ypos);
        return new HaarDataSample<IntegralT>(id, xpos, ypos, m_features,
                                             m_integral,
                                             m_label->at(xpos, ypos));
    }

    virtual LabelArrayPtr getLabels() const {
        LabelArray* ls = new LabelArray(m_ids.size());

        for (uint i = 0; i < m_ids.size(); ++i) {
            (*ls)[i] = m_label->linearByRow(i);
        }

        return ls;
    }

    virtual void selectLabels(LabelArray& ls, const IdArray& ids) const {
        Utils::extract(ls, m_ys, ids);
    }

    virtual void getIds(IdArray& ids) const {
        ids = m_ids;
    }

    virtual uint numClasses() const {
        return 2;
    }

protected:
    void idToImageLocation(Id id, uint& xpos, uint& ypos) const {
        // Convert the id into a pixel location
        // See HaarFeatureSet::operator[]
        uint xsz = m_im->xsize() - 2 * m_features.borderWidth();
        uint ysz = m_im->ysize() - 2 * m_features.borderWidth();
        assert(id < xsz * ysz);

        xpos = id % xsz + m_features.borderWidth();
        ypos = id / xsz + m_features.borderWidth();
    }

private:
    /**
     * Array of sample ids
     */
    IdArray m_ids;

    /**
     * The input image
     */
    typename Image2D<ImageT>::Ptr m_im;

    /**
     * Integral image
     */
    IntegralImage<IntegralT> m_integral;

    /**
     * Labels for each pixel
     */
    Image2D<bool>::Ptr m_label;

    /**
     * Labels for each pixel in linear form, excluding border regions
     */
    LabelArray m_ys;

    /**
     * The features
     */
    HaarFeatureManager<IntegralT> m_features;
};



#endif // YARF_IMAGEDATASET_HPP
