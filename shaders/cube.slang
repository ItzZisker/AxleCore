#pragma pack_matrix(column_major)

// UBO definition for transformation matrices (Bound to register b0)
cbuffer PerFrameData : register(b0) {
    float4x4 u_modelMatrix;
    float4x4 u_viewMatrix;
    float4x4 u_projectionMatrix;
};

// --- Vertex Input ---
struct VertexInput {
    float3 position; 
    float3 normal;   
    float2 texCoord; 
};

// --- Vertex Output / Fragment Input ---
struct VSOutput {
    float3 worldPos : TEXCOORD0;
    float3 normal   : TEXCOORD1;
    float2 uv       : TEXCOORD2;
    float4 clipPos  : SV_Position;
};

[shader("vertex")]
VSOutput mainVertex(VertexInput input)
{
	float4 worldPos = mul(float4(input.position, 1.0f), u_modelMatrix);
	float4 viewPos  = mul(worldPos, u_viewMatrix);

    VSOutput output;
    output.clipPos = mul(viewPos, u_projectionMatrix);
    output.worldPos = worldPos.xyz;
    output.normal = input.normal;
    output.uv = input.texCoord;
   
    return output;
}

float HueToRGB(float p, float q, float t) {
    if (t < 0.0f) t += 1.0f;
    if (t > 1.0f) t -= 1.0f;

    if (t < 1.0f/6.0f) return p + (q - p) * 6.0f * t;
    if (t < 1.0f/2.0f) return q;
    if (t < 2.0f/3.0f) return p + (q - p) * (2.0f/3.0f - t) * 6.0f;

    return p;
}

float3 HSLToRGB(float H, float S, float L) {
    float3 rgb;

    if (S == 0.0f) {
        rgb = float3(L, L, L);
    } else {
        float q = (L < 0.5f) ? (L * (1.0f + S)) : (L + S - L * S);
        float p = 2.0f * L - q;

        rgb.r = HueToRGB(p, q, H + 1.0f/3.0f);
        rgb.g = HueToRGB(p, q, H);
        rgb.b = HueToRGB(p, q, H - 1.0f/3.0f);
    }
    return rgb;
}

// --- Fragment Shader Stage ---
[shader("fragment")]
float4 mainFragment(VSOutput input) : SV_Target0 {
    float3 p = (input.worldPos + 1.0f) / 2.0f;

    float hue = dot(p, float3(1.0f, 1.0f, 1.0f)) / 3.0f;
    hue = saturate(hue);

    float saturation = 1.0f;
    float lightness = 0.5f;

    float3 color = HSLToRGB(hue, saturation, lightness);

    return float4(color, 1.0f);
}
