// -----------------------------------------------------------------------------
// AUTO-GENERATED FILE. DO NOT EDIT.
// -----------------------------------------------------------------------------

#pragma once

#include <string_view>

namespace axle::eshdr
{

struct EmbeddedShader
{
    std::string_view moduleName;
    std::string_view source;
};

inline constexpr std::string_view engine_transform = R"AXLE_SHADER(
module engine.transform

float4x4 GetModelTransform();
)AXLE_SHADER";

inline constexpr std::string_view engine_transform_gpu_driven = R"AXLE_SHADER(
StructuredBuffer<float4x4> g_InstanceMatrices;

float4x4 GetModelTransform()
{
    return g_InstanceMatrices[SV_InstanceID];
}
)AXLE_SHADER";

inline constexpr std::string_view engine_transform_instanced = R"AXLE_SHADER(
struct InstanceInput
{
    float4 row0 : INSTANCE_ROW0;
    float4 row1 : INSTANCE_ROW1;
    float4 row2 : INSTANCE_ROW2;
    float4 row3 : INSTANCE_ROW3;
};

static InstanceInput g_instance;

float4x4 GetModelTransform()
{
    return float4x4(
        g_instance.row0,
        g_instance.row1,
        g_instance.row2,
        g_instance.row3
    );
}
)AXLE_SHADER";

inline constexpr std::string_view engine_transform_uniform = R"AXLE_SHADER(
cbuffer PerObject : register(b1)
{
    float4x4 u_modelMatrix;
};

float4x4 GetModelTransform()
{
    return u_modelMatrix;
}
)AXLE_SHADER";

inline constexpr EmbeddedShader kShaders[] =
{
    { "engine/transform", engine_transform },
    { "engine/transform_gpu_driven", engine_transform_gpu_driven },
    { "engine/transform_instanced", engine_transform_instanced },
    { "engine/transform_uniform", engine_transform_uniform },
};

} // namespace axle::fw::generated
