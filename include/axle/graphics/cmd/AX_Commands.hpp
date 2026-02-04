#pragma once

#include "axle/utils/AX_Types.hpp"

#include <vector>
#include <type_traits>
#include <cstdint>
#include <atomic>

namespace axle::gfx {

enum class CmdType : uint16_t {
    Clear,
    SetViewport,
    SetPipeline,
    BindVertexBuffer,
    Draw
};

struct CmdHeader {
    CmdType type;
    uint16_t size; // total command size (including header)
};

struct ICmd {
    CmdHeader header;
};

class CommandBuffer {
    std::vector<uint8_t> m_Buffer;
    std::atomic_bool m_QueueClear;
public:
    void Reset() {
        m_Buffer.clear();
        m_QueueClear.store(true);
    }

    template<typename T>
    requires std::is_base_of_v<ICmd, T>
    T* Push(CmdType type) {
        static_assert(std::is_trivially_copyable_v<T>);
        m_QueueClear.store(false);

        size_t offset = m_Buffer.size();
        m_Buffer.resize(offset + sizeof(T));

        T* cmd = reinterpret_cast<T*>(&m_Buffer[offset]);
        cmd->header.type = type;
        cmd->header.size = sizeof(T);

        return cmd;
    }

    const uint8_t* Data() const { return m_Buffer.data(); }
    size_t Size() const { return m_Buffer.size(); }

    bool ShouldClearOnce() {
        bool shouldClear = m_QueueClear.load();
        if (shouldClear) m_QueueClear.store(false);
        return shouldClear;
    }
};

struct CmdClear : public ICmd {
    uint32_t clearMask;
    float clearColor[4];
};

struct CmdSetViewport : public ICmd {
    int x, y;
    int width, height;
};

struct CmdSetPipeline : public ICmd {
    PipelineHandle pipeline;
};

struct CmdBindVertexBuffer : public ICmd {
    BufferHandle buffer;
    uint32_t offset;
};

struct CmdDraw : public ICmd {
    uint32_t vertexCount;
    uint32_t firstVertex;
};


};