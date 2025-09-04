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


//flag
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

float smin(float a, float b, float k)
{
    
    float h = saturate(0.5 + 0.5 * (b - a) / k);
    return lerp(b, a, h) - k * h * (1.0 - h);
}

float sdf_circle(float2 p, float2 c, float r)
{
    return length(p - c) - r;
}

float smooth01(float t)
{
    return t * t * (3.0f - 2.0f * t);
}


float2 NdcToPixel(float2 ndc, float2 res)
{
    float2 uv01 = ndc * 0.5f + 0.5f;
    uv01.y = 1.0f - uv01.y;
    return uv01 * res;
}
float2 PixelToNdc(float2 px, float2 res)
{
    float2 uv01 = px / res;
    uv01.y = 1.0f - uv01.y;
    return uv01 * 2.0f - 1.0f;
}
float smoothInflate(float overlap, float k)
{
    // overlap ∈ [0, k]에서 0→1로 변화
    float t = saturate(overlap / k);
    // cubic S-curve
    return t * t * (3.0f - 2.0f * t);
}

//VS_OUT mainVS(VS_INPUT input )
//{
//    VS_OUT output;
//    output.position = input.position * float4(Scale + 0.02, Scale + 0.02, Scale, 1) + float4(Offset, 0);
//    output.color = input.color; 
//    output.uv = input.uv;
    
    
//    // [0 , 1] 
//    float2 ndcCenter = (Offset.xy) * 0.5f + 0.5f; 
//    ndcCenter.y = 1.0f - ndcCenter.y; 
    
//    output.center = ndcCenter * Resolution.xy;
    
//    return output;
//} 


VS_OUT mainVS(VS_INPUT input)
{
    VS_OUT o;

    // 1) 원래 파이프라인대로 클립 공간으로 변환
    float4 clipPos = input.position * float4(Scale + 0.02, Scale + 0.02, Scale, 1) + float4(Offset, 0);
        
    float2 playerNDC = float2(0.0f, 0.0f);
    float2 selfCenterNDC = Offset.xy;
    
    float2 dir = playerNDC - selfCenterNDC;
    float dist = length(dir) + 1e-6;
    float overlap = (Scale + 0.02f + playerScale) - dist;
    
    float2 nNdc = dir / dist;
    
    if (overlap > 0.2f )
    {
    // 밴드 내에서 0..1 가중치를 만들기 위해 shift & normalize
    // overlap = -K 에서 0, overlap = +K 에서 1 근처가 되도록
        float t = saturate((overlap + SmoothK_NDC) / (2.0f * SmoothK_NDC));
        float w = smooth01(t); // 0..1

    // 가장자리에서 더 크게 부풀리기(예: 원 가장자리쪽에서 강하게)
        float edgeFactor = saturate((length(input.uv - 0.5f) - 0.3f) / 0.2f);

    // 실제 이동량(NDC). w는 0..1, 최대 이동폭을 SmoothK_NDC로 제한
        float inflateNdc = w * SmoothK_NDC * edgeFactor;

    // 정점을 플레이어 방향으로 살짝 이동(clip==NDC 가정 → w=1이면 그냥 더해도 됨)
        clipPos.xy += nNdc * inflateNdc * clipPos.w; // clipPos.w가 1이라면 곱해도 동일
    }

    o.position = clipPos;
    o.color = input.color;
    o.uv = input.uv; 
    return o;
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
    
    //0.02
    float2 textureCoord = float2(fragCoord.x - iTime * velocity.x * 500.f, fragCoord.y - iTime * velocity.y * 500.f);
    float4 text = NoiseTexture.Sample(NoiseSampler, textureCoord * zoom);
    
    float i = text.x;
   
    a *= a;
    
    a /= i / 5.;

    
    //float3 color = color1 * a; 
    //return float4(color, 1.);
    
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