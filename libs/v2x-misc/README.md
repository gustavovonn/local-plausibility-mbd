# v2x-misc

Reusable V2X motion primitives. Header-only, standard library only, and free of any
detection policy, so it cross-compiles for embedded targets and can be shared by
experiments that use a different confidence mechanism.

| Header | Contents |
|---|---|
| `Constants.h` | WGS-84 ellipsoid parameters and radii of curvature; numerical guards of the motion math |
| `MotionTypes.h` | `KinematicState`, `PredictedState` |
| `CoordinatePolicies.h` | Cartesian and WGS-84 policies: `translate`, `distance`, `displacement` |
| `CtraKinematics.h` | Coordinate-free CTRA integrator with constant-acceleration fallback |
| `KinematicRegression.h` | OLS estimation of acceleration and yaw rate over a history window |
| `MotionModels.h` | `IMotionModel`, single-step and history-aware CTRA models, `MotionModelFactory` |

Usage: add `libs/v2x-misc/include` to the include path. There is nothing to compile or link.
