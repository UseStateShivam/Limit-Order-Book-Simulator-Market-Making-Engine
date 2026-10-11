#!/usr/bin/env python3
"""
Repeatable Experiment Runner for Limit Order Book & Market-Making Engine.
Executes parameter sweeps and comparative strategy evaluations.
"""

import subprocess
import os
import sys
import json
from pathlib import Path

ROOT_DIR = Path(__file__).resolve().parent.parent

if os.name == "nt":
    os.environ["PATH"] = r"C:\msys64\ucrt64\bin;" + os.environ.get("PATH", "")

# Detect binary name
BIN_DIR = ROOT_DIR / "build"
EXE_NAME = "lob_sim.exe" if os.name == "nt" else "lob_sim"
EXE_PATH = BIN_DIR / EXE_NAME
if not EXE_PATH.exists():
    # Check if inside build/release or build/Debug
    for candidate in [BIN_DIR / "Release" / EXE_NAME, BIN_DIR / "Debug" / EXE_NAME, ROOT_DIR / "build" / "release" / EXE_NAME]:
        if candidate.exists():
            EXE_PATH = candidate
            break

def run_experiment(config_file, output_dir, seed=42, scenario=None):
    cmd = [str(EXE_PATH), "run-simulation", "--config", str(config_file), "--output", str(output_dir), "--seed", str(seed)]
    if scenario:
        cmd.extend(["--scenario", scenario])
    
    print(f"Running: {' '.join(cmd)}")
    res = subprocess.run(cmd, capture_output=True, text=True)
    if res.returncode != 0:
        print(f"Error executing experiment:\n{res.stderr}")
        return None
    
    summary_path = Path(output_dir) / "summary.json"
    if summary_path.exists():
        with open(summary_path, "r") as f:
            return json.load(f)
    return None

def main():
    print("=== Limit Order Book Experiment Runner ===")
    if not EXE_PATH.exists():
        print(f"Error: Executable not found at {EXE_PATH}. Please build the project first.")
        sys.exit(1)

    experiments = [
        {"name": "baseline_stable", "config": "configs/strategy_baseline.yaml", "scenario": "stable"},
        {"name": "fixed_spread_stable", "config": "configs/strategy_fixed_spread.yaml", "scenario": "stable"},
        {"name": "liquidity_aware_imbalance", "config": "configs/strategy_liquidity_aware.yaml", "scenario": "liquidity_imbalance"},
        {"name": "baseline_toxic_flow", "config": "configs/synthetic_toxic_flow.yaml", "scenario": "toxic_flow"}
    ]

    results = []
    results_dir = ROOT_DIR / "results" / "comparative_experiments"
    results_dir.mkdir(parents=True, exist_ok=True)

    for exp in experiments:
        cfg_path = ROOT_DIR / exp["config"]
        out_path = results_dir / exp["name"]
        data = run_experiment(cfg_path, out_path, scenario=exp["scenario"])
        if data:
            results.append({
                "experiment": exp["name"],
                "strategy": data.get("strategy_type"),
                "scenario": data.get("scenario"),
                "net_pnl": data.get("financials", {}).get("net_pnl"),
                "max_drawdown_pct": data.get("risk", {}).get("max_drawdown_pct"),
                "fill_rate_pct": data.get("execution", {}).get("fill_rate_pct"),
                "executed_volume": data.get("execution", {}).get("executed_volume"),
                "avg_spread_captured": data.get("execution", {}).get("avg_spread_captured")
            })

    print("\n=== Comparative Experiment Results ===")
    print(f"{'Experiment':<28} | {'Strategy':<18} | {'Net P&L ($)':<12} | {'Max DD (%)':<10} | {'Fill Rate (%)':<14} | {'Volume':<8}")
    print("-" * 105)
    for r in results:
        print(f"{r['experiment']:<28} | {r['strategy']:<18} | {r['net_pnl']:<12.2f} | {r['max_drawdown_pct']:<10.2f} | {r['fill_rate_pct']:<14.2f} | {r['executed_volume']:<8}")

if __name__ == "__main__":
    main()
