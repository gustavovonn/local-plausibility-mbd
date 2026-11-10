#ifndef MOTION_TYPES_H_
#define MOTION_TYPES_H_

// Core kinematic data types.

struct PredictedState {
    double lat = 0.0;
    double lon = 0.0;
    double speed = 0.0;
    double heading = 0.0;
};

struct KinematicState {
    double lat = 0.0;      // in degrees
    double lon = 0.0;      // in degrees
    double speed = 0.0;    // in m/s
    double heading = 0.0;  // in degrees from North
    double accel = 0.0;    // in m/s^2
    double yawRate = 0.0;  // in rad/s

    double time = 0.0;
};

#endif // MOTION_TYPES_H_
