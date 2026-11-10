#!/usr/bin/env python3
"""History window W selection (paper Sec. V-B).

For each candidate W, the dynamic-threshold weights are calibrated on one hour of the
calibration traces (optimize_weights.calibrate) and the full model is evaluated on the
other hour of the eight fault profiles. The selected W* is the smallest window whose
macro F1 lies within --f1-tol percentage points of the best and whose macro FPR lies
within --fpr-tol (relative) of the lowest.

Usage: python window_selection.py [--windows 3 4 5 6 8 10 12 15 20] [--workers 2]
"""
import argparse
import json
import os
from concurrent.futures import ThreadPoolExecutor

import optimize_weights
import run_full_detection_eval as rca

from results_io import default_dir, start_log


def evaluate(scenario, w, weights, eval_hour):
    """Pooled confusion counts of the full model on one hourly directory of a scenario."""
    flags = ["--maxPosDev", str(weights[0]), "--speedWeight", str(weights[1]), "--yawWeight", str(weights[2])]
    total = dict(tp=0, fp=0, tn=0, fn=0)
    for d in rca.find_parquet_dirs(scenario):
        if os.path.basename(d).startswith(f"VeReMi_{eval_hour}_"):
            c = rca.evaluate_simulation(d, flags, w)
            if c is None:
                raise RuntimeError(f"runner failed on {d}")
            for k in total:
                total[k] += c[k]
    p = total["tp"] / max(1, total["tp"] + total["fp"])
    r = total["tp"] / max(1, total["tp"] + total["fn"])
    f1 = 2 * p * r / (p + r) if p + r else 0.0
    fpr = 100 * total["fp"] / max(1, total["fp"] + total["tn"])
    return f1, fpr


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--windows", nargs="+", type=int, default=[3, 4, 5, 6, 8, 10, 12, 15, 20])
    ap.add_argument("--subset", default="1416", help="Subset whose two hours are used (default: 1416)")
    ap.add_argument("--calib-scenario", default="ConstPos_1416")
    ap.add_argument("--calib-hour", type=int, default=50400, help="Hour used to calibrate the weights")
    ap.add_argument("--eval-hour", type=int, default=54000, help="Hour used to evaluate each W")
    ap.add_argument("--f1-tol", type=float, default=0.1, help="Macro F1 tolerance below the best [pp]")
    ap.add_argument("--fpr-tol", type=float, default=0.10, help="Relative macro FPR tolerance above the lowest")
    ap.add_argument("--workers", type=int, default=2)
    ap.add_argument("--output-dir", default=default_dir("window_selection"))
    args = ap.parse_args()
    start_log(args.output_dir, "window_selection")

    scenarios = [s.replace("0709", args.subset) for s in rca.ALL_0709_SCENARIOS]
    rows = []
    print(f"{'W':>3}  {'d0':>7} {'w_v':>7} {'w_w':>7}  {'macro F1':>9}  {'macro FPR %':>11}", flush=True)
    for w in args.windows:
        weights = optimize_weights.calibrate(args.calib_scenario, w, window=args.calib_hour)
        with ThreadPoolExecutor(args.workers) as pool:
            res = list(pool.map(lambda sc: evaluate(sc, w, weights, args.eval_hour), scenarios))
        f1 = sum(r[0] for r in res) / len(res)
        fpr = sum(r[1] for r in res) / len(res)
        rows.append({"W": w, "weights": weights[:3], "macro_f1": f1, "macro_fpr": fpr})
        print(f"{w:>3}  {weights[0]:7.4f} {weights[1]:7.4f} {weights[2]:7.4f}  {f1:9.4f}  {fpr:11.3f}", flush=True)

    best_f1 = max(r["macro_f1"] for r in rows)
    low_fpr = min(r["macro_fpr"] for r in rows)
    ok = [r for r in rows if r["macro_f1"] >= best_f1 - args.f1_tol / 100 and r["macro_fpr"] <= low_fpr * (1 + args.fpr_tol)]
    w_star = min(r["W"] for r in ok)
    spread = 100 * (best_f1 - min(r["macro_f1"] for r in rows))
    print(f"\nmacro F1 spread across W: {spread:.3f} pp")
    print(f"W* = {w_star} (smallest W within {args.f1_tol} pp of the best F1 and {100 * args.fpr_tol:g}% of the lowest FPR)")

    out = os.path.join(args.output_dir, "window_selection.json")
    with open(out, "w") as f:
        json.dump({"rows": rows, "W_star": w_star, "f1_spread_pp": spread}, f, indent=1)
    print(f"Saved {out}")


if __name__ == "__main__":
    main()
