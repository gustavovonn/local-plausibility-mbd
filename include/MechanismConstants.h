#ifndef MECHANISM_CONSTANTS_H
#define MECHANISM_CONSTANTS_H

#include <cmath>

// Parameters of the confidence-attribution mechanism evaluated in this work:
// the empirically calibrated weights, the inter-frame bounds and the Beta
// confidence policy.

namespace ConstantsApp {

    constexpr size_t DEFAULT_HISTORY_WINDOW = 8;
}

namespace ConstantsPhysics {

    // -------------------------------------------------------------------------
    // Inter-frame Transmission Continuity Window
    // -------------------------------------------------------------------------

    // Minimum inter-frame time [s] below which packets are treated as duplicate
    // reception artifacts across multiple receiver logs.
    // Reference: ETSI EN 302 637-2 (minimum CAM generation interval, 100 ms).
    constexpr double MIN_INTER_FRAME_TIME = 0.1;

    // Maximum inter-frame time [s] for differential kinematic continuity.
    // Operational choice; sensitivity test: detection-eval/scripts/t_max_sweep.py.
    constexpr double MAX_INTER_FRAME_TIME = 1.5;

    // -------------------------------------------------------------------------
    // Kinematic Plausibility and Standstill Noise Floor
    // -------------------------------------------------------------------------

    // Speed threshold [m/s] above which a sender is treated as moving.
    // Reference: honest-trace check, detection-eval/scripts/phase1_honest_residuals.py.
    constexpr double FROZEN_SPEED_THRESHOLD = 2.0;

    // Displacement [m] below which a moving sender is treated as frozen.
    // Reference: honest-trace check, detection-eval/scripts/phase1_honest_residuals.py.
    constexpr double FROZEN_DISPLACEMENT_MIN = 0.3;

    // -------------------------------------------------------------------------
    // Operational Defaults (Parametrizable via EngineConfig)
    // -------------------------------------------------------------------------

    // Default kinematic speed discrepancy tolerance (|v_reported - v_implied|) [m/s].
    // Reference: honest residuals, detection-eval/scripts/phase1_honest_residuals.py.
    constexpr double DEFAULT_SPEED_TOLERANCE_MPS = 4.0;

    // -------------------------------------------------------------------------
    // Context-Aware Dynamic Threshold (paper Eq. 10)
    //   d_max = d_max_0 + w_v * v_safe * dt + w_w * |w| * v_safe
    // -------------------------------------------------------------------------

    // Calibrated offline via quantile regression (q = 0.99) over honest senders
    // of ConstPos_1416, hour 50400, for W = DEFAULT_HISTORY_WINDOW (window_selection.py; paper Sec. V-B).
    constexpr double DEFAULT_MAX_POS_DEV = 2.3448;          // d_max_0 [m]
    constexpr double DEFAULT_CONTEXT_SPEED_WEIGHT = 0.4945; // w_v (longitudinal displacement)
    constexpr double DEFAULT_CONTEXT_YAW_WEIGHT = 0.9807;   // w_w (rotational factor)

    // Prior weight W_p of the counterless Beta update (paper Eq. 11).
    // Reference: properties of the Beta recurrence (paper Sec. IV-C).
    constexpr double DEFAULT_PRIOR_WEIGHT = 10.0;

    // Default Reward Policy (Bayesian evidence increments)
    constexpr double DEFAULT_ALPHA_REWARD = 1.0;          // Full confirmation increment

    // Default Penalty Policy (Severity-graduated β increments)
    // Reference: properties of the Beta recurrence (paper Sec. IV-C).
    constexpr double DEFAULT_SOFT_MARGIN_RATIO = 1.35;    // Jitter allowance
    constexpr double DEFAULT_PENALTY_SOFT_BASE = 0.10;    // Jitter base penalty
    constexpr double DEFAULT_PENALTY_SOFT_SCALE = 0.50;   // Jitter multiplier
    constexpr double DEFAULT_PENALTY_SEVERE_BASE = 5.0;   // Severe violation base
    constexpr double DEFAULT_PENALTY_SEVERE_SCALE = 5.0;  // Severe violation multiplier
    constexpr double DEFAULT_MAX_BETA_PENALTY = 15.0;     // Phase 1 penalty and single-frame ceiling (> W_p)
}


#endif
