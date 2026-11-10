#ifndef ENGINE_CONFIG_H_
#define ENGINE_CONFIG_H_

#include "MechanismConstants.h"
#include <cstddef>

/**
 * @brief Runtime configuration for the beacon confidence pipeline.
 *
 * Every field is default-initialised from MechanismConstants.h, so a default-constructed
 * EngineConfig is already the paper-calibrated operating point. Drivers override
 * individual fields from the CLI (parameter sweeps, ablation studies); nothing
 * outside MechanismConstants.h holds a literal.
 */
struct EngineConfig {

    // -------------------------------------------------------------------------
    // Predictor
    // -------------------------------------------------------------------------

    /// Tracking history window W* (paper Sec. IV-A, selected in Sec. V-B).
    std::size_t historyWindow = ConstantsApp::DEFAULT_HISTORY_WINDOW;

    // -------------------------------------------------------------------------
    // Checker - Phase 1 consistency check (paper Sec. IV-B)
    // -------------------------------------------------------------------------

    /// Speed discrepancy tolerance v_tol, |v_avg - v_implied| [m/s].
    double speedToleranceMps = ConstantsPhysics::DEFAULT_SPEED_TOLERANCE_MPS;

    /// Maximum inter-frame time T_max [s]; longer gaps purge the sender's history.
    double maxInterFrameTime = ConstantsPhysics::MAX_INTER_FRAME_TIME;

    // -------------------------------------------------------------------------
    // Checker - context-aware dynamic threshold (paper Eq. 10)
    //   d_max = d_max_0 + w_v * v_safe * dt + w_w * |w| * v_safe
    // -------------------------------------------------------------------------

    double maxPosDev          = ConstantsPhysics::DEFAULT_MAX_POS_DEV;           // d_max_0 [m]
    double contextSpeedWeight = ConstantsPhysics::DEFAULT_CONTEXT_SPEED_WEIGHT;  // w_v
    double contextYawWeight   = ConstantsPhysics::DEFAULT_CONTEXT_YAW_WEIGHT;    // w_w

    // -------------------------------------------------------------------------
    // Scorer - counterless Beta confidence (paper Eq. 11, Algorithm 1)
    // -------------------------------------------------------------------------

    /// Prior weight W_p of the counterless Beta update (paper Eq. 11).
    double priorWeight = ConstantsPhysics::DEFAULT_PRIOR_WEIGHT;

    /// Reward increment applied when a motion vector passes both checks.
    double alphaReward = ConstantsPhysics::DEFAULT_ALPHA_REWARD;

    /// Severity-graduated penalty policy.
    double softMarginRatio    = ConstantsPhysics::DEFAULT_SOFT_MARGIN_RATIO;
    double penaltySoftBase    = ConstantsPhysics::DEFAULT_PENALTY_SOFT_BASE;
    double penaltySoftScale   = ConstantsPhysics::DEFAULT_PENALTY_SOFT_SCALE;
    double penaltySevereBase  = ConstantsPhysics::DEFAULT_PENALTY_SEVERE_BASE;
    double penaltySevereScale = ConstantsPhysics::DEFAULT_PENALTY_SEVERE_SCALE;
    double maxBetaPenalty     = ConstantsPhysics::DEFAULT_MAX_BETA_PENALTY;

    // -------------------------------------------------------------------------
    // Ablation toggles (paper Table II)
    // -------------------------------------------------------------------------

    // Consistency check: hard physical invariance verification.
    bool enableConsistency = true;

    /// Context-aware dynamic threshold; false falls back to the static maxPosDev.
    bool enableDynThresh = true;

    // Predictive check: spatial prediction; false forces distError = 0.
    bool enablePrediction = true;
};

#endif // ENGINE_CONFIG_H_
