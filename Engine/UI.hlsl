 
cbuffer UI_PerDraw : register(b0)
{
    float2 WindowSize; 
    int IsHovering;
    float Padding;
};

cbuffer UI_BackgroundInfo : register(b1)
{
    float2 playerWorldPos;
    float2 textureResolution; 
    float flag;
    float3 Padding3;
}

Texture2D UITexture : register(t0);
SamplerState UISampler : register(s0);

struct VS_INPUT
{
    float2 position : POSITION;
    float2 uv : TEXCOORD;
    float4 color : COLOR;
};

struct PS_INPUT
{
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD;
    float4 color : COLOR;
};
 
PS_INPUT mainVS(VS_INPUT input)
{
    PS_INPUT output;
    
    // [-1.0f , 1.0f] 
    float2 ndc = float2(        
     ((input.position.x / WindowSize.x) - 0.5f) * 2.0f,
     -((input.position.y / WindowSize.y) - 0.5f) * 2.0f
    );
     
    output.position = float4(ndc, 0.0f, 1.0f);
    output.uv = input.uv;
    output.color = input.color;
    
    return output;
}

float4 mainPS(PS_INPUT input) : SV_Target
{
    float4 tex0 = float4(0, 0, 0, 0);
    if(flag)
    {
        //0.1  / 256  
       // float2 uvOffset = frac(playerWorldPos / textureResolution);
       // tex0 = UITexture.Sample(UISampler, input.uv + uvOffset);
        
        //tex0 = UITexture.Sample(UISampler, (playerWorldPos % textureResolution) / textureResolution);
        //tex0 = UITexture.Sample(UISampler, input.uv);
        float2 uv = frac(input.uv + playerWorldPos % textureResolution);
         
        tex0 = UITexture.Sample(UISampler, uv);
    }
     
    else
    {
        tex0 = UITexture.Sample(UISampler, input.uv); 
        
    }
  
    if (IsHovering == 1)
    {
        tex0.rgb *= 1.5; 
    } 
    
    return float4(tex0.rgb, tex0.a);
}