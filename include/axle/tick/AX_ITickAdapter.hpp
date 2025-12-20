#pragma once

namespace axle::tick
{

class ITickAdapter {
public:
    virtual ~ITickAdapter() {};
    virtual void Tick(float dT) = 0;
};

}