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