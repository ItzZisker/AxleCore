#pragma once

#include "axle/utils/AX_Types.hpp"

#include <vector>
#include <type_traits>
#include <algorithm>
#include <bit>

/*  
    RULES FOR A "GOOD" COMPARATOR:

    1. If comp(a, b) is true, then comp(b, a) must be false.
    (You can’t say both “a comes before b” and “b comes before a”.)

    2. comp(a, a) should always be false.
    (An element is not before itself.)

    3. Be consistent: if you sort by (stage, sortKey), always compare in that order, don’t mix.
*/

namespace axle::utils
{

using MagicId = uint32_t;

struct MagicHandle {
public:
    MagicId index{0}, generation{0};
};

template <typename Tag>
struct MagicHandleTagged : public MagicHandle {};

template <typename T_Extern>
requires std::is_base_of_v<MagicHandle, T_Extern>
struct MagicInternal {
    using handle_type = T_Extern;

    MagicId index{0}, generation{0};
    bool alive{false};

    T_Extern External() {
        T_Extern res;
        res.index = index;
        res.generation = generation;
        return res;
    }

    void Sign() { alive = true; }
    void UnSign() { alive = false; }
};

template <typename T_Intern>
requires (std::is_base_of_v<MagicInternal<typename T_Intern::handle_type>, T_Intern>)
class MagicPool {
private:
    std::vector<T_Intern> m_Handles;

    std::vector<MagicId> m_Free;
    std::vector<MagicId> m_Order;

    bool m_Ordered{false};
public:
    using T_Extern = typename T_Intern::handle_type;

    MagicPool(bool ordered = false) : m_Ordered(ordered) {}

    bool IsValid(MagicId index, MagicId generation) const {
        if (index >= m_Handles.size())
            return false;

        const auto& internal = m_Handles[index];
        return internal.alive && internal.generation == generation;
    }

    bool IsValid(const T_Extern& handle) const {
        return IsValid(handle.index, handle.generation);
    }

    bool IsEqual(const T_Extern& a, const T_Extern& b) const {
        return a.index == b.index && a.generation == b.generation;
    }

    T_Intern* Get(const T_Extern& handle) {
        if (!IsValid(handle))
            return nullptr;
        return &m_Handles[handle.index];
    }

    T_Intern& GetRaw(MagicId idx) {
        return m_Handles[idx];
    }

    const T_Intern* GetConst(const T_Extern& handle) const {
        Get(handle);
    }

    const std::vector<T_Intern>& GetInternal() const {
        return m_Handles;
    }

    T_Intern* Reserve(const MagicId after = UINT32_MAX) {
        MagicId index;
        if (!m_Free.empty()) {
            index = m_Free.back();
            m_Free.pop_back();
        } else {
            index = static_cast<MagicId>(m_Handles.size());
            m_Handles.emplace_back();
            m_Handles.back().index = index;
        }
        if (m_Ordered) {
            if (after == UINT32_MAX) {
                m_Order.push_back(index);
            } else {
                auto it = std::find(m_Order.begin(), m_Order.end(), index);
                m_Order.insert(it == m_Order.end() ? m_Order.end() : std::next(it), index);
            }
        }
        return &m_Handles[index];
    }

    bool Delete(const T_Extern& handle) {
        if (!IsValid(handle))
            return false;
        
        auto& internal = m_Handles[handle.index];
        if (!internal.alive)
            return false;        
        internal.UnSign();
        internal.generation++;

        m_Free.push_back(handle.index);
        
        if (m_Ordered) {
            auto it = std::remove(m_Order.begin(), m_Order.end(), handle.index);
            m_Order.erase(it, m_Order.end());
        }
        return true;
    }

    const std::vector<MagicId>& GetOrder() const {
        return m_Order;
    }

    void ModifyOrder(std::function<void(std::vector<MagicId>&)> modify_func) {
        if (!m_Ordered) return;
        modify_func(m_Order);
    }

    void SortOrder(Predicate<T_Intern> cmp) {
        if (!m_Ordered) return;
        std::sort(
            m_Order.begin(),
            m_Order.end(),
            [&](MagicId a, MagicId b) {
                return cmp(m_Handles[a], m_Handles[b]);
            }
        );
    }

    void SortOrder(Predicate<T_Extern> cmp) {
        if (!m_Ordered) return;
        std::sort(
            m_Order.begin(),
            m_Order.end(),
            [&](MagicId a, MagicId b) {
                return cmp(m_Handles[a].External(), m_Handles[b].External());
            }
        );
    }
};

}