#ifndef MOTION_VECTOR_ADAPTER_H_
#define MOTION_VECTOR_ADAPTER_H_

#include "ReceivedVector.h"
#include <cmath>

/**
 * @brief Derivation helpers shared by the experiment trace loaders.
 *
 */
namespace MotionVectorAdapter {

/// Heading vector -> compass degrees in [0, 360).
inline double headingDegrees(double hx, double hy)
{
    double deg = std::atan2(hx, hy) * 180.0 / M_PI;
    if (deg < 0.0) deg += 360.0;
    return deg;
}

} // namespace MotionVectorAdapter

#endif // MOTION_VECTOR_ADAPTER_H_
