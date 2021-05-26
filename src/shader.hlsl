//--------------------------------------------------------------------------------------
// Constant Buffer Variables
//--------------------------------------------------------------------------------------
cbuffer ConstantBuffer: register(b0) {
    matrix World;
    matrix View;
    matrix Projection;
}

//--------------------------------------------------------------------------------------
struct VS_OUTPUT {
    float4 Pos: SV_POSITION;
    float2 TexCoord: TEXCOORD0;
};

Texture2D < uint4 > g_texture[2];
SamplerState g_samLinear;
//--------------------------------------------------------------------------------------
// Vertex Shader
//--------------------------------------------------------------------------------------
VS_OUTPUT VS(float4 Pos: POSITION, float2 TexCoord: TEXCOORD0) {
    VS_OUTPUT output = (VS_OUTPUT) 0;
    output.Pos = Pos;
    output.TexCoord = TexCoord.xy;
    return output;
}

float4 YUV2RGB(float4 YUVA) {
    float C = (YUVA.r * 256) - 16;
    float D = (YUVA.g * 256) - 128;
    float E = (YUVA.b * 256) - 128;

    float R = clamp((298 * C + 409 * E + 128) / 256, 0, 256);
    float G = clamp((298 * C - 100 * D - 208 * E + 128) / 256, 0, 256);
    float B = clamp((298 * C + 516 * D + 128) / 256, 0, 256);

    return float4(R / 256, G / 256, B / 256, YUVA.a);
}

//--------------------------------------------------------------------------------------
// Pixel Shader
//--------------------------------------------------------------------------------------
float4 PS(VS_OUTPUT input): SV_Target {

    float width, height;
    g_texture[0].GetDimensions(width, height);
    uint y = g_texture[0].Load(int3(input.TexCoord.x * width, input.TexCoord.y * height, 0)).r;

    g_texture[1].GetDimensions(width, height);

    uint2 uv = g_texture[1].Load(int3(input.TexCoord.x * width, input.TexCoord.y * height, 0)).rg;

    return YUV2RGB(float4(y / 255.0, uv.r / 255.0, uv.g / 255.0, 1.0));
}
