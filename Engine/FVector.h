// NOTE: 관례상 수학 관련 모듈은 header-only로 구현되므로 별도의 .cpp 파일이 없습니다.
#pragma once
#include <cmath>

struct FVector
{
    float x, y, z;
	FVector() : x(0), y(0), z(0) {}
    FVector(float _x, float _y, float _z) : x(_x), y(_y), z(_z) {}

    static float Distance2(const FVector& a, const FVector& b)
    {
        float dx = a.x - b.x, dy = a.y - b.y, dz = a.z - b.z;
        return (dx * dx + dy * dy + dz * dz);
    }
    float Magnitude()
    {
        return std::sqrt(x * x + y * y + z * z);
    }
    void Normalize()
    {
        float len = Magnitude();
        if (len > 1e-6f) { x /= len; y /= len; z /= len; }
        else { x = y = z = 0.0f; }
    }

    // 연산자들 (필요한 것만 옮겨도 됩니다)
    FVector& operator+=(const FVector& v) { x += v.x; y += v.y; z += v.z; return *this; }
    FVector  operator+(const FVector& v) const { return { x + v.x, y + v.y, z + v.z }; }
    FVector& operator-=(const FVector& v) { x -= v.x; y -= v.y; z -= v.z; return *this; }
    FVector  operator-(const FVector& v) const { return { x - v.x, y - v.y, z - v.z }; }
    FVector& operator*=(const FVector& v) { x *= v.x; y *= v.y; z *= v.z; return *this; }
    FVector& operator*=(float s) { x*= s, y*= s, z*= s; return *this; }
    FVector  operator*(const FVector& v) const { return { x * v.x, y * v.y, z * v.z }; }
    FVector  operator*(float s) const { return { x * s, y * s, z * s }; }
    FVector& operator/=(const FVector& v) { x /= v.x; y /= v.y; z /= v.z; return *this; }
    FVector  operator/(const FVector& v) const
    {
        return { v.x == 0 ? 0.0f : x / v.x, v.y == 0 ? 0.0f : y / v.y, v.z == 0 ? 0.0f : z / v.z };
    }
    FVector  operator/(float s) const { return s == 0 ? FVector{} : FVector{ x / s, y / s, z / s }; }
};