#include "UCamera.h"
#include <algorithm>
#include <cmath>

UCamera* UCamera::Main = nullptr;

void UCamera::SetLocation(FVector Location)
{
    this->Location = Location;
}

void UCamera::UpdateCamera(FVector CenterOfMass, float DesiredRenderScale)
{
    SetLocation(CenterOfMass);
    TargetRenderScale = std::max(MinScale, std::min(MaxScale, DesiredRenderScale));
    RenderScale = SmoothT * TargetRenderScale + (1.0f - SmoothT) * RenderScale;
    if (std::fabs(RenderScale - TargetRenderScale) < 0.01f)
        RenderScale = TargetRenderScale;
}

FVector UCamera::ConvertToCameraSpaceLocation(FVector WorldCoord) const
{
    return (WorldCoord - this->Location) * RenderScale;
}

float UCamera::ConvertToCameraSpaceRadius(float WorldRadius) const
{
    return WorldRadius * RenderScale;
}

FVector UCamera::ConvertToWorldSpaceLocation(FVector CameraSpaceCoord)
{
    if (Main)
        return CameraSpaceCoord / Main->RenderScale + Main->Location;
    return CameraSpaceCoord; // fallback
}
