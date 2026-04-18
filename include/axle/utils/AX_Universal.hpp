#pragma once

#include "axle/utils/AX_Types.hpp"

namespace axle::utils
{

template <typename T>
class Range {
private:
    T m_From;
    T m_To;
public:
    Range(T from, T to) : m_From(from), m_To(to) {}

    T From() const { return m_From; }
    T To() const { return m_To; }

    bool IsIn(const T& t) const {
        return m_From <= t && t <= m_To;
    }

    bool IsBetween(const T& t) const {
        return m_From < t && t < m_To;
    }
};

void Uni_NanoSleep(ChNanos nanos);

}