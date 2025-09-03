
Texture2D NoiseTexture : register(t0);
SamplerState NoiseSampler : register(s0);

cbuffer constants : register(b0)
{
    float3 Offset;
    float Scale;
}

cbuffer playerInfo : register(b1)
{
    int Attribute;
    float2 Resolution;
    float iTime; 
}

struct VS_INPUT
{
    float4 position : POSITION;    // input position
    float2 uv : TEXCOORD0;
    float4 color : COLOR;           // input color 
};

struct PS_INPUT
{
    float4 position : SV_POSITION; // Transformed position
    float2 uv : TEXCOORD0; 
    float4 color : COLOR;
};

PS_INPUT mainVS(VS_INPUT input)
{
    PS_INPUT output;
    output.position = input.position * float4(Scale, Scale, Scale,1) + float4(Offset, 0);
    output.color = input.color; 
    output.uv = input.uv;
    return output;
} 

float4 mainPS(PS_INPUT input) : SV_Target
{
    float2 norm = input.position.xy / 1024.0f; 
    float4 tex0 = NoiseTexture.Sample(NoiseSampler, input.uv);
    
    return float4(tex0); 
}
