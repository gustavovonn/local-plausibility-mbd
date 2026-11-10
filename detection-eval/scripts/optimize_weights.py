import subprocess
import pandas as pd
import io
import statsmodels.api as sm
import numpy as np

import os
import glob
import argparse

from run_full_detection_eval import extract_attackers, header_history_window

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
PARENT_DIR = os.path.dirname(SCRIPT_DIR)
RUNNER_PATH = os.path.join(PARENT_DIR, "build", "veremi_runner")

def calibrate(scenario="ConstPos_1416", history_size=None, quantile=0.99, window=None, verbose=False):
    """Fit d_max = beta_0 + beta_v * (v_safe * dt) + beta_yaw * (|yawRateHat| * v_safe)
    on honest senders of the scenario; window restricts it to one hourly directory
    (its start time, e.g. 50400). Returns (beta_0, beta_v, beta_yaw, coverage %)."""
    history_size = history_size or header_history_window()
    source_dirs = sorted(glob.glob(os.path.abspath(os.path.join(
        PARENT_DIR, "../veremi-extension/parquet", scenario, f"VeReMi_{window}_*" if window else "VeReMi_*"))))
    if not source_dirs:
        raise FileNotFoundError(f"No parquet directories found for {scenario} (window {window})")

    frames = []
    for d in source_dirs:
        cmd = [RUNNER_PATH, "--dir", d, "--calib", "--historySize", str(history_size)]
        result = subprocess.run(cmd, capture_output=True, text=True)
        if result.returncode != 0:
            raise RuntimeError(f"veremi_runner failed: {result.stderr}")

        # Keep only messages sent by honest vehicles (receiver logs also contain messages from attackers).
        d_df = pd.read_csv(io.StringIO(result.stdout))
        d_df = d_df[~d_df['sender'].isin(extract_attackers(d))]
        if verbose:
            print(f"  {os.path.basename(d)}: {len(d_df)} honest-sender samples")
        frames.append(d_df)

    df = pd.concat(frames, ignore_index=True).dropna()
    if verbose:
        print(f"Extracted {len(df)} samples from {scenario}.")

    # The regression models the natural prediction error as a function of the same terms
    # used by the dynamic threshold at runtime.
    df['X1_speed_dt'] = df['safeSpeed'] * df['dt']
    df['X2_yaw_speed'] = df['yawRateHat'].abs() * df['safeSpeed']

    X = sm.add_constant(df[['X1_speed_dt', 'X2_yaw_speed']])
    y = df['distError']

    # Fit the conditional quantile: (q * 100)% of honest errors fall below d_max
    model = sm.QuantReg(y, X).fit(q=quantile)
    if verbose:
        print(f"Performing Quantile Regression (q = {quantile}) for the upper-envelope boundary...")
        print(model.summary())

    coverage = float(np.mean(y <= model.predict(X)) * 100)
    return (float(model.params['const']), float(model.params['X1_speed_dt']),
            float(model.params['X2_yaw_speed']), coverage)

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Calibrate the dynamic threshold weights (paper Eq. 10).")
    parser.add_argument("--scenario", default="ConstPos_1416", help="Calibration scenario (default: ConstPos_1416)")
    parser.add_argument("--window", type=int, default=None, help="Restrict to one hourly directory by its start time, e.g. 50400 (default: all)")
    parser.add_argument("--quantile", type=float, default=0.99, help="Upper-envelope quantile (default: 0.99)")
    parser.add_argument("--history-size", type=int, default=header_history_window(), help="History window W (default: DEFAULT_HISTORY_WINDOW in MechanismConstants.h)")
    from results_io import default_dir, start_log
    parser.add_argument("--output-dir", default=default_dir("calibration"))
    args = parser.parse_args()
    start_log(args.output_dir, "optimize_weights")

    print("Running veremi_runner on honest senders to extract prediction errors...")
    beta_0, beta_v, beta_yaw, coverage = calibrate(args.scenario, args.history_size, args.quantile,
                                                   args.window, verbose=True)

    print("\n=============================================")
    print("OPTIMIZED VEREMI PARAMETERS FOR veremi_runner")
    print("=============================================")
    print(f"maxPosDev (beta_0):       {beta_0:.4f}")
    print(f"contextSpeedWeight (w_v): {beta_v:.4f}")
    print(f"contextYawWeight (w_yaw): {beta_yaw:.4f}")
    print(f"Honest samples <= d_max:  {coverage:.2f}%")
    print("=============================================\n")
    import json
    with open(os.path.join(args.output_dir, "optimize_weights.json"), "w") as f:
        json.dump({"W": args.history_size, "d0": beta_0, "w_v": beta_v, "w_yaw": beta_yaw, "coverage_pct": coverage}, f, indent=1)
