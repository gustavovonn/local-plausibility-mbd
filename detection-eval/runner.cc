#include "MotionModels.h"
#include "CoordinatePolicies.h"
#include "Checks.h"
#include "BeaconConfidenceEngine.h"
#include "ReceivedVector.h"
#include "MotionVectorAdapter.h"
#include "ParquetTraceReader.h"
#include "ConfidenceEvaluators.h"
#include "Constants.h"
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <deque>
#include <cmath>
#include <memory>
#include <algorithm>
#include <chrono>

// -----------------------------------------------------------------------------
// Utilities & Data Structures
// -----------------------------------------------------------------------------
#include <arrow/api.h>
#include <arrow/io/api.h>
#include <parquet/arrow/reader.h>
#include <filesystem>
#include <string_view>


// Compile-time switch for the per-message CSV stream.
inline constexpr bool kEmitMessageOutput = true;

// -----------------------------------------------------------------------------
// Trace loading
//
// ParquetTraceReader decodes straight into the engine's motion-vector model.
// -----------------------------------------------------------------------------
static std::vector<ReceivedVector> loadMotionVectors(const std::string& fpath)
{
    return ParquetTraceReader::read(fpath);
}


// -----------------------------------------------------------------------------
// Runner front-end over the shared BeaconConfidenceEngine
// -----------------------------------------------------------------------------
class RunnerEngine {
private:
    BeaconConfidenceEngine<CartesianCoordinatePolicy> m_engine;
    bool silent;
    bool calib;

public:
    RunnerEngine(const std::string& motionModelName, const std::string& evaluatorName,
                 const EngineConfig& engineConfig, bool silentMode = false, bool calibMode = false)
        : m_engine(motionModelName, evaluatorName, engineConfig), silent(silentMode), calib(calibMode) {}

    void reset() {
        m_engine.reset();
    }

    void processMessage(const ReceivedVector& rv) {
        const ConfidenceResult res = m_engine.onMotionVector(rv.sender, rv.mv);

        // Calibration stream: dynamic-threshold features of evaluated vectors only.
        if (calib) {
            if (res.evaluated()) {
                std::cout << rv.receiver << "," << rv.sender << "," << res.dt << ","
                          << res.safeSpeed << "," << res.yawRateHat << "," << res.distError << "\n";
            }
            return;
        }

        switch (res.disposition) {
        case VectorDisposition::Duplicate:
            if (!silent && kEmitMessageOutput) {
                std::cout << rv.mv.time << "," << rv.receiver << "," << rv.sender << ","
                          << res.confidence << ",0.0\n";
            }
            return;

        case VectorDisposition::FirstSighting:
            if (!silent && kEmitMessageOutput) {
                std::cout << rv.mv.time << "," << rv.receiver << "," << rv.sender << ",1.0,0.0\n";
            }
            return;

        case VectorDisposition::Stale:
            if (!silent && kEmitMessageOutput) {
                std::cout << rv.mv.time << "," << rv.receiver << "," << rv.sender << ","
                          << res.confidence << ",0.0\n";
            }
            return;

        case VectorDisposition::Evaluated:
            if (!silent && kEmitMessageOutput) {
                std::cout << rv.mv.time << "," << rv.receiver << "," << rv.sender << ","
                          << res.confidence << "," << res.distError << "\n";
            }
            return;
        }
    }
};

// -----------------------------------------------------------------------------
// Main Runner
// -----------------------------------------------------------------------------
int main(int argc, char* argv[]) {
    std::string drModelName = "HistoryAwareCTRA";
    std::string trustModelName = "BetaReputation";
    std::string dirPath = "";
    bool profileMode = false;
    bool calibMode = false;

    // Every field defaults to the paper-calibrated value from MechanismConstants.h;
    // the flags below override individual entries.
    EngineConfig cfg;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--dr" && i + 1 < argc) {
            drModelName = argv[++i];
        } else if (arg == "--trust" && i + 1 < argc) {
            trustModelName = argv[++i];
        } else if (arg == "--dir" && i + 1 < argc) {
            dirPath = argv[++i];
        } else if (arg == "--maxPosDev" && i + 1 < argc) {
            cfg.maxPosDev = std::stod(argv[++i]);
        } else if (arg == "--speedWeight" && i + 1 < argc) {
            cfg.contextSpeedWeight = std::stod(argv[++i]);
        } else if (arg == "--yawWeight" && i + 1 < argc) {
            cfg.contextYawWeight = std::stod(argv[++i]);
        } else if (arg == "--priorWeight" && i + 1 < argc) {
            cfg.priorWeight = std::stod(argv[++i]);
        } else if (arg == "--maxInterFrameTime" && i + 1 < argc) {
            cfg.maxInterFrameTime = std::stod(argv[++i]);
        } else if (arg == "--no-consistency") {
            cfg.enableConsistency = false;
        } else if (arg == "--no-dyn-thresh") {
            cfg.enableDynThresh = false;
        } else if (arg == "--no-dr") {
            cfg.enablePrediction = false;
        } else if (arg == "--profile") {
            profileMode = true;
        } else if (arg == "--calib") {
            calibMode = true;
        } else if ((arg == "--historySize" || arg == "--history-size" || arg == "--history") && i + 1 < argc) {
            cfg.historyWindow = std::stoul(argv[++i]);
        }
    }
    
    if (dirPath.empty()) {
        std::cerr << "Usage: " << argv[0] << " --dir <path> [--dr <model>] [--trust <model>] [--historySize <size>] [--profile] [--calib]\n";
        std::cerr << "Ablation flags: [--no-consistency] [--no-dyn-thresh] [--no-dr]\n";
        return 1;
    }

    // 1. Discover all trace files
    auto traceFiles = ParquetTraceReader::getTraceFiles(dirPath);
    if (traceFiles.empty()) {
        std::cerr << "Error: No traceJSON-*.parquet files found in " << dirPath << "\n";
        return 1;
    }

    // 2. Initialize engine
    bool engineSilent = profileMode;
    RunnerEngine engine(drModelName, trustModelName, cfg, engineSilent, calibMode);

    // 3. Process stream
    if (profileMode) {
        std::cout << "Loading " << traceFiles.size() << " trace files for profiling...\n";
        std::vector<std::vector<ReceivedVector>> fileBatches;
        fileBatches.reserve(traceFiles.size());
        size_t totalMessages = 0;
        for (const auto& fpath : traceFiles) {
            auto msgs = loadMotionVectors(fpath);
            totalMessages += msgs.size();
            fileBatches.push_back(std::move(msgs));
        }

        std::cout << "Starting performance profiling over " << totalMessages << " messages (" << traceFiles.size() << " observer nodes)...\n";
        auto start = std::chrono::high_resolution_clock::now();
        
        for (const auto& batch : fileBatches) {
            engine.reset();
            for (const auto& msg : batch) {
                engine.processMessage(msg);
            }
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
        
        std::cout << "----------------------------------------\n";
        std::cout << "PROFILING RESULTS\n";
        std::cout << "----------------------------------------\n";
        std::cout << "Observer Files:       " << traceFiles.size() << "\n";
        std::cout << "History Size:         " << cfg.historyWindow << "\n";
        std::cout << "Total Execution Time: " << duration << " us\n";
        std::cout << "Messages Processed:   " << totalMessages << "\n";
        std::cout << "Avg Latency per Msg:  " << (totalMessages > 0 ? (double)duration / totalMessages : 0.0) << " us\n";
        std::cout << "Throughput:           " << (duration > 0 ? (totalMessages * 1000000.0) / duration : 0.0) << " msgs/s\n";
        std::cout << "----------------------------------------\n";
    } else {
        std::cout << (calibMode ? "receiver,sender,dt,safeSpeed,yawRateHat,distError\n"
                                : "rcvTime,receiver,sender,score,distError\n");
        for (const auto& fpath : traceFiles) {
            auto msgs = loadMotionVectors(fpath);
            engine.reset();
            for (const auto& msg : msgs) {
                engine.processMessage(msg);
            }
        }
    }

    return 0;
}
