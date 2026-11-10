#!/usr/bin/env python3
"""Sensitivity of the full model to T_max on the eight 0709 fault profiles.

Reports macro F1 and macro FPR for each T_max; the value used by the paper is not chosen here.
Usage: python t_max_sweep.py [--values 1.5 2.0 2.5 3.0]
"""
import argparse
import json
import os
from concurrent.futures import ThreadPoolExecutor

import run_full_detection_eval as rca
from results_io import default_dir, start_log

parser = argparse.ArgumentParser()
parser.add_argument("--values", nargs="+", type=float, default=[1.5, 2.0, 2.5, 3.0])
parser.add_argument("--scenarios", nargs="+", default=rca.ALL_0709_SCENARIOS)
parser.add_argument("--workers", type=int, default=2)
parser.add_argument("--output-dir", default=default_dir("t_max"))
args = parser.parse_args()
start_log(args.output_dir, "t_max_sweep")

window = rca.header_history_window()
rows = []
print("T_max [s]  macro F1  macro FPR [%]")
for t_max in args.values:
    cfg = {**rca.CHECKER_CONFIGS[0], "flags": ["--maxInterFrameTime", str(t_max)]}
    with ThreadPoolExecutor(max_workers=args.workers) as pool:
        res = list(pool.map(lambda sc: rca.run_scenario_config(sc, cfg, window), args.scenarios))
    f1 = sum(r["f1"] for r in res) / len(res)
    fpr = sum(r["fpr"] for r in res) / len(res)
    rows.append({"t_max": t_max, "macro_f1": f1, "macro_fpr": fpr})
    print(f"{t_max:9.1f}  {f1:8.4f}  {fpr:13.3f}", flush=True)
with open(os.path.join(args.output_dir, "t_max_sweep.json"), "w") as f:
    json.dump(rows, f, indent=1)
