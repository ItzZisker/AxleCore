#pragma once

#include "AX_DataSerializer.hpp"
#include "AX_DataDeserializer.hpp"

#include <cstring>

namespace axle::data
{
    template<typename T>
    void LE_Write(DataSerializer* buff, T value);

    template<typename T>
    T LE_Read(DataDeserializer* buff);
}