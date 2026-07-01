#pragma once

#include "axle/core/concurrency/AX_ThreadCycler.hpp"

namespace axle::scene
{

class Scene : AX_THR_RENDER_OWNED {
private:
    
public:
    Scene(ThreadGfxScope gfxThread);
    ~Scene();
};

}