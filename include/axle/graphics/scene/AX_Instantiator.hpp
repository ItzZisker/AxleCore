#pragma once

namespace axle::scene
{

class ModelInstantiator {
private:
    core::ThreadContextGfx m_GfxThread;
    std::mutex m_Mutex;
public:
    ModelInstantiator();
};

}