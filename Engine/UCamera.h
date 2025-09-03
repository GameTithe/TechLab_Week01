#pragma once
#include <cfloat>
#include "FVector.h"

// 카메라만의 순수 모듈. UPrimitive에 의존하지 않도록 설계.
// - 자동 스케일 모드: UpdateCamera(focusLocation, focusRadius)
// - 수동 스케일 모드: UpdateCamera2(centerOfMass, desiredRenderScale)
class UCamera
{
public:
    static UCamera* Main;     // 활성 카메라(옵션)

    FVector Location{};       // 월드 기준 카메라 위치(=포커스/중심)
    float   RenderScale = 1.0f;  // 현재 실제 스케일
    float   TargetRenderScale = 1.0f;  // 목표 스케일(스무딩 대상으로 사용)

    float RefRadius = 0.2f;   // 이 반지름일 때 RenderScale=1.0이 되도록 자동 스케일링
    float MinScale = 0.15f;  // 스케일 하한
    float MaxScale = 2.0f;   // 스케일 상한
    float SmoothT = 0.25f;   // Lerp 비율(0~1)

public:
    // 위치 설정
    void SetLocation(FVector Location);

    void UpdateCamera(FVector CenterOfMass, float DesiredRenderScale);

    // 월드 → 카메라 변환
    FVector ConvertToCameraSpaceLocation(FVector WorldCoord) const;
    float   ConvertToCameraSpaceRadius(float WorldRadius) const;
	// 카메라 → 월드 변환
    FVector ConvertToWorldSpaceLocation(FVector CameraSpaceCoord);
};
