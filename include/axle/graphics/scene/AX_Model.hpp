#pragma once

#include "axle/core/concurrency/AX_ThreadCycler.hpp"

#include "axle/graphics/AX_GraphicsParams.hpp"

#include "axle/utils/AX_Types.hpp"
#include "axle/utils/AX_Expected.hpp"
#include "axle/utils/AX_MagicPool.hpp"

namespace axle::scene
{

struct Model {

};

class ModelCreator {
private:
    SharedPtr<core::ThreadContextGfx> m_GfxThread;
    

    std::mutex m_Mutex{};
public:
    ModelManager(SharedPtr<core::ThreadContextGfx> gfxThread);

    utils::ExResult<Model> Create();
};

}