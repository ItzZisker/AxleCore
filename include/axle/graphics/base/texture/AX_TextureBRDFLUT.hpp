#pragma once

#include "AX_Texture.hpp"

namespace axle::graphics
{
    void SG_generateBRDFLUT(TextureImage& img, int samples = 128, int size = 1024);
};