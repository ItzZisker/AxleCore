cbuffer PerObject : register(b1)
{
    float4x4 u_modelMatrix;
};

float4x4 GetModelTransform()
{
    return u_modelMatrix;
}