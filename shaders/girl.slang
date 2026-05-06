#pragma pack_matrix(column_major)

Sampler2D g_albedoMap : register(s0);
Sampler2D g_normalMap : register(s1);

// UBO definition for transformation matrices (Bound to register b0)
cbuffer PerFrameData : register(b0) {
    float4x4 u_modelMatrix;
    float4x4 u_viewMatrix;
    float4x4 u_projectionMatrix;

    float3 u_lightColor;
    float3 u_cameraPos;
    float3 u_lightDir;

    bool u_useNormalMap;
};

// --- Vertex Input ---
struct VertexInput {
    float3 position  : POSITION;
    float3 normal    : NORMAL;
    float2 texCoord  : TEXCOORD0;
    float3 tangent   : TANGENT;
    float3 bitangent : BINORMAL;
};

// --- Vertex Output / Fragment Input ---
struct VSOutput {
    float3 worldPos : TEXCOORD0;
    float3 normal   : TEXCOORD1;
    float2 uv       : TEXCOORD2;
    float3x3 TBN    : TEXCOORD3;
    float3 Bdummy   : TEXCOORD4;
    float4 clipPos  : SV_Position;
};

[shader("vertex")]
VSOutput mainVertex(VertexInput input)
{
    // Use matrix * vector form for column-major matrices
    float4 worldPos = mul(float4(input.position, 1.0f), u_modelMatrix);
    float4 viewPos  = mul(worldPos, u_viewMatrix);
    float3 Bdummy = input.bitangent * 0.0;

    // Transform tangent & normal to world space using the 3x3 part
    float3 T = normalize(mul(input.tangent, (float3x3)u_modelMatrix));
    float3 N = normalize(mul(input.normal, (float3x3)u_modelMatrix));
    T = normalize(T - dot(T, N) * N);  // orthogonalize T to N
    float3 B = cross(N, T);

    VSOutput output;
    output.clipPos  = mul(viewPos, u_projectionMatrix);
    output.worldPos = worldPos.xyz;
    output.normal   = N;               // world-space normal
    output.uv       = input.texCoord;
    output.TBN      = float3x3(T, B, N);
    output.Bdummy   = Bdummy;

    return output;
}

float calc_Specular(float3 normal, float3 halfwayDir, float2 uv) {
    // float shininess = sampleShininess(uv);
    float shininess = 32.0f;
    return (( 8.0 + shininess ) / ( 8.0 * 3.14159265 )) * pow(max(dot(normal, halfwayDir), 0.0), shininess);
}

float3 calc_DirLightColor(float3 lightDir, float3 normal, float3 viewDir, float2 uv) {
    float3 halfwayDir = normalize(lightDir + viewDir);

    float diff = max(dot(normal, lightDir), 0.0f);

    float3 albedo = g_albedoMap.Sample(uv).xyz;

    float3 lightColorAmbient = float3(0.16f, 0.16f, 0.16f);
    float3 lightColorDiffuse = float3(0.57f, 0.57f, 0.39f);
    float3 lightColorSpecular = float3(0.74f, 0.74f, 0.74f);
    
    float3 ambient = lightColorAmbient * albedo;
    float3 diffuse = lightColorDiffuse * diff * albedo;
    // float3 specular = lightColorSpecular * calc_Specular(normal, halfwayDir, uv) * 0.5f;

    // return (ambient + diffuse + specular);
    return (ambient + diffuse);
}

// --- Fragment Shader Stage ---
[shader("fragment")]
float4 mainFragment(VSOutput input) : SV_Target0 {
    float2 uv = input.uv;
    float3 normal;
    float3 Bdummy;

    if (u_useNormalMap) {
        normal = g_normalMap.Sample(uv).xyz;
        normal = normal * 2.0 - 1.0;
        normal = normalize(mul(normal, input.TBN));
    } else {
        normal = normalize(input.normal);
    }
    Bdummy = input.Bdummy;

    float3 dirLight = normalize(float3(1.0f, 0.5f, 1.0f));
    float3 viewDir  = normalize(u_cameraPos - input.worldPos);

    float3 color       = calc_DirLightColor(dirLight, normal, viewDir, uv);
    float  albedoAlpha = g_albedoMap.Sample(uv).a;

    return float4(color, albedoAlpha);
}