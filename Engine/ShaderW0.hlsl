Texture2D NoiseTexture : register(t0);
SamplerState NoiseSampler : register(s0);

static const float speed = 70.0f;
static const float dist = 60.0f;
static const float zoom = 0.0004f;

static const float3 red = float3(0.99, 0.05, 0.01);
static const float3 green = float3(0.058, 0.933, 0.124);
static const float3 blue = float3(0.05, 0.411, 0.941);


 
cbuffer constants : register(b0)
{
    // C++�� FVector(float3)�� ���߱� ���� float3�� ����
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

struct VS_OUT
{
    float4 position : SV_POSITION; // Transformed position
    float2 uv : TEXCOORD0; 
    float4 color : COLOR;
    float2 center : TEXCOORD1;
};

VS_OUT mainVS(VS_INPUT input )
{
    VS_OUT output;
    output.position = input.position * float4(Scale + 0.1, Scale + 0.1, Scale + 0.1,1) + float4(Offset, 0);
    output.color = input.color; 
    output.uv = input.uv;
    
    
    // [0 , 1] 
    float2 ndcCenter = (Offset.xy) * 0.5f + 0.5f; 
    ndcCenter.y = 1.0f - ndcCenter.y; 
    
    output.center = ndcCenter * Resolution.xy;
    
    return output;
} 

float4 mainPS(VS_OUT input) : SV_Target
{
    //float2 norm = input.position.xy / 1024.0f; 
    //float4 tex0 = NoiseTexture.Sample(NoiseSampler, input.uv);
    //return float4(tex0); 
    float2 fragCoord = input.position.xy;
    float2 center = input.center;
     
    float2 uv = input.uv;
    float a = max(0.0f, 1.0f - distance(uv, float2(0.5f, 0.5f)) / 0.5f);
    
    //float2 textureCoord = float2(input.uv.x, input.uv.y - iTime * speed); 
    //float text = NoiseTexture.Sample(NoiseSampler, textureCoord * zoom );
    float2 textureCoord = float2(fragCoord.x, fragCoord.y - iTime * speed);
    float4 text = NoiseTexture.Sample(NoiseSampler, textureCoord * zoom);
    
    float i = text.x;
   
    a *= a;
    
    a /= i / 10.;

    
    //float3 color = color1 * a; 
    //return float4(color, 1.);
    
    float3 color1;
    if(Attribute == 0)
        color1 = blue;
    
    if(Attribute == 1)
        color1 = red;
    
    
    if(Attribute == 2)
        color1 = green;
    
    float3 color = color1 * a;
    return float4(color, a);
}