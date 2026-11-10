"""Output convention of the detection-eval scripts: one directory per experiment under
results/ (overridable with --output-dir), holding its result files and <name>.log."""
import os
import sys

RESULTS_ROOT = os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))), "results")


def default_dir(experiment):
    return os.path.join(RESULTS_ROOT, experiment)


class _Tee:
    def __init__(self, *streams):
        self.streams = streams

    def write(self, data):
        for s in self.streams:
            s.write(data)

    def flush(self):
        for s in self.streams:
            s.flush()


def start_log(output_dir, name):
    """Create output_dir and mirror stdout to output_dir/<name>.log."""
    os.makedirs(output_dir, exist_ok=True)
    log = open(os.path.join(output_dir, f"{name}.log"), "w", buffering=1)
    sys.stdout = _Tee(sys.__stdout__, log)
    return output_dir
