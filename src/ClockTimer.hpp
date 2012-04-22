/**
 * A class for timing
 */
#ifndef YARF_CLOCKTIMER_HPP
#define YARF_CLOCKTIMER_HPP

#include <ctime>
#include <list>
#include <string>
#include <utility>

class ClockTimer
{
public:
    typedef std::pair<std::string, std::clock_t> ClockTime;
    typedef std::list<ClockTime> ClockTimes;

    ClockTimer():
        m_start(std::clock()) {
    }

    void time(const std::string s = "") {
        std::clock_t t = std::clock() - m_start;
        m_times.push_back(ClockTime(s, t / (CLOCKS_PER_SEC / 1000)));
    }

    const ClockTimes& getTimes() const {
        return m_times;
    }

private:
    std::clock_t m_start;
    ClockTimes m_times;
};


#endif // YARF_CLOCKTIMER_HPP
