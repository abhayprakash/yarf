/**
 * 2D images
 */
#ifndef YARF_IMAGE2D_HPP
#define YARF_IMAGE2D_HPP

#include "RFtypes.hpp"
#include "refcountptr.hpp"


template <typename T>
class Image2D
{
public:
    /**
     * Reference counted pointer
     */
    typedef RefCountPtr<Image2D<T> > Ptr;

    /**
     * Create an image filled with 0s
     * xsz: X-size (number of columns)
     * ysz: Y-size (number of rows)
     */
    Image2D(uint xsz, uint ysz):
        m_nx(xsz), m_ny(ysz), m_im(new T[xsz * ysz]) {
    }

    virtual ~Image2D() {
        delete [] m_im;
    }

    /**
     * Return the X-size (number of columns)
     */
    virtual uint xsize() const {
        return m_nx;
    }

    /**
     * Return the Y-size (number of rows)
     */
    virtual uint ysize() const {
        return m_ny;
    }

    /**
     * Access an element in form (x, y) or (column, row)
     */
    virtual T at(uint x, uint y) const {
        assert(x < m_nx && y < m_ny);
        return m_im[y * m_nx + x];
    }

    /**
     * Access an element in form (x, y) or (column, row)
     */
    virtual T operator()(uint x, uint y) const {
        return at(x, y);
    }

    /**
     * Access an element in form (x, y) or (column, row)
     */
    virtual T& at(uint x, uint y) {
        assert(x < m_nx && y < m_ny);
        return m_im[y * m_nx + x];
    }

    /**
     * Access an element in form (x, y) or (column, row)
     */
    virtual T& operator()(uint x, uint y) {
        return at(x, y);
    }

    /**
     * Access an element by linear index, using row-major order
     */
    virtual T linearByRow(uint n) const {
        assert(n < m_nx * m_ny);
        return m_im[n];
    }

    /**
     * Access an element by linear index, using column-major order
     */
    virtual T linearByColumn(uint n) const {
        assert(n < m_nx * m_ny);
        uint y = n / m_ny;
        uint x = n - y * m_ny;
        return m_im[y * m_nx + x];
    }

    virtual Image2D* clone() {
        Image2D* p = new Image2D<T>(m_nx, m_ny);
        for (uint i = 0; i < m_nx * m_ny; ++i) {
            p->m_im[i] = m_im[i];
        }
        return p;
    }

private:
    // Prevent non-explicit copying and assignment
    Image2D(const Image2D&);
    void operator=(const Image2D&);

    /**
     * Number of columns
     */
    uint m_nx;

    /**
     * Number of rows
     */
    uint m_ny;

    /**
     * 1D array containing the image
     */
    T* m_im;
};


template <typename T>
class IntegralImage: public Image2D<T>
{
public:
    /**
     * Build an integral image.
     * Note the template type of the target image does not have to be the same,
     * since the range of the integral image may be significantly greater
     * (in principle up to max(InputT) * size(im))
     * im: Image to be integrated, must have non-zero dimensions
     */
    template <typename InputT>
    IntegralImage(const Image2D<InputT>& im):
        Image2D<T>(im.xsize(), im.ysize()) {
        assert(im.xsize() > 0 && im.ysize() > 0);
        integrate(im);
    }

protected:
    /**
     * Calculate the integral image
     */
    template <typename InputT>
    void integrate(const Image2D<InputT>& im) {
        Image2D<T>& a = *this;
        a(0, 0) = 0;

        for (uint x = 1; x < a.xsize(); ++x) {
            a(x, 0) = a(x - 1, 0) + im(x, 0);
        }

        for (uint y = 1; y < a.ysize(); ++y) {
            a(0, y) = a(0, y - 1) + im(0, y);
        }

        for (uint y = 1; y < a.ysize(); ++y) {
            for (uint x = 1; x < a.xsize(); ++x) {
                a(x, y) = a(x - 1, y) + a(x, y - 1) + im(x, y);
            }
        }
    }
};




#endif // YARF_IMAGE2D_HPP
