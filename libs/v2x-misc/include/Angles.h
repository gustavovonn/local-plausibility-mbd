#ifndef ANGLES_H_
#define ANGLES_H_

// Angle wrapping and the two V2X angle conventions:
//   compass:   clockwise from north (CAM heading)
//   Cartesian: counter-clockwise from east (CDD CartesianAngle)

#include <cmath>

namespace Angles {

// [-pi, pi]
inline double wrapPi(double rad)
{
    return std::remainder(rad, 2.0 * M_PI);
}

// [0, 360)
inline double wrap360(double deg)
{
    const double r = std::fmod(deg, 360.0);
    return r < 0.0 ? r + 360.0 : r;
}

// both directions are the reflection a -> 90 - a
inline double compassToCartesianDeg(double deg)
{
    return wrap360(90.0 - deg);
}

inline double cartesianToCompassDeg(double deg)
{
    return wrap360(90.0 - deg);
}

}  // namespace Angles

#endif  // ANGLES_H_
