cbuffer UI_PerFrame : register(b0)
{
    float2 WindowSize;
    float2 Pading0;
}

Texture2D UITexture : register(t0);
SamplerState UISampler : register(s0);

struct VS_INPUT
{
    float4 position : POSITIONT;
    float2 uv : TEXCOORD0;
    float4 color : COLOR;
};

struct PS_INPUT
{
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD0;
    float4 color : COLOR;
};
 
PS_INPUT mainVS(VS_INPUT input)
{
    PS_INPUT output;
    
    // [-1.0f , 1.0f] 
    float2 ndc = float2(        
     ((input.position.x / WindowSize.x) - 0.5f) * 2.0f,
     ((input.position.y / WindowSize.y) - 0.5f) * 2.0f
    );
    
    
    output.position = float4(output.position, 0.0f, 1.0f);
    output.uv = input.uv;
    output.color = input.color;

    
    return output;
}

float4 mainPS(PS_INPUT input) : SV_Target
{
    float4 tex0 = UITexture.Sample(UITexture, input.uv);
    

    // color.a ·Î fadeout Ã³¸®??
    return float4(tex0.rbg, tex0.a);   
}