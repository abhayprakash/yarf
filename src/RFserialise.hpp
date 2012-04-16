/**
 * Serialisation methods for the random forest
 */
#ifndef YARF_RFSERIALISE_HPP
#define YARF_RFSERIALISE_HPP

#include <sstream>
#include "RFtypes.hpp"
#include "DataIO.hpp"


inline std::string in(uint n) {
    return std::string(2 * n, ' ');
}

inline std::string indent(uint n, char c = ' ') {
    return std::string(n, c);
}

template<typename ContainerT>
std::string arrayToString(const ContainerT& xs, bool printSize = true,
                          uint p1 = 0, uint p2 = -1)
{
    std::ostringstream oss;

    p2 = std::min(xs.size(), p2);
    if (printSize) {
        oss << "[" << p2 - p1 << "] ";
    }

    for (uint p = p1; p < p2; ++p)
    {
        if (p > p1)
        {
            oss << ",";
        }
        oss << xs[p];
    }

    return oss.str();
}


class Deserialiser
{
public:
    enum Type {
        ParseError,
        Scalar,
        EmptyArray,
        NumericArray,
        ObjectArray,
        ObjectStart,
        ObjectEnd
    };

    struct Token
    {
        Type type;
        std::string tag;
        uint n;
        std::string value;
        std::string object;
    };

    static std::string toString(const Token& t) {
        std::ostringstream oss;
        switch (t.type) {
        case ParseError:
            oss << "ParseError";
            break;
        case Scalar:
            oss << "Scalar";
            break;
        case EmptyArray:
            oss << "EmptyArray";
            break;
        case NumericArray:
            oss << "NumericArray";
            break;
        case ObjectArray:
            oss << "ObjectArray";
            break;
        case ObjectStart:
            oss << "ObjectStart";
            break;
        case ObjectEnd:
            oss << "ObjectEnd";
            break;
        default:
            oss << "UNKNOWN-TYPE";
        }

        oss << " : "
            << t.tag << " : "
            << t.n << " : "
            << t.value << " : "
            << t.object;

        return oss.str();
    }

    Deserialiser(std::istream& is):
        m_is(is), m_count(0) {
    }

    const Token& next() {
        resetToken(m_tok);
        std::string s = read();
        if (s.empty()) {
            // Failed
            return m_tok;
        }

        if (isTagName(s)) {
            m_tok.tag = s;
            s = read();

            if (s.empty()) {
                // Failed
                return m_tok;
            }
        }

        if (isObjectStart(s)) {
            m_tok.type = ObjectStart;
            m_tok.object = parseObjectStart(s);
        }
        else if (isObjectEnd(s)) {
            m_tok.type = ObjectEnd;
            m_tok.object = parseObjectEnd(s);
        }
        else if (isArraySize(s)) {
            m_tok.n = parseArraySize(s);
            if (m_tok.n == 0) {
                m_tok.type = EmptyArray;
            }
            else {
                s = read();
                if (isObjectStart(s)) {
                    m_tok.type = ObjectArray;
                    unread(s);
                }
                else if (isNumericArray(s)) {
                    m_tok.type = NumericArray;
                    m_tok.value = s;
                }
                else {
                    fail("Unknown array type", s);
                }
            }
        }
        else {
            m_tok.type = Scalar;
            m_tok.value = s;
        }

        return m_tok;
    }

protected:
    bool isTagName(std::string s) {
        return isAlpha(s);
    }

    bool isObjectStart(std::string s) {
        return (s[s.size() - 1] == '{') && isAlpha(s.substr(0, s.size() - 1));
    }

    std::string parseObjectStart(std::string s) {
        return s.substr(0, s.size() - 1);
    }

    bool isObjectEnd(std::string s) {
        return (s[0] == '}') && isAlpha(s.substr(1, s.size() - 1));
    }

    std::string parseObjectEnd(std::string s) {
        return s.substr(1, s.size() - 1);
    }

    bool isArraySize(std::string s) {
        return (s.size() >= 3) && (s[0] == '[') && (s[s.size() - 1] == ']') &&
            isUint(s.substr(1, s.size() - 2));
    }

    uint parseArraySize(std::string s) {
        return Tokeniser::convert<uint>(s.substr(1, s.size() - 2));
    }

    bool isAlpha(std::string s) {
        return s.find_first_not_of(
            "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
            "abcdefghijklmnopqrstuvwxyz"
            ) == std::string::npos;
    }

    bool isNumericArray(std::string s) {
        return s.find_first_not_of(
            "0123456789"
            ".-e,"
            ) == std::string::npos;
    }

    bool isUint(std::string s) {
        return s.find_first_not_of(
            "0123456789"
            ) == std::string::npos;
    }

    void parseNumericArray(std::string s) {
    }


    void resetToken(Token& tok) const {
        tok.type = ParseError;
        tok.tag = "";
        tok.n = 0;
        tok.value = "";
        tok.object = "";
    }

    void fail(std::string msg, std::string tok = "") {
        resetToken(m_tok);
        m_tok.tag = "ERROR near token number " +
            Tokeniser::toString(m_count) +
            " : " + msg +
            " :\"" + tok + "\"";
    }

    std::string read() {
        std::string s;

        if (!m_last.empty()) {
            s = m_last;
            m_last = "";
            return s;
        }

        m_is >> s;
        if (!m_is) {
            fail("Read failed");
        }

        ++m_count;
        //assert(!s.empty());

        return s;
    }

    void unread(std::string s) {
        m_last = s;
    }

    std::istream& m_is;
    uint m_count;
    Token m_tok;
    std::string m_last;
};


#endif // YARF_RFSERIALISE_HPP
