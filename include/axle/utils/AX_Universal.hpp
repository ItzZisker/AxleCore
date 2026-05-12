#pragma once

#include "axle/utils/AX_Types.hpp"

#ifdef __AX_ASSETS_ASSIMP__
#include "glm/glm.hpp"
#include <assimp/matrix4x4.h>
#endif

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

class ScopeCall {
private:
    std::function<void()> m_Func;
public:
    ScopeCall(std::function<void()> func) : m_Func(func) {}
    ~ScopeCall() { m_Func(); }

    void Call() const { m_Func(); }
};

inline void HashCombine(size_t& h, size_t v) {
    h ^= v + 0x9e3779b97f4a7c15ull + (h << 6) + (h >> 2);
}

template<typename T>
inline void HashValue(size_t& h, const T& v) {
    HashCombine(h, std::hash<T>{}(v));
}

template<typename E>
inline void HashEnum(size_t& h, E v) {
    using U = std::underlying_type_t<E>;
    HashCombine(h, std::hash<U>{}(static_cast<U>(v)));
}

void Uni_NanoSleep(ChNanos nanos);

unsigned int Decode85Byte(char c);
void Decode85(const unsigned char* src, unsigned char* dst, std::size_t size);

#ifdef __AX_ASSETS_ASSIMP__
    glm::mat4 Assimp_ToGLM(const aiMatrix4x4& aiMat);
#endif

}