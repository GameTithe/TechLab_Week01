// ShaderW0.hlsl ���� ��ü�� �Ʒ� �ڵ�� ��ü�ϼ���.

cbuffer PerFrame : register(b0)
{
    // C++�� FVector(float3)�� ���߱� ���� float3�� ����
    float3 Offset;
    float Scale;
    float4 ObjectColor; // C++�� Color(float3)�� Alpha(float)�� ��ģ ��
};

struct VS_OUT
{
    float4 position : SV_POSITION;
    float4 color : COLOR; 
};

VS_OUT mainVS(float4 position : POSITION, float4 color : COLOR)
{
    VS_OUT output;
    // Offset�� float3�̹Ƿ�, float4 �����ڿ� �°� z ��Ҹ� 0���� ä���ݴϴ�.
    output.position = position * float4(Scale, Scale, 1.0, 1.0) + float4(Offset, 0.0);
    output.color = color;
    return output;
}

float4 mainPS(VS_OUT input) : SV_TARGET
{
    // ��� ���ۿ��� ���� ������ ObjectColor�� ���� �������� ���
    return ObjectColor;
}