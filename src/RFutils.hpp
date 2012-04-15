/**
 * Useful functions
 */
#ifndef YARF_RFUTILS_HPP
#define YARF_RFUTILS_HPP

#include <ctime>
#include <cstdlib>
#include <algorithm>
#include <functional>

class Utils
{
public:
    static void srand(uint n = 0) {
        if (n == 0) {
            std::srand(std::time(NULL));
        }
        else {
            std::srand(n);
        }
    }

    static int randint(int minn, int maxn) {
        return std::rand() % (maxn - minn) + minn;
    }

    /**
     * Extracts the relevant elements according to select
     */
    template <typename Td, typename Ts>
    static void extract(Td& dst, const Ts& src, const UintArray& select) {
        dst.resize(select.size());
        for (uint i = 0; i < select.size(); ++i) {
            dst[i] = src[select[i]];
        }
    }

    /**
     * Check if an array is sorted in ascending order (A[i+1]>=A[i])
     */
    template <typename ContainerT>
    static bool issorted(typename ContainerT::const_iterator a,
                         typename ContainerT::const_iterator b) {
        typename ContainerT::const_iterator prev = a;
        while (++a != b) {
            if (*prev > *a) {
                return false;
            }
            prev = a;
        }
        return true;
    }

    /**
     * A functional object which accumulates (sums) each value
     */
    template <typename T>
    class Accumulator
    {
    public:
        /**
         * Adds x to the total
         */
        T operator()(const T& x) {
            m_x += x;
            return m_x;
        }

        /**
         * Returns the total
         */
        T operator()() const {
            return m_x;
        }

    protected:
        T m_x;
    };

    /**
     * Normalise an array to sum to 1
     * xs: The array of value
     * div: Normalise by dividing each element of xs by this value, if 0 then
     *      calculate the total
     */
    template <typename T>
    static void normalise(typename T::iterator from, typename T::iterator to,
                          typename T::value_type div = 0) {
        typedef typename T::value_type V;
        if (div == 0) {
            div = for_each(from, to, Utils::Accumulator<V>())();
        }
        std::transform(from, to, from, std::bind2nd(std::divides<V>(), div));
    }
};


/**
 * A confusion matrix
 */
class ConfusionMatrix
{
public:
    /**
     * Create a confusion matrix, which optionally also tracks the scores/votes
     * ncls: Number of class labels
     */
    ConfusionMatrix(uint ncls):
        m_ncls(ncls), m_cm(ncls * ncls), m_score(ncls * ncls), m_n(0) {
    }

    /**
     * Access an entry in the confusion matrix
     * iTrue: True label
     * iPred: Predicted label
     */
    uint operator()(Label iTrue, Label iPred) const {
        return m_cm[index(iTrue, iPred)];
    }

    /**
     * Access an entry in the scores confusion matrix
     * iTrue: True label
     * iPred: Predicted label
     */
    double score(Label iTrue, Label iPred) const {
        return m_score[index(iTrue, iPred)];
    }

    /**
     * Return the number of samples processed
     */
    uint total() const {
        return m_n;
    }

    /**
     * Increment an entry in the confusion matrix, and the scoring matrix
     * iTrue: True label
     * distPred: Predicted distribution of class labels, the maximum is used as
     *           the predicted label
     */
    void inc(Label iTrue, const DoubleArray& distPred) {
        assert(distPred.size() == m_ncls);
        DoubleArray::const_iterator p =
            max_element(distPred.begin(), distPred.end());
        assert(*p > 0);

        uint iPred = p - distPred.begin();
        m_cm[index(iTrue, iPred)] += 1;

        for (Label q = 0; q < m_ncls; ++q) {
            m_score[index(iTrue, q)] += distPred[q];
        }

        ++m_n;
    }

    /**
     * Error rates for each true class, and overall class weighted error rate
     * err: Array to hold the class error rates
     * Returns the overall class weighted error rate
     */
    double classErrorRates(DoubleArray& err) const {
        err.resize(m_ncls);
        double overallCorrect = 0.0;

        for (Label t = 0; t < m_ncls; ++t) {
            double clTotal = 0.0;
            for (Label p = 0; p < m_ncls; ++p) {
                clTotal += m_cm[index(t, p)];
            }

            err[t] = (clTotal - double(m_cm[index(t, t)])) / clTotal;
            overallCorrect += double(m_cm[index(t, t)]);
        }

        return (double(m_n) - overallCorrect) / double(m_n);
    }

protected:
    /**
     * Get the index of an element
     */
    uint index(Label iTrue, Label iPred) const {
        assert(iTrue < m_ncls && iPred < m_ncls);
        return iTrue * m_ncls + iPred;
    }

private:
    /**
     * Number of classes
     */
    uint m_ncls;

    /**
     * The confusion matrix
     */
    std::vector<uint> m_cm;

    /**
     * The confusion matrix of cumulative scores
     */
    std::vector<double> m_score;

    /**
     * The number of samples
     */
    uint m_n;
};



#endif // YARF_RFUTILS_HPP
