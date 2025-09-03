#pragma once
#include <cfloat>
#include "FVector.h"

// ī�޶��� ���� ���. UPrimitive�� �������� �ʵ��� ����.
// - �ڵ� ������ ���: UpdateCamera(focusLocation, focusRadius)
// - ���� ������ ���: UpdateCamera2(centerOfMass, desiredRenderScale)
class UCamera
{
public:
    static UCamera* Main;     // Ȱ�� ī�޶�(�ɼ�)

    FVector Location{};       // ���� ���� ī�޶� ��ġ(=��Ŀ��/�߽�)
    float   RenderScale = 1.0f;  // ���� ���� ������
    float   TargetRenderScale = 1.0f;  // ��ǥ ������(������ ������� ���)

    float RefRadius = 0.2f;   // �� �������� �� RenderScale=1.0�� �ǵ��� �ڵ� �����ϸ�
    float MinScale = 0.15f;  // ������ ����
    float MaxScale = 2.0f;   // ������ ����
    float SmoothT = 0.25f;   // Lerp ����(0~1)

public:
    // ��ġ ����
    void SetLocation(FVector Location);

    void UpdateCamera(FVector CenterOfMass, float DesiredRenderScale);

    // ���� �� ī�޶� ��ȯ
    FVector ConvertToCameraSpaceLocation(FVector WorldCoord) const;
    float   ConvertToCameraSpaceRadius(float WorldRadius) const;
	// ī�޶� �� ���� ��ȯ
    FVector ConvertToWorldSpaceLocation(FVector CameraSpaceCoord);
};
