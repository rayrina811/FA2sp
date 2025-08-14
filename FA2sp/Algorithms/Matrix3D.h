#pragma once
#include <iostream>
#include <cmath>

class Matrix3D {
public:
    const double PI = 3.14159265358979323846;
    const double DEG_TO_RAD = PI / 180.0;
    const double SIN_60 = sin(60 * DEG_TO_RAD);

    Matrix3D(double f, double l, double h, int facing, int facingCount)
    {
        F = f;
        L = l;
        H = h;
        FacingCount = facingCount;
        Facing = facing % FacingCount;
        OutputX = 0.0;
        OutputY = 0.0;
        TransformCoordinates();
    }

    void TransformCoordinates() {
        if (F == 0.0 && L == 0.0 && H == 0.0)
            return;
        if (F == 0.0 && L == 0.0)
        {
            OutputY = H * SIN_60;
            OutputY /= -6.035;
            return;
        }

        double xyAngle = 0.0;
        if (F == 0.0 && L < 0.0)
            xyAngle = 90.0;
        else if (F == 0.0 && L > 0.0)
            xyAngle = -90.0;
        else
            xyAngle = atan2(-L, F) / DEG_TO_RAD;

        double rad = (45.0 - Facing * 360.0 / FacingCount - xyAngle) * DEG_TO_RAD;

        double length = sqrt(F * F + L * L);
        OutputX = cos(rad) * length;
        OutputY = sin(rad) * length;

        OutputY *= 0.5;
        OutputY += H * SIN_60;

        OutputX /= 6.035;
        OutputY /= -6.035;
    }

private:
    double F;
    double L;
    double H;
    int Facing;
    int FacingCount;
public:
    double OutputX;
    double OutputY;
};