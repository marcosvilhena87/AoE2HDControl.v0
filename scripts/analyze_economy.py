#!/usr/bin/env python3
"""Summarize observed net resource changes per (run_id, session). Python stdlib only."""
import argparse
import csv
from collections import defaultdict
from pathlib import Path

RESOURCES = ("Food", "Wood", "Stone", "Gold")


def analyze(path):
    sessions = {}
    with open(path, newline="", encoding="utf-8-sig") as handle:
        reader = csv.DictReader(handle)
        required = {"elapsed_ms", "session", "event", "resource", "current"}
        if not required.issubset(reader.fieldnames or []):
            raise ValueError("CSV missing required columns: " + ", ".join(sorted(required - set(reader.fieldnames or []))))
        for line, row in enumerate(reader, 2):
            try:
                session_id = int(row["session"])
                elapsed = int(row["elapsed_ms"])
                if elapsed < 0 or session_id < 0:
                    raise ValueError("negative elapsed/session")
                run_id = row.get("run_id") or "legacy"
                event = row["event"].strip()
                key = (run_id, session_id)
                if session_id == 0 and event == "unavailable":
                    continue
                state = sessions.setdefault(key, {
                    "run_id": run_id, "session": session_id, "start_ms": None,
                    "end_ms": None, "resources": {
                        r: {"initial": None, "final": None, "increases": 0.0,
                            "decreases": 0.0, "events": 0} for r in RESOURCES
                    }
                })
                if event not in ("initial", "change", "unavailable"):
                    continue
                if event == "unavailable":
                    if state["start_ms"] is not None:
                        state["end_ms"] = max(state["end_ms"] or 0, elapsed)
                    continue
                resource = row["resource"]
                if resource not in RESOURCES:
                    continue
                if not row["current"]:
                    continue
                current = float(row["current"])
                r = state["resources"][resource]
                if event == "initial":
                    r["initial"] = current
                    r["final"] = current
                    if state["start_ms"] is None:
                        state["start_ms"] = elapsed
                    state["end_ms"] = max(state["end_ms"] or 0, elapsed)
                elif event == "change":
                    if r["initial"] is None:
                        continue
                    previous = float(row["previous"])
                    delta = current - previous
                    if abs(delta) < 0.005:
                        continue
                    r["increases"] += max(delta, 0)
                    r["decreases"] += max(-delta, 0)
                    r["final"] = current
                    r["events"] += 1
                    state["end_ms"] = max(state["end_ms"] or 0, elapsed)
            except (ValueError, TypeError) as exc:
                raise ValueError(f"Invalid data at CSV line {line}: {exc}") from exc
    summary = []
    for key in sorted(sessions):
        state = sessions[key]
        if state["start_ms"] is None:
            continue
        duration_ms = max(0, (state["end_ms"] or 0) - state["start_ms"])
        for resource in RESOURCES:
            r = state["resources"][resource]
            if r["initial"] is None:
                continue
            net = r["final"] - r["initial"]
            summary.append({
                "run_id": state["run_id"],
                "session": state["session"],
                "resource": resource,
                "initial": round(r["initial"], 2),
                "final": round(r["final"], 2),
                "increases_observed": round(r["increases"], 2),
                "decreases_observed": round(r["decreases"], 2),
                "net": round(net, 2),
                "duration_seconds": round(duration_ms / 1000, 3),
                "net_per_minute": round(net * 60000 / duration_ms, 2) if duration_ms else 0.0,
                "change_events": r["events"],
            })
    return summary


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("csv_file", type=Path)
    parser.add_argument("--output", type=Path, help="Export summary as CSV")
    args = parser.parse_args()
    rows = analyze(args.csv_file)
    for row in rows:
        print(f"Run={row['run_id']} Session={row['session']} {row['resource']}: "
              f"{row['initial']:.2f} -> {row['final']:.2f} "
              f"net={row['net']:+.2f} increased={row['increases_observed']:.2f} "
              f"decreased={row['decreases_observed']:.2f} "
              f"net/min={row['net_per_minute']:+.2f} "
              f"events={row['change_events']}")
    if args.output:
        fieldnames = ("run_id", "session", "resource", "initial", "final",
                      "increases_observed", "decreases_observed", "net",
                      "duration_seconds", "net_per_minute", "change_events")
        with open(args.output, "w", newline="", encoding="utf-8") as out:
            writer = csv.DictWriter(out, fieldnames=fieldnames)
            writer.writeheader()
            writer.writerows(rows)
    if not rows:
        print("No completed initial resource snapshots found.")


if __name__ == "__main__":
    main()
