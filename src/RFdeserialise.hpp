/**
 * Deserialisation methods for the random forest
 */
#ifndef YARF_RFDESERIALISE_HPP
#define YARF_RFDESERIALISE_HPP

#include <sstream>
#include "RFtypes.hpp"
#include "DataIO.hpp"

#include "RFnode.hpp"
#include "RFtree.hpp"
#include "RFsplit.hpp"



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


class RFbuilder
{
public:
    typedef Deserialiser D;

    RFbuilder(D& deserialiser):
        m_deserialiser(deserialiser) {
    }

    D::Token nextToken() {
        return next();
    }

    RFparameters* dRFparameters(D::Token t) {
        check(t.type == D::ObjectStart && t.object == "RFparameters");
        RFparameters* obj = new RFparameters();

        while (true) {
            t = next();

            if (t.tag == "numTrees" && t.type == D::Scalar) {
                set(obj->numTrees, t.value);
            }
            else if (t.tag == "numSplitFeatures" && t.type == D::Scalar) {
                set(obj->numSplitFeatures, t.value);
            }
            else if (t.tag == "minScore" && t.type == D::Scalar) {
                set(obj->minScore, t.value);
            }
            else if (t.type == D::ObjectEnd && t.object == "RFparameters") {
                break;
            }
            else {
                // error
                LOG(Log::ERROR) << "Unexpected token: "
                                << Deserialiser::toString(t);
                assert(false);
            }
        }

        return obj;
    }

    RFforest* dRFforest(D::Token t) {
        check(t.type == D::ObjectStart && t.object == "RFforest");
        RFforest* obj = new RFforest();

        while (true) {
            t = next();

            if (t.tag == "data" && t.type == D::EmptyArray) {
                // TODO
                obj->m_data = NULL;
            }
            else if (t.tag == "params" && t.type == D::ObjectStart) {
                obj->m_params = dRFparameters(t);
            }
            else if(t.tag == "trees" && t.type == D::ObjectArray) {
                obj->m_trees.resize(t.n);
                for (uint n = 0; n < t.n; ++n) {
                    obj->m_trees[n] = dRFtree(next());
                }
            }
            else if (t.type == D::ObjectEnd && t.object == "RFforest") {
                break;
            }
            else {
                // error
                LOG(Log::ERROR) << "Unexpected token: "
                                << Deserialiser::toString(t);
                assert(false);
            }
        }

        return obj;
    }

    RFtree* dRFtree(D::Token t) {
        check(t.type == D::ObjectStart && t.object == "RFtree");
        RFtree* obj = new RFtree();

        while (true) {
            t = next();

            if (t.tag == "data" && t.type == D::EmptyArray) {
                // TODO
                obj->m_data = NULL;
            }
            else if (t.tag == "params" && t.type == D::ObjectStart) {
                obj->m_params = dRFparameters(t);
            }
            else if(t.tag == "ids" && t.type == D::NumericArray) {
                set(obj->m_ids, t.value);
            }
            else if(t.tag == "bag" && t.type == D::NumericArray) {
                set(obj->m_bag, t.value);
            }
            else if(t.tag == "oob" && t.type == D::NumericArray) {
                set(obj->m_oob, t.value);
            }
            else if (t.tag == "root" && t.type == D::ObjectStart) {
                obj->m_root = dRFnode(t);
            }
            else if (t.type == D::ObjectEnd && t.object == "RFtree") {
                break;
            }
            else {
                // error
                LOG(Log::ERROR) << "Unexpected token: "
                                << Deserialiser::toString(t);
                assert(false);
            }
        }

        return obj;
    }

    RFnode* dRFnode(D::Token t) {
        check(t.type == D::ObjectStart && t.object == "RFnode");
        RFnode* obj = new RFnode();

        while (true) {
            t = next();

            if (t.tag == "Left" && t.type == D::ObjectStart) {
                obj->m_left = dRFnode(t);
            }
            else if (t.tag == "Right" && t.type == D::ObjectStart) {
                obj->m_right = dRFnode(t);
            }
            else if (t.tag == "counts" && t.type == D::NumericArray) {
                set(obj->m_counts, t.value);
            }
            else if (t.tag == "n" && t.type == D::Scalar) {
                set(obj->m_n, t.value);
            }
            else if (t.tag == "split" && t.type == D::ObjectStart) {
                obj->m_split = dSplitSelector(t);
            }
            else if (t.tag == "depth" && t.type == D::Scalar) {
                set(obj->m_depth, t.value);
            }
            else if (t.type == D::ObjectEnd && t.object == "RFnode") {
                break;
            }
            else {
                // error
                LOG(Log::ERROR) << "Unexpected token: "
                                << Deserialiser::toString(t);
                assert(false);
            }
        }

        return obj;
    }

    SplitSelector* dSplitSelector(D::Token t) {
        check(t.type == D::ObjectStart);

        if (t.object == "MaxInfoGainSplit") {
            return dMaxInfoGainSplit(t);
        }
        else {
            //error
            assert(false);
        }
    }

    MaxInfoGainSplit* dMaxInfoGainSplit(D::Token t) {
        check(t.type == D::ObjectStart && t.object == "MaxInfoGainSplit");
        MaxInfoGainSplit* obj = new MaxInfoGainSplit();

        while (true) {
            t = next();

            if (t.tag == "counts" && t.type == D::NumericArray) {
                set(obj->m_counts, t.value);
            }
            else if (t.tag == "gotSplit" && t.type == D::Scalar) {
                set(obj->m_gotSplit, t.value);
            }
            else if (t.tag == "bestft" && t.type == D::Scalar) {
                set(obj->m_bestft, t.value);
            }
            else if (t.tag == "split" && t.type == D::EmptyArray) {
                // m_split should default to empty
            }
            else if (t.tag == "split" && t.type == D::ObjectArray) {
                obj->m_splits.resize(t.n);
                for (uint n = 0; n < t.n; ++n) {
                    obj->m_splits[n] = dMaxInfoGainSingleSplit(next());
                }
            }
            else if (t.type == D::ObjectEnd && t.object == "MaxInfoGainSplit") {
                break;
            }
            else {
                // error
                LOG(Log::ERROR) << "Unexpected token: "
                                << Deserialiser::toString(t);
                assert(false);
            }
        }

        return obj;
    }

    MaxInfoGainSingleSplit* dMaxInfoGainSingleSplit(D::Token t) {
        check(t.type == D::ObjectStart && t.object == "MaxInfoGainSingleSplit");
        MaxInfoGainSingleSplit* obj = new MaxInfoGainSingleSplit();

        while (true) {
            t = next();

            if (t.tag == "ids" && t.type == D::NumericArray) {
                set(obj->m_ids, t.value);
            }
            else if (t.tag == "ftid" && t.type == D::Scalar) {
                set(obj->m_ftid, t.value);
            }
            else if (t.tag == "perm" && t.type == D::NumericArray) {
                set(obj->m_perm, t.value);
            }
            else if (t.tag == "counts" && t.type == D::NumericArray) {
                set(obj->m_counts, t.value);
            }
            else if (t.tag == "ig" && t.type == D::NumericArray) {
                set(obj->m_ig, t.value);
            }
            else if (t.tag == "splitpos" && t.type == D::Scalar) {
                set(obj->m_splitpos, t.value);
            }
            else if (t.tag == "splitval" && t.type == D::Scalar) {
                set(obj->m_splitval, t.value);
            }
            else if (t.type == D::ObjectEnd &&
                     t.object == "MaxInfoGainSingleSplit") {
                break;
            }
            else {
                // error
                LOG(Log::ERROR) << "Unexpected token: "
                                << Deserialiser::toString(t);
                assert(false);
            }
        }

        return obj;
    }

protected:
    template <typename T>
    static void set(T& x, const std::string s) {
        x = Utils::convert<T>(s);
    }

    template <typename T>
    static void set(std::vector<T>& xs, std::string s) {
        xs.clear();
        Tokeniser tok(",", false);
        tok.set(s);

        while (tok.hasNext()) {
            xs.push_back(Utils::convert<T>(tok.next()));
        }
    }

    // TODO
    static void check(bool b) {
        if (!b) {
            LOG(Log::ERROR) << "RFbuilder.check() failed";
        }
        assert(b);
    }

    D::Token next() {
        D::Token t = m_deserialiser.next();
        LOG(Log::DEBUG2) << Deserialiser::toString(t);

        check(t.type != D::ParseError);
        return t;
    }

private:
    Deserialiser m_deserialiser;
};



#endif // YARF_RFDESERIALISE_HPP
