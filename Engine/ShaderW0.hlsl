Texture2D NoiseTexture : register(t0);
SamplerState NoiseSampler : register(s0);

static const float speed = 70.0f;
static const float dist = 60.0f;
static const float zoom = 0.0004f;

static const float3 red = float3(0.99, 0.05, 0.01);
static const float3 green = float3(0.058, 0.933, 0.124);
static const float3 blue = float3(0.05, 0.411, 0.941);
static const float3 gray = float3(0.15, 0.15, 0.15);

static const float SmoothK = 20.0f; 
static const float SmoothK_NDC = 0.05f;
 
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
    
    float2 velocity; 
}

cbuffer contactInfo : register(b2)
{ 
    float playerScale;
    float playerCenter; 
    float2 Padding;
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
 
VS_OUT mainVS(VS_INPUT input)
{
    VS_OUT o;
     
    float4 clip = input.position * float4(Scale, Scale, Scale, 1) + float4(Offset, 0);
     
    const float2 playerNDC = float2(0.0, 0.0);
    float2 centerNDC = Offset.xy;
    float2 toPlayer = playerNDC - centerNDC;
    float dist = length(toPlayer);
     
    float blendStart = Scale + playerScale + 0.2; // 시작 거리
    
    float t = saturate((blendStart - dist) / 0.05);
    t = t * t * (3.0 - 2.0 * t); // smoothstep(0,1,t)
     
    //edge는 가중치 up
    float edge = saturate(length(input.uv - float2(0.5, 0.5)) * 2.0);
     
    if (t > 0.0)
    {
        float2 dirN = toPlayer / max(dist, 1e-5);
        float disp = t * edge * SmoothK_NDC; // 한 개의 노브
        clip.xy += dirN * disp; 
         
     } 
     
    o.position = clip;
    o.color = input.color;
    o.uv = input.uv;
      
    return o;
}
float4 mainPS(VS_OUT input) : SV_Target
{ 
    float2 fragCoord = input.position.xy;
    float2 center = input.center;
     
    float2 uv = input.uv;
    float a = max(0.0f, 1.0f - distance(uv, float2(0.5f, 0.5f)) / 0.5f);
    
    //0.02
    float2 textureCoord = float2(fragCoord.x - iTime * velocity.x * 500.f, fragCoord.y - iTime * velocity.y * 500.f);
    float4 text = NoiseTexture.Sample(NoiseSampler, textureCoord * zoom);
    
    float i = text.x;
   
    a *= a;
    
    a /= i / 5.;
     
    float3 color1;
    if(Attribute == 0)
        color1 = blue;
    
    if(Attribute == 1)
        color1 = red;
    
    if(Attribute == 2)
        color1 = green;
    
    if(Attribute == 3)
        color1 = gray; 
    
    float3 color = color1 * a;
    return float4(color, a);
}