/**
 * Debug logging
 * Based on
 * http://www.drdobbs.com/article/print?articleId=201804215&dept_url=/cpp/
 */
#ifndef YARF_LOGGER_HPP
#define YARF_LOGGER_HPP

#include <ctime>
#include <iostream>
#include <sstream>
#include <string>

class Log
{
public:
    enum LogLevel {
        ERROR,
        WARNING,
        INFO,
        DEBUG1,
        DEBUG2
    };

    Log();
    virtual ~Log();
    std::ostringstream& get(LogLevel level = INFO);
public:
    static LogLevel& reportingLevel();
    static std::string toString(LogLevel level);
    std::string nowTime();

protected:
    std::ostringstream m_os;
private:
    Log(const Log&);
    Log& operator =(const Log&);
};

inline Log::Log()
{
}

inline std::ostringstream& Log::get(LogLevel level)
{
    //m_os << "- " << nowTime();
    m_os << toString(level) << ": ";
    //m_os << std::string(level > DEBUG1 ? level - DEBUG1 : 0, '\t');
    return m_os;
}

inline Log::~Log()
{
    std::cerr << m_os.str() << std::endl;
}

inline Log::LogLevel& Log::reportingLevel()
{
    static LogLevel level = DEBUG1;
    return level;
}

inline std::string Log::toString(Log::LogLevel level)
{
    static const char* const buffer[] = {"ERROR", "WARNING", "INFO",
                                         "DEBUG1", "DEBUG2"};
    return buffer[level];
}

inline std::string Log::nowTime()
{
    std::time_t now;
    std::time(&now);
    char s[9];
    strftime(s, 9, "%H:%M:%S", std::localtime(&now));
    return s;
}

#define LOG(level)                           \
    if (level > Log::reportingLevel()) ;     \
    else Log().get(level)


#endif // YARF_LOGGER_HPP

