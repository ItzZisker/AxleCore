#pragma once

#include "axle/core/concurrency/AX_ThreadCycler.hpp"

namespace axle::gfx
{

class RenderProcedure : AX_THR_RENDER_OWNED {
private:
    
public:
    RenderProcedure();
    ~RenderProcedure();
};

}