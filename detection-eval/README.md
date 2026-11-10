# Detection Evaluation

`veremi_runner` replays the VeReMi Extension receiver logs through the confidence mechanism
(`../include`, `../src`) and prints one line per reception (`rcvTime,receiver,sender,score,distError`).
The Python scripts turn that stream into the detection results of the paper. A reception is
positive when its sender runs a fault profile and flagged when its confidence is S < 0.5.

## Requirements

- C++20 toolchain, CMake, Apache Arrow and Parquet (`libarrow-dev`, `libparquet-dev`).
- The Parquet datalake from `../veremi-extension/run_pipeline.sh` (`../veremi-extension/parquet/`).
- Python: `python3 -m venv .venv && .venv/bin/pip install -r requirements.txt`
  (`../reproduce_paper.sh` creates it on first run).

## Reproducing the paper

From the repository root, in two stages, because the mechanism constants are compiled in from
`include/MechanismConstants.h`:

```bash
./reproduce_paper.sh select    # window selection; then copy W* and its weights into include/MechanismConstants.h
./reproduce_paper.sh results   # every other result, built with the updated header
```

Results and one log per step go to `results/paper/<experiment>/`. `WORKERS=2` by default.

## Scripts

| Script | Paper | Output (default `results/<dir>/`) |
|---|---|---|
| `window_selection.py` | Sec. V-B: W* and its d_max weights | `window_selection/` |
| `optimize_weights.py` | Sec. V-B: d_max weights for one W (also used by `window_selection.py`) | `calibration/` |
| `phase1_honest_residuals.py` | Sec. IV-B: v_tol and frozen-position rule on honest senders | `phase1/` |
| `run_full_detection_eval.py` | Table I (Full Model rows) and Table II (ablation); `--configs full` for Table I only | `detection/` |
| `run_scorer_comparison.py` | Table III: Beta vs. EWMA vs. Majority Vote, ROC AUC, eviction latency | `scorers/` |
| `t_max_sweep.py` | Sec. IV-B: T_max sensitivity | `t_max/` |

Every script takes `--output-dir` and writes its result files plus `<name>.log` (`results_io.py`);
the evaluation scripts also take `--workers`.

## Runner

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release && cmake --build build -j 2
./build/veremi_runner --dir ../veremi-extension/parquet/ConstPos_0709/VeReMi_<...>
```

Main flags: `--trust BetaReputation|EWMA|MajorityVote`, `--historySize`, `--maxPosDev`,
`--speedWeight`, `--yawWeight`, `--priorWeight`, `--maxInterFrameTime`; ablation:
`--no-consistency` (w/o Phase 1), `--no-dr` (w/o Phase 2), `--no-dyn-thresh`;
`--calib` prints the dynamic-threshold features of evaluated vectors; `--profile` prints latency.
Unset flags take the values of `include/MechanismConstants.h`.
