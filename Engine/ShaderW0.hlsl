// ShaderW0.hlsl 파일 전체를 아래 코드로 교체하세요.

cbuffer PerFrame : register(b0)
{
    // C++의 FVector(float3)와 맞추기 위해 float3로 변경
    float3 Offset;
    float Scale;
    float4 ObjectColor; // C++의 Color(float3)와 Alpha(float)를 합친 것
};

struct VS_OUT
{
    float4 position : SV_POSITION;
    float4 color : COLOR; 
};

VS_OUT mainVS(float4 position : POSITION, float4 color : COLOR)
{
    VS_OUT output;
    // Offset이 float3이므로, float4 생성자에 맞게 z 요소를 0으로 채워줍니다.
    output.position = position * float4(Scale, Scale, 1.0, 1.0) + float4(Offset, 0.0);
    output.color = color;
    return output;
}

float4 mainPS(VS_OUT input) : SV_TARGET
{
    // 상수 버퍼에서 직접 가져온 ObjectColor를 최종 색상으로 사용
    return ObjectColor;
}