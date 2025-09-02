cbuffer constants : register(b0)
{
    float3 Offset;
    float Scale;
}
struct VS_INPUT
{
    float4 position : POSITION;    // input position
    float4 color : COLOR;           // input color 
};

struct PS_INPUT
{
    float4 position : SV_POSITION; // Transformed position
    float4 color : COLOR;
};

PS_INPUT mainVS(VS_INPUT input)
{
    PS_INPUT output;
    output.position = input.position * float4(Scale, Scale, Scale,1) + float4(Offset, 0);
    output.color = input.color; 
    
    return output;
}


float4 mainPS(PS_INPUT input) : SV_Target
{
    float2 norm = input.position.xy / 1024.0f;
    return float4(norm, norm.x, 1.0f);
}
