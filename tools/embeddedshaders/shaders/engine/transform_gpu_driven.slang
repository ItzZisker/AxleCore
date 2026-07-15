StructuredBuffer<float4x4> g_InstanceMatrices;

float4x4 GetModelTransform()
{
    return g_InstanceMatrices[SV_InstanceID];
}