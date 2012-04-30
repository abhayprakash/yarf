/**
 * Haar-like features
 */
#ifndef YARF_HAARFEATURE_HPP
#define YARF_HAARFEATURE_HPP

#include <string>
#include <sstream>
#include "DataIO.hpp"
#include "RFtypes.hpp"
#include "Image2D.hpp"
#include "Logger.hpp"

// TODO: Make a single generic 2D Haar-like feature class for any number of
// rectangles


namespace HaarFeature
{
    enum Orientation {
        North = 0,
        East,
        South,
        West
    };

    inline char orientationToChar(Orientation o) {
        static const char ornames[] = "NESW";
        return ornames[o];
    }

    inline Orientation charToOrientation(char c) {
        switch (c) {
        case 'N':
            return North;
        case 'E':
            return East;
        case 'S':
            return South;
        case 'W':
            return West;
        default:
            assert(false);
        }
    }
}

template <typename T>
class HaarFeature2D
{
public:
    typedef RefCountPtr<HaarFeature2D> Ptr;

    virtual ~HaarFeature2D() { }

    /**
     * A string representing this feature
     */
    virtual std::string name() const = 0;

    /**
     * The minimum distance from the edge for this Haar feature to be valid
     */
    virtual int requiredBorder() const = 0;

    /**
     * Calculate the Haar feature at the given pixel location
     * im: The integral image from which the feature should be calculated
     * x: X-coordinate of the pixel
     * y: Y-coordinate of the pixel
     */
    virtual Ftval eval(const IntegralImage<T>& im, int x, int y) const = 0;

};


/**
 * A Haar-like feature consisting of two rectangles:
 *
 *       e
 *      / \
 *   / +++++
 * d1  +++++
 *   \ +++++
 *   / -----
 * d2  -----
 *   \ -----
 *
 * d1, d2, e: dimensions
 * o: indicates the position of d1 relative to d2
 */
template <typename T>
class Haar2DRect2: public HaarFeature2D<T>
{
public:

    Haar2DRect2(HaarFeature::Orientation o, int d1, int d2, int e):
        m_o(o), m_d1(d1), m_d2(d2), m_e(e) {
        using namespace HaarFeature;

        switch (o) {
        case North:
        case South:
            m_xoff = - e / 2;
            m_yoff = - (d1 + d2) / 2;
            m_border = std::max((e % 2 == 0)? -m_xoff : -m_xoff + 1,
                                ((d1 + d2) % 2 == 0)? -m_yoff : -m_yoff + 1);
            break;

        case East:
        case West:
            m_xoff = - (d1 + d2) / 2;
            m_yoff = - e / 2;
            m_border = std::max(((d1 + d2) % 2 == 0)? -m_xoff : -m_xoff + 1,
                                (e % 2 == 0)? -m_yoff : -m_yoff + 1);
            break;

        default:
            assert(false);
        }

        std::ostringstream oss;
        oss << "Haar2DRect2" << '_' << orientationToChar(o)
            << '_' << d1 << '_' << d2 << '_' << e;
        m_name = oss.str();
    }

    virtual std::string name() const {
        return m_name;
    }

    virtual int requiredBorder() const {
        return m_border;
    }

    virtual Ftval eval(const IntegralImage<T>& im, int x, int y) const {
        using namespace HaarFeature;

        int x0 = x + m_xoff;
        int y0 = y + m_yoff;
        assert(x0 >= 0);
        assert(y0 >= 0);

        // Convert the integral image values to double to avoid sign issues
        double p1, p2, p3, p4, p5, p6, h;

        switch (m_o) {
        case North:
            // p1+--+p2
            //   |  |
            // p3+--+p4
            //   |  |
            // p5+--+p6
            p1 = im(x0, y0);
            p2 = im(x0 + m_e, y0);
            p3 = im(x0, y0 + m_d1);
            p4 = im(x0 + m_e, y0 + m_d1);
            p5 = im(x0, y0 + m_d1 + m_d2);
            p6 = im(x0 + m_e, y0 + m_d1 + m_d2);

            h = (p1 + p4 - p2 - p3) - (p3 + p6 - p5 - p4);
            break;

        case South:
            // p5+--+p6
            //   |  |
            // p3+--+p4
            //   |  |
            // p1+--+p2
            p1 = im(x0, y0 + m_d1 + m_d2);
            p2 = im(x0 + m_e, y0 + m_d1 + m_d2);
            p3 = im(x0, y0 + m_d2);
            p4 = im(x0 + m_e, y0 + m_d2);
            p5 = im(x0, y0);
            p6 = im(x0 + m_e, y0);

            h = (p3 + p2 - p1 - p4) - (p5 + p4 - p3 - p6);
            break;

        case West:
            // p1 p2 p3
            // +--+--+
            // |  |  |
            // +--+--+
            // p4 p5 p6
            p1 = im(x0, y0);
            p2 = im(x0 + m_d1, y0);
            p3 = im(x0 + m_d1 + m_d2, y0);
            p4 = im(x0, y0 + m_e);
            p5 = im(x0 + m_d1, y0 + m_e);
            p6 = im(x0 + m_d1 + m_d2, y0 + m_e);

            h = (p1 + p5 - p2 - p4) - (p2 + p6 - p3 - p5);
            break;

        case East:
            // p3 p2 p1
            // +--+--+
            // |  |  |
            // +--+--+
            // p6 p5 p4
            p1 = im(x0 + m_d1 + m_d2, y0);
            p2 = im(x0 + m_d2, y0);
            p3 = im(x0, y0);
            p4 = im(x0 + m_d1 + m_d2, y0 + m_e);
            p5 = im(x0 + m_d2, y0 + m_e);
            p6 = im(x0, y0 + m_e);

            h = (p2 + p4 - p1 - p5) - (p3 + p5 - p2 - p6);
            break;
        }

        return h;
    }

private:
    /**
     * Orientation
     */
    typename HaarFeature::Orientation m_o;

    /**
     * Length of the first part of the split dimension
     */
    int m_d1;

    /**
     * Length of the second part of the split dimension
     */
    int m_d2;

    /**
     * Length of the non-split dimension
     */
    int m_e;

    /**
     * X-offset to ensure feature is centred on a pixel
     */
    int m_xoff;

    /**
     * Y-offset to ensure feature is centred on a pixel
     */
    int m_yoff;

    /**
     * The minimum distance from the edge of the image to ensure this Haar
     * feature can be calculated from valid image pixels
     */
    int m_border;

    /**
     * Name of this feature
     */
    std::string m_name;
};


/**
 * A Haar-like feature consisting of four rectangles in a checkerboard:
 *
 *       e1   e2
 *      / \  / \
 *   / +++++-----
 * d1  +++++-----
 *   \ +++++-----
 *   / -----+++++
 * d2  -----+++++
 *   \ -----+++++
 *
 * d1, d2, e1, e2: dimensions
 * o: indicates the position of d1 relative to d2
 */
template <typename T>
class Haar2DRect4: public HaarFeature2D<T>
{
public:

    Haar2DRect4(HaarFeature::Orientation o, int d1, int d2, int e1, int e2):
        m_o(o), m_d1(d1), m_d2(d2), m_e1(e1), m_e2(e2) {
        using namespace HaarFeature;

        switch (o) {
        case North:
        case South:
            m_xoff = - (e1 + e2) / 2;
            m_yoff = - (d1 + d2) / 2;
            m_border = std::max(((e1 + e2) % 2 == 0)? -m_xoff : -m_xoff + 1,
                                ((d1 + d2) % 2 == 0)? -m_yoff : -m_yoff + 1);
            break;

        case East:
        case West:
            m_xoff = - (d1 + d2) / 2;
            m_yoff = - (e1 + e2) / 2;
            m_border = std::max(((d1 + d2) % 2 == 0)? -m_xoff : -m_xoff + 1,
                                ((e1 + e2) % 2 == 0)? -m_yoff : -m_yoff + 1);
            break;

        default:
            assert(false);
        }

        std::ostringstream oss;
        oss << "Haar2DRect4" << '_' << orientationToChar(o)
            << '_' << d1 << '_' << d2 << '_' << e1 << '_' << e2;
        m_name = oss.str();
    }

    virtual std::string name() const {
        return m_name;
    }

    virtual int requiredBorder() const {
        return m_border;
    }

    virtual Ftval eval(const IntegralImage<T>& im, int x, int y) const {
        using namespace HaarFeature;

        int x0 = x + m_xoff;
        int y0 = y + m_yoff;
        assert(x0 >= 0);
        assert(y0 >= 0);

        // Convert the integral image values to double to avoid sign issues
        double p1, p2, p3, p4, p5, p6, p7, p8, p9, h;

        switch (m_o) {
        case North:
            //    e1 e2
            //  p1 p2 p3
            //d1  +  -
            //  p4 p5 p6
            //d2  -  +
            //  p7 p8 p9
            p1 = im(x0, y0);
            p2 = im(x0 + m_e1, y0);
            p3 = im(x0 + m_e1 + m_e2, y0);
            p4 = im(x0, y0 + m_d1);
            p5 = im(x0 + m_e1, y0 + m_d1);
            p6 = im(x0 + m_e1 + m_e2, y0 + m_d1);
            p7 = im(x0, y0 + m_d1 + m_d2);
            p8 = im(x0 + m_e1, y0 + m_d1 + m_d2);
            p9 = im(x0 + m_e1 + m_e2, y0 + m_d1 + m_d2);

            h = (p1 + p5 - p2 - p4) - (p2 + p6 - p3 - p5) -
                (p4 + p8 - p5 - p7) + (p5 + p9 - p6 - p8);
            break;

        case South:
            // p9 p8 p7
            //   +  -  d2
            // p6 p5 p4
            //   -  +  d1
            // p3 p2 p1
            //   e2 e1
            p1 = im(x0 + m_e1 + m_e2, y0 + m_d1 + m_d2);
            p2 = im(x0 + m_e2, y0 + m_d1 + m_d2);
            p3 = im(x0, y0 + m_d1 + m_d2);
            p4 = im(x0 + m_e1 + m_e2, y0 + m_d2);
            p5 = im(x0 + m_e2, y0 + m_d2);
            p6 = im(x0, y0 + m_d2);
            p7 = im(x0 + m_e1 + m_e2, y0);
            p8 = im(x0 + m_e2, y0);
            p9 = im(x0, y0);
            h = (p1 + p5 - p2 - p4) - (p2 + p6 - p3 - p5) -
                (p4 + p8 - p5 - p7) + (p5 + p9 - p6 - p8);
            break;

        case West:
            //  p3 p6 p9
            //e2  -  +
            //  p2 p5 p8
            //e1  +  -
            //  p1 p4 p7
            //    d1 d2
            p1 = im(x0, y0 + m_e1 + m_e2);
            p2 = im(x0, y0 + m_e2);
            p3 = im(x0, y0);
            p4 = im(x0 + m_d1, y0 + m_e1 + m_e2);
            p5 = im(x0 + m_d1, y0 + m_e2);
            p6 = im(x0 + m_d1, y0);
            p7 = im(x0 + m_d1 + m_d2, y0 + m_e1 + m_e2);
            p8 = im(x0 + m_d1 + m_d2, y0 + m_e2);
            p9 = im(x0 + m_d1 + m_d2, y0);

            h = (p2 + p4 - p1 - p5) - (p3 + p5 - p2 - p6) -
                (p5 + p7 - p4 - p8) + (p6 + p8 - p5 - p9);
            break;

        case East:
            //   d2 d1
            // p7 p4 p1
            //   -  +  e1
            // p8 p5 p2
            //   +  -  e2
            // p9 p6 p3
            p1 = im(x0 + m_d1 + m_d2, y0);
            p2 = im(x0 + m_d1 + m_d2, y0 + m_e1);
            p3 = im(x0 + m_d1 + m_d2, y0 + m_e1 + m_e2);
            p4 = im(x0 + m_d2, y0);
            p5 = im(x0 + m_d2, y0 + m_e1);
            p6 = im(x0 + m_d2, y0 + m_e1 + m_e2);
            p7 = im(x0, y0);
            p8 = im(x0, y0 + m_e1);
            p9 = im(x0, y0 + m_e1 + m_e2);

            h = (p2 + p4 - p1 - p5) - (p3 + p5 - p2 - p6) -
                (p5 + p7 - p4 - p8) + (p6 + p8 - p5 - p9);
            break;
        }

        return h;
    }

private:
    /**
     * Orientation
     */
    typename HaarFeature::Orientation m_o;

    /**
     * Length of the first part of the first split dimension
     */
    int m_d1;

    /**
     * Length of the second part of the first split dimension
     */
    int m_d2;

    /**
     * Length of the first part of the second split dimension
     */
    int m_e1;

    /**
     * Length of the second part of the second split dimension
     */
    int m_e2;

    /**
     * X-offset to ensure feature is centred on a pixel
     */
    int m_xoff;

    /**
     * Y-offset to ensure feature is centred on a pixel
     */
    int m_yoff;

    /**
     * The minimum distance from the edge of the image to ensure this Haar
     * feature can be calculated from valid image pixels
     */
    int m_border;

    /**
     * Name of this feature
     */
    std::string m_name;
};


/**
 * Create a 2D Haar feature from a string representation
 */
template <typename T>
class HaarFeature2DFactory
{
public:
    static HaarFeature2D<T>* fromString(const char s[]) {
        Tokeniser tok("_", false);
        tok.set(s);

        HaarFeature2D<T>* h;

        assert(tok.hasNext());
        std::string haartype = tok.next();
        if (haartype == "Haar2DRect2") {
            h = createHaar2DRect2(tok);
        }
        else if (haartype == "Haar2DRect4") {
            h = createHaar2DRect4(tok);
        }
        else {
            LOG(Log::ERROR) << "Unknown Haar feature type: " << haartype;
            return NULL;
        }

        return h;
    }

    static Haar2DRect2<T>* createHaar2DRect2(Tokeniser& tok) {
        assert(tok.hasNext());
        std::string t = tok.next();
        if(t.size() != 1) {
            return NULL;
        }
        HaarFeature::Orientation o = HaarFeature::charToOrientation(t[0]);

        assert(tok.hasNext());
        int d1 = Tokeniser::convert<int>(tok.next());
        assert(tok.hasNext());
        int d2 = Tokeniser::convert<int>(tok.next());
        assert(tok.hasNext());
        int e = Tokeniser::convert<int>(tok.next());

        Haar2DRect2<T>* h = new Haar2DRect2<T>(o, d1, d2, e);
        return h;
    }

    static Haar2DRect4<T>* createHaar2DRect4(Tokeniser& tok) {
        assert(tok.hasNext());
        std::string t = tok.next();
        if(t.size() != 1) {
            return NULL;
        }
        HaarFeature::Orientation o = HaarFeature::charToOrientation(t[0]);

        assert(tok.hasNext());
        int d1 = Tokeniser::convert<int>(tok.next());
        assert(tok.hasNext());
        int d2 = Tokeniser::convert<int>(tok.next());
        assert(tok.hasNext());
        int e1 = Tokeniser::convert<int>(tok.next());
        assert(tok.hasNext());
        int e2 = Tokeniser::convert<int>(tok.next());

        Haar2DRect4<T>* h = new Haar2DRect4<T>(o, d1, d2, e1, e2);
        return h;
    }
};


/**
 * Deals with calculating Haar features from an underlying image
 */
template <typename T>
class HaarFeatureManager
{
public:
    /**
     * Create a list of Haar features from a list of their names
     */
    HaarFeatureManager(const StringArray& features):
        m_border(0) {
        createFeatures(features);
    }

    Ftval getFeature(const IntegralImage<T>& im, uint ftid, int x, int y)
        const {
        return m_features[ftid]->eval(im, x, y);
    }

    uint numFeatures() const {
        return m_features.size();
    }

    int borderWidth() const {
        return m_border;
    }

protected:
    /**
     * Populate the list of Haar features from a list of feature names
     */
    void createFeatures(const StringArray& features) {
        for (StringArray::const_iterator it = features.begin();
             it != features.end(); ++it) {
            HaarFeature2D<T>* h = HaarFeature2DFactory<T>::fromString(
                it->c_str());
            assert(h);

            m_border = std::max(m_border, h->requiredBorder());
            m_features.push_back(h);
        }
    }

private:
    /**
     * The closest a pixel can be to the edge for all Haar features to be valid
     */
    int m_border;

    /**
     * List of Haar-like features
     */
    std::vector<typename HaarFeature2D<T>::Ptr> m_features;
};



#endif // YARF_HAARFEATURE_HPP
