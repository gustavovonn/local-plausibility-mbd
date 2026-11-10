#!/usr/bin/env python3
"""Phase 1 on honest senders of the calibration traces (ConstPos_1416, both hours).

Mirrors the engine: consecutive receptions of a sender per receiver log on rcvTime, receptions
less than T_min after the previous one skipped as duplicates, pairs farther apart than T_max
excluded. Prints percentiles of |v_avg - v_implied| (for v_tol)
and how often the frozen-position rule fires (it should not, on honest senders).
"""
import argparse
import glob
import json
import os
import re
import sys
from concurrent.futures import ProcessPoolExecutor

import numpy as np
import pandas as pd

ROOT = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..", "veremi-extension", "parquet", "ConstPos_1416")
T_MIN, T_MAX = 0.1, 1.5            # MIN/MAX_INTER_FRAME_TIME
V_TOL = 4.0                        # DEFAULT_SPEED_TOLERANCE_MPS
FROZEN_SPEED, FROZEN_DISP = 2.0, 0.3

FILES = sorted(glob.glob(os.path.join(ROOT, "VeReMi_*", "traceJSON-*.parquet")))
ATTACKERS = {int(m.group(1)) for f in FILES
             if (m := re.match(r"traceJSON-(\d+)-\d+-A(\d+)-", os.path.basename(f))) and int(m.group(2)) > 0}


def drop_duplicates(d):
    """As the engine: a reception less than T_MIN after the last kept one of the same sender is skipped."""
    keep = np.ones(len(d), dtype=bool)
    sender, t = d.sender.to_numpy(), d.rcvTime.to_numpy()
    last_s, last_t = None, -np.inf
    for i in range(len(d)):
        if sender[i] == last_s and t[i] - last_t < T_MIN:
            keep[i] = False
        else:
            last_s, last_t = sender[i], t[i]
    return d[keep]


def honest_pairs(path):
    d = pd.read_parquet(path, columns=["type", "rcvTime", "sender", "pos_x", "pos_y", "spd_x", "spd_y"])
    d = d[(d.type == 3) & ~d.sender.isin(ATTACKERS)].sort_values(["sender", "rcvTime"])
    d = drop_duplicates(d)
    d["v"] = np.hypot(d.spd_x, d.spd_y)
    g = d.groupby("sender")
    dt = g.rcvTime.diff()
    disp = np.hypot(g.pos_x.diff(), g.pos_y.diff())
    v_avg = 0.5 * (d.v + g.v.shift())
    valid = (dt >= T_MIN) & (dt <= T_MAX)
    residual = (v_avg - disp / dt).abs()[valid]
    frozen = ((v_avg > FROZEN_SPEED) & (disp < FROZEN_DISP))[valid]
    return residual.to_numpy(), int(frozen.sum())


if __name__ == "__main__":
    sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
    from results_io import default_dir, start_log
    ap = argparse.ArgumentParser()
    ap.add_argument("--workers", type=int, default=2)
    ap.add_argument("--output-dir", default=default_dir("phase1"))
    args = ap.parse_args()
    start_log(args.output_dir, "phase1_honest_residuals")
    with ProcessPoolExecutor(args.workers) as pool:
        out = list(pool.map(honest_pairs, FILES, chunksize=20))
    r = np.concatenate([o[0] for o in out])
    frozen = sum(o[1] for o in out)
    print(f"honest beacon pairs: {r.size}")
    for q in (95, 99, 99.5, 99.9):
        print(f"  p{q:<4} |v_avg - v_implied| = {np.percentile(r, q):.3f} m/s")
    print(f"  above v_tol = {V_TOL} m/s: {100 * (r > V_TOL).mean():.4f}%")
    print(f"  frozen-position rule fires: {frozen}")
    summary = {"pairs": int(r.size), "percentiles": {str(q): float(np.percentile(r, q)) for q in (95, 99, 99.5, 99.9)},
               "above_v_tol_pct": float(100 * (r > V_TOL).mean()), "frozen_triggers": int(frozen)}
    with open(os.path.join(args.output_dir, "phase1_honest_residuals.json"), "w") as f:
        json.dump(summary, f, indent=1)
