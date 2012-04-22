/**
 * Handle input and output of data
 */
#ifndef YARF_DATAIO_HPP
#define YARF_DATAIO_HPP

#include <string>
#include <fstream>
#include <sstream>
#include <vector>
#include "refcountptr.hpp"
#include "RFtypes.hpp"
#include "Logger.hpp"

class Tokeniser
{
public:
    /**
     * Create a string tokeniser
     * delims: List of single character delimiters
     * condense: If true consecutive delimiters are treated as a single
     *           delimiter and leading/trailing delimiters are skipped.
     *           If false consecutive delimiters, and leading/trailing
     *           delimiters, will result in empty strings being returned.
     */
    //Tokeniser(const std::string delims, bool incDelims = false):
    Tokeniser(const std::string delims, bool condense):
        m_delims(delims), m_condense(condense) {
        set("");
    }

    /**
     * Set the string to be tokenised
     */
    void set(const std::string s) {
        m_s = s;
        m_p = 0;
        m_q = 0;
        m_taken = true;
    }

    /**
     * Are there any tokens left?
     */
    bool hasNext() {
        if (m_taken) {
            findNext();
        }
        return m_p != std::string::npos;
    }

    /**
     * Return the next token
     */
    std::string next() {
        bool b = hasNext();
        assert(b);

        if (b) {
            m_taken = true;
            if (m_q == std::string::npos) {
                return m_s.substr(m_p);
            }
            else {
                return m_s.substr(m_p, m_q - m_p);
            }
        }
        else {
            // TODO: throw an error
            return "";
        }
    }

    /**
     * Convert a string into type T using the << operator
     */
    template <typename T>
    static T convert(const std::string s) {
        std::istringstream iss(s);
        T x;
        iss >> x;
        // TODO: check whether conversion succeeded
        return x;
    }

    /**
     * Convert an object into a string using the >> operator
     */
    template <typename T>
    static std::string toString(const T& x) {
        std::ostringstream oss;
        oss << x;
        // TODO: check whether conversion succeeded
        return oss.str();
    }

private:
    /**
     * Find the next token
     * m_q points to the previous ending delimiter, apart from the first token
     */
    void findNext() {
        m_taken = false;

        if (m_p == std::string::npos) {
            return;
        }

        if (m_q == std::string::npos) {
            m_p = std::string::npos;
            return;
        }

        if (m_q == 0) {
            // First time
            m_p = 0;
        }
        else {
            m_p = m_q + 1;
        }

        if (m_condense) {
            // Skip all delimiters
            m_p = m_s.find_first_not_of(m_delims, m_p);
        }
        if (m_p == std::string::npos) {
            return;
        }

        m_q = m_s.find_first_of(m_delims, m_p);
        // s[m_p..m_q-1] is the next token
        // Note that this intentionally returns an empty first token if
        // !m_condense and m_p == 0
    }

    /**
     * List of delimiting characters
     */
    const std::string m_delims;

    /**
     * Whether to treat consecutive delimiters as a single delimiter one or not
     */
    bool m_condense;

    /**
     * Whether to return delimiters as tokens or not
     */
    //const bool m_incDelims;

    /**
     * Current string
     */
    std::string m_s;

    /**
     * Index of the next token
     */
    uint m_p;

    /**
     * Index of one past the end of the next token
     */
    uint m_q;

    /**
     * Has the last token been taken yet?
     */
    bool m_taken;
};


/**
 * Read a numeric CSV file, no escapes, no header
 */
class CsvReader
{
public:
    /**
     * Read a csv file
     */
    CsvReader():
        m_tok(",", true), m_cols(0) {
    }

    /**
     * Parse a CSV file
     * file: The CSV file name
     */
    bool parse(const char file[]) {
        std::ifstream is(file);
        if (!is) {
            return false;
        }
        return parse(is);
    }

    /**
     * Parse a CSV file provided as an istream object
     * is: The input stream
     */
    bool parse(std::istream& is) {
        std::string line;

        while(getline(is, line)) {
            DoubleArrayPtr x = new DoubleArray;
            parseLine(line, *x);

            if (m_xs.empty()) {
                m_cols = x->size();
                if (m_cols < 1) {
                    LOG(Log::ERROR) << "Line " << m_xs.size() << " was empty";
                    return false;
                }
            }
            else if (x->size() != m_cols) {
                LOG(Log::ERROR) << "Line " << m_xs.size() << " expected "
                                << m_cols << " tokens, found " << x->size();
                return false;
            }

            m_xs.push_back(x);
        }

        return true;
    }

    /**
     * Return the number of rows
     */
    uint rows() const {
        return m_xs.size();
    }

    /**
     * Return the number of columns
     */
    uint cols() const {
        return m_cols;
    }

    /**
     * Get an element
     * r: Row index
     * c: Column index
     */
    double operator()(uint r, uint c) const {
        return (*m_xs[r])[c];
    }

protected:
    typedef RefCountPtr<DoubleArray> DoubleArrayPtr;

    /**
     * Split a single line into tokens around commas
     * line: A line from the CSV file
     * x: Vector to which the tokens will be appended
     */
    void parseLine(const std::string& line, std::vector<double>& x) {
        m_tok.set(line);
        while (m_tok.hasNext()) {
            x.push_back(Tokeniser::convert<double>(m_tok.next()));
        }
    }

private:
    /**
     * The tokeniser
     */
    Tokeniser m_tok;

    /**
     * The number of columns
     */
    uint m_cols;

    /**
     * The matrix of csv values
     */
    std::vector<DoubleArrayPtr> m_xs;
};


#endif // YARF_DATAIO_HPP
