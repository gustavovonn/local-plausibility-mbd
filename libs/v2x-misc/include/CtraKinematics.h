#ifndef CTRA_KINEMATICS_H_
#define CTRA_KINEMATICS_H_

#include "Constants.h"
#include "MotionTypes.h"
#include <algorithm>
#include <cmath>

/**
 * @brief A predicted step in the vehicle's local east/north tangent plane.
 *
 * Motion models produce this; a CoordinatePolicy then projects the metric
 * displacement onto whatever coordinate system the trace uses. 
 */
struct LocalDisplacement {
    double dx      = 0.0;  // Eastward displacement [m].
    double dy      = 0.0;  // Northward displacement [m].
    double speed   = 0.0;  // Predicted speed [m/s].
    double heading = 0.0;  // Predicted heading [deg from North].
};

/**
 * @brief Constant Turn Rate and Acceleration integration (paper Sec. IV-A-2).
 *
 * Coordinate-free: returns a metric displacement in the local tangent plane.
 *
 * When the estimated yaw rate is negligible (|w| ~ 0) the CTRA trajectory
 * degenerates into straight-line motion and the closed-form integrals introduce
 * a division by zero, so the Constant Acceleration form is the physically and
 * numerically consistent fallback (paper Eq. 3-4). Otherwise the closed-form
 * CTRA displacements of Eq. 5-6 apply.
 *
 * @param lastState Last accepted state, supplying the start speed and heading.
 * @param a         Longitudinal acceleration (OLS-smoothed a_hat, or reported).
 * @param w         Yaw rate (OLS-smoothed w_hat, or reported) [rad/s].
 * @param dt        Elapsed time since that state [s].
 */
inline LocalDisplacement integrateCtra(const KinematicState& lastState,
                                       double a, double w, double dt)
{
    LocalDisplacement out;

    const double startSpeed   = lastState.speed;
    const double startHeading = lastState.heading;
    const double headingRad   = startHeading * ConstantsWGS84::DEG_TO_RAD;

    // --- Constant Acceleration fallback (|w| ~ 0) ---------------------------
    if (std::abs(w) < ConstantsNumeric::YAW_RATE_THRESHOLD)
    {
        const double predictedSpeed = startSpeed + a * dt;
        double dist = startSpeed * dt + 0.5 * a * dt * dt;
        if (predictedSpeed <= 0.0 || dist < 0.0) {
            dist = 0.0;
            out.speed = 0.0;
        } else {
            out.speed = predictedSpeed;
        }

        out.dy = dist * std::cos(headingRad); // North [m]
        out.dx = dist * std::sin(headingRad); // East  [m]
        out.heading = startHeading;
        return out;
    }

    // --- Constant Turn Rate and Acceleration --------------------------------
    const double v_dt = startSpeed + a * dt;
    const double newHeadingRad = headingRad + w * dt;

    const double inv_w = 1.0 / w;
    const double a_over_w2 = a / (w * w);

    const double sinH0 = std::sin(headingRad);
    const double cosH0 = std::cos(headingRad);
    const double sinH1 = std::sin(newHeadingRad);
    const double cosH1 = std::cos(newHeadingRad);

    out.dx = inv_w * (-v_dt * cosH1 + startSpeed * cosH0) + a_over_w2 * (sinH1 - sinH0); // East [m]
    out.dy = inv_w * ( v_dt * sinH1 - startSpeed * sinH0) + a_over_w2 * (cosH1 - cosH0); // North [m]
    out.speed = std::max(0.0, v_dt);

    double headingDeg = std::fmod(newHeadingRad * ConstantsWGS84::RAD_TO_DEG, 360.0);
    if (headingDeg < 0.0) headingDeg += 360.0;
    out.heading = headingDeg;

    return out;
}

#endif // CTRA_KINEMATICS_H_
