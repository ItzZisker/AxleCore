#pragma once

#include "AX_Image.hpp"

#include <glm/glm.hpp>

namespace axle::gfx
{
    float BRDF_RadicalInverse_VdC(unsigned int bits);

    glm::vec2 BRDF_Hammersley(unsigned int i, unsigned int N);
    glm::vec3 BRDF_ImportanceSampleGGX(glm::vec2 Xi, float roughness, glm::vec3 N);

    float BRDF_GeometrySchlickGGX(float NdotV, float roughness);
    float BRDF_GeometrySmith(float roughness, float NoV, float NoL);

    glm::vec2 BRDF_IntegrateBRDF_RG(float NdotV, float roughness, unsigned int samples);

    void BRDFLUT_Generate(Image& img, int samples, int size);
};