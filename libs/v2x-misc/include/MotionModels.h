#ifndef MOTION_MODELS_H_
#define MOTION_MODELS_H_

#include "MotionTypes.h"
#include "KinematicRegression.h"
#include "CoordinatePolicies.h"
#include "CtraKinematics.h"
#include <memory>
#include <string>
#include <deque>

// Interface every motion model implements. Lives here rather than in its own
// header because the models are its only implementers.
class IMotionModel
{
public:
    virtual ~IMotionModel() = default;

    // Predict vehicle state forward over delta time (dt in seconds)
    virtual void predict(const std::deque<KinematicState>& history,
                         const KinematicState& lastState,
                         PredictedState& nextState,
                         double dt) = 0;

    // Compute spatial position error in meters between the actual received
    // beacon and the predicted expected state
    virtual double computeError(const PredictedState& expected,
                                const KinematicState& actual) = 0;

    virtual std::string getName() const = 0;
};

template <typename CoordPolicy>
class CtraMotionModel : public IMotionModel
{
public:
    void predict(const std::deque<KinematicState>& history, 
                 const KinematicState& lastState, 
                 PredictedState& nextState, 
                 double dt) override
    {
        if (dt <= 0.0) return;

        // Single-step CTRA straight off the reported payload derivatives.
        const LocalDisplacement step =
            integrateCtra(lastState, lastState.accel, lastState.yawRate, dt);
        project(lastState, step, nextState);
    }

    double computeError(const PredictedState& expected, 
                        const KinematicState& actual) override
    {
        return CoordPolicy::distance(expected.lat, expected.lon, actual.lat, actual.lon);
    }

    std::string getName() const override { return std::string(CoordPolicy::name()) + "CTRA"; }

protected:
    /// Project a local metric step onto the configured coordinate system.
    static void project(const KinematicState& lastState,
                        const LocalDisplacement& step,
                        PredictedState& nextState)
    {
        CoordPolicy::translate(lastState.lat, lastState.lon, step.dx, step.dy,
                               nextState.lat, nextState.lon);
        nextState.speed = step.speed;
        nextState.heading = step.heading;
    }
};

template <typename CoordPolicy>
class HistoryAwareCtraMotionModel : public IMotionModel
{
public:
    void predict(const std::deque<KinematicState>& history, 
                 const KinematicState& lastState, 
                 PredictedState& nextState, 
                 double dt) override
    {
        if (dt <= 0.0) return;

        // OLS-smoothed derivatives over the tracking window (paper Eq. 1 and 2),
        // then the same CTRA integration.
        double a_hat = lastState.accel;
        double w_hat = lastState.yawRate;
        KinematicRegression::fitOLS(history, lastState.accel, lastState.yawRate, a_hat, w_hat);

        const LocalDisplacement step = integrateCtra(lastState, a_hat, w_hat, dt);
        project(lastState, step, nextState);
    }

    double computeError(const PredictedState& expected, 
                        const KinematicState& actual) override
    {
        return CoordPolicy::distance(expected.lat, expected.lon, actual.lat, actual.lon);
    }

    std::string getName() const override { return std::string(CoordPolicy::name()) + "HistoryAwareCTRA"; }

protected:
    /// Project a local metric step onto the configured coordinate system.
    static void project(const KinematicState& lastState,
                        const LocalDisplacement& step,
                        PredictedState& nextState)
    {
        CoordPolicy::translate(lastState.lat, lastState.lon, step.dx, step.dy,
                               nextState.lat, nextState.lon);
        nextState.speed = step.speed;
        nextState.heading = step.heading;
    }
};

// Aliases
using CartesianCtraModel             = CtraMotionModel<CartesianCoordinatePolicy>;
using GeographicCtraModel            = CtraMotionModel<Wgs84CoordinatePolicy>;
using CartesianHistoryAwareCtraModel = HistoryAwareCtraMotionModel<CartesianCoordinatePolicy>;
using GeographicHistoryAwareCtraModel = HistoryAwareCtraMotionModel<Wgs84CoordinatePolicy>;

class MotionModelFactory
{
public:
    static std::unique_ptr<IMotionModel> create(
        const std::string& name,
        const std::string& coordSystem = "wgs84")
    {
        const bool cartesian = (coordSystem == "cartesian" || coordSystem == "Cartesian");

        if (name == "HistoryAwareCTRA" || name == "historyawarectra" || name == "ols-ctra") {
            if (cartesian) {
                return std::make_unique<CartesianHistoryAwareCtraModel>();
            }
            return std::make_unique<GeographicHistoryAwareCtraModel>();
        }

        if (cartesian) {
            return std::make_unique<CartesianCtraModel>();
        }
        return std::make_unique<GeographicCtraModel>();
    }
};

#endif // MOTION_MODELS_H_
