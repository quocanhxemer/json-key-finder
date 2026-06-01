#!/usr/bin/env python3
"""
Benchmark runner for json-key-finder.

Generates key sets via `gen_keys`, then runs `findkey` across a configuration
matrix and writes results to CSV.

Two modes:
  - bench: performance (no --collect-stats)
  - stats: Teddy compilation + false-positive stats (--collect-stats)
  - both: run both and emit two CSVs

This script intentionally treats JSON inputs as opaque files; it does not parse
or inspect their content itself.
"""

from __future__ import annotations

import argparse
import csv
import datetime as _dt
import json
import os
import platform
import re
import shlex
import subprocess
import sys
import time
from concurrent.futures import ProcessPoolExecutor, as_completed
from dataclasses import dataclass
from pathlib import Path
from typing import Any, Iterable, Optional


FINDKEY_DEFAULT = Path("build/findkey")
GENKEYS_DEFAULT = Path("build/gen_keys")


_RE_KV = re.compile(r"^([A-Za-z0-9][A-Za-z0-9 \\-]*):\\s*(.*)$")


def _now_iso() -> str:
    return _dt.datetime.now(_dt.timezone.utc).astimezone().isoformat()


def _run(cmd: list[str], *, timeout_s: Optional[float] = None) -> subprocess.CompletedProcess[str]:
    return subprocess.run(
        cmd,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        text=True,
        timeout=timeout_s,
        check=False,
    )


def _ensure_file(path: Path, what: str) -> None:
    if not path.exists():
        raise SystemExit(f"{what} not found: {path}")
    if not path.is_file():
        raise SystemExit(f"{what} is not a file: {path}")


def _safe_stem(path: Path) -> str:
    # Keep names stable and filesystem-friendly.
    return re.sub(r"[^A-Za-z0-9._-]+", "_", path.stem)


def _parse_findkey_output(output: str) -> dict[str, Any]:
    """
    Parses stdout from `findkey` into a dict.

    Handles both normal mode and --collect-stats mode.
    """
    res: dict[str, Any] = {}
    lines = [ln.rstrip("\n") for ln in output.splitlines()]

    # Always present in normal successful runs
    # (collect-stats runs also print these lines in src/main.cpp)
    for ln in lines:
        if ln.startswith("Total key-value pairs found:"):
            # e.g. "Total key-value pairs found: 123"
            try:
                res["total_found"] = int(ln.split(":")[1].strip())
            except Exception:
                pass
        if ln.startswith("Compile time:"):
            res["compile_ns"] = _parse_float_tail(ln)
        if ln.startswith("Match time:"):
            res["match_ns"] = _parse_float_tail(ln)
        if ln.startswith("Time taken:"):
            res["total_ns"] = _parse_float_tail(ln)
        if ln.startswith("Data size:"):
            # "Data size: 12.34 MiB"
            res["data_mib"] = _parse_float_tail(ln)
        if ln.startswith("Throughput:"):
            res["throughput_mib_s"] = _parse_float_tail(ln)
        if ln.startswith("End-to-end throughput:"):
            res["e2e_throughput_mib_s"] = _parse_float_tail(ln)

    # Optional blocks for --collect-stats
    current_block: Optional[str] = None
    for ln in lines:
        if ln.strip() == "Teddy Compilation Stats:":
            current_block = "teddy_compilation"
            continue
        if ln.strip() == "Teddy Runtime Stats:":
            current_block = "teddy_runtime"
            continue
        if not ln.startswith("\t"):
            continue

        m = _RE_KV.match(ln.strip())
        if not m or not current_block:
            continue

        key = m.group(1).strip().lower().replace(" ", "_").replace("-", "_")
        val_raw = m.group(2).strip()

        # numeric coercion
        val: Any = val_raw
        if re.fullmatch(r"-?\\d+", val_raw):
            try:
                val = int(val_raw)
            except Exception:
                val = val_raw
        else:
            try:
                val = float(val_raw)
            except Exception:
                val = val_raw

        res[f"{current_block}__{key}"] = val

    return res


def _parse_float_tail(line: str) -> float:
    # "Compile time: 123.00 ns" -> 123.00
    # "Throughput: 456.78 MiB/s" -> 456.78
    tail = line.split(":", 1)[1].strip()
    # first token should be number
    token = tail.split()[0]
    return float(token)


@dataclass(frozen=True)
class KeyCase:
    json_path: Path
    key_type: str
    num_keys: int
    seed: int


@dataclass(frozen=True)
class TeddyConfig:
    grouping: str
    hash_algo: str
    suffix_mode: str
    sigma: int


@dataclass(frozen=True)
class FindkeyCase:
    mode: str  # bench | stats
    # scalar | teddy | teddy_baseline (ignored in stats by binary, but recorded)
    algo: str
    key_case: KeyCase
    keys_file: Path
    teddy: TeddyConfig
    repeat_index: int


def _gen_keys_if_needed(gen_keys_bin: Path, out_dir: Path, kc: KeyCase) -> Path:
    json_stem = _safe_stem(kc.json_path)
    keys_dir = out_dir / "keys" / json_stem
    keys_dir.mkdir(parents=True, exist_ok=True)

    out_file = keys_dir / f"{kc.key_type}-n{kc.num_keys}-seed{kc.seed}.txt"
    if out_file.exists() and out_file.stat().st_size > 0:
        return out_file

    cmd = [
        str(gen_keys_bin),
        str(kc.json_path),
        str(kc.num_keys),
        kc.key_type,
        str(out_file),
        str(kc.seed),
    ]
    cp = _run(cmd)
    if cp.returncode != 0:
        raise RuntimeError(
            "gen_keys failed\n"
            f"cmd: {shlex.join(cmd)}\n"
            f"exit: {cp.returncode}\n"
            f"output:\n{cp.stdout}"
        )
    return out_file


def _findkey_cmd(findkey_bin: Path, fc: FindkeyCase) -> list[str]:
    cmd = [
        str(findkey_bin),
        "--keys",
        str(fc.keys_file),
        "--data",
        str(fc.key_case.json_path),
    ]

    if fc.mode == "stats":
        cmd.append("--collect-stats")

    # algo selection is ignored by findkey when --collect-stats is on, but
    # we keep it explicit for readability/repro.
    cmd += ["--algo", fc.algo]

    if fc.algo != "scalar":
        cmd += [
            "--teddy-grouping-strategy",
            fc.teddy.grouping,
            "--teddy-suffix-mode",
            fc.teddy.suffix_mode,
            "--sigma",
            str(fc.teddy.sigma),
        ]
        if fc.teddy.grouping == "hash":
            cmd += ["--teddy-hash-algo", fc.teddy.hash_algo]

    return cmd


def _run_one(findkey_bin: str, fc: FindkeyCase, timeout_s: Optional[float]) -> dict[str, Any]:
    findkey_path = Path(findkey_bin)
    cmd = _findkey_cmd(findkey_path, fc)

    t0 = time.time()
    cp = _run(cmd, timeout_s=timeout_s)
    t1 = time.time()

    base: dict[str, Any] = {
        "timestamp": _now_iso(),
        "mode": fc.mode,
        "json_path": str(fc.key_case.json_path),
        "key_type": fc.key_case.key_type,
        "num_keys": fc.key_case.num_keys,
        "seed": fc.key_case.seed,
        "keys_file": str(fc.keys_file),
        "algo": fc.algo,
        "grouping_strategy": fc.teddy.grouping if fc.algo != "scalar" else "",
        "hash_algo": (fc.teddy.hash_algo if (fc.algo != "scalar" and fc.teddy.grouping == "hash") else ""),
        "suffix_mode": fc.teddy.suffix_mode if fc.algo != "scalar" else "",
        "sigma": (fc.teddy.sigma if fc.algo != "scalar" else ""),
        "repeat_index": fc.repeat_index,
        "exit_code": cp.returncode,
        "wall_time_s": t1 - t0,
    }

    if cp.returncode != 0:
        base["error_output"] = cp.stdout
        return base

    parsed = _parse_findkey_output(cp.stdout)
    base.update(parsed)
    return base


def _iter_key_cases(
    json_paths: list[Path],
    key_types: list[str],
    num_keys_list: list[int],
    seeds: list[int],
) -> list[KeyCase]:
    cases: list[KeyCase] = []
    for jp in json_paths:
        for kt in key_types:
            for nk in num_keys_list:
                for sd in seeds:
                    # "all" ignores num_keys, but we still record it for traceability
                    cases.append(
                        KeyCase(json_path=jp, key_type=kt, num_keys=nk, seed=sd))
    return cases


def _iter_teddy_configs(
    groupings: list[str],
    hash_algos: list[str],
    suffix_modes: list[str],
    sigmas: list[int],
) -> list[TeddyConfig]:
    res: list[TeddyConfig] = []
    for g in groupings:
        for sm in suffix_modes:
            for s in sigmas:
                if g == "hash":
                    for ha in hash_algos:
                        res.append(TeddyConfig(
                            grouping=g, hash_algo=ha, suffix_mode=sm, sigma=s))
                else:
                    res.append(TeddyConfig(
                        grouping=g, hash_algo="std", suffix_mode=sm, sigma=s))
    return res


def _write_csv(path: Path, rows: list[dict[str, Any]]) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    if not rows:
        raise SystemExit("No rows to write")

    # stable-ish column ordering: known keys first, then the rest sorted
    known = [
        "timestamp",
        "mode",
        "json_path",
        "key_type",
        "num_keys",
        "seed",
        "keys_file",
        "algo",
        "grouping_strategy",
        "hash_algo",
        "suffix_mode",
        "sigma",
        "repeat_index",
        "exit_code",
        "wall_time_s",
        "total_found",
        "compile_ns",
        "match_ns",
        "total_ns",
        "data_mib",
        "throughput_mib_s",
        "e2e_throughput_mib_s",
    ]
    all_keys = set().union(*(r.keys() for r in rows))
    extra = sorted(k for k in all_keys if k not in known)
    fieldnames = [k for k in known if k in all_keys] + extra

    with path.open("w", newline="") as f:
        w = csv.DictWriter(f, fieldnames=fieldnames)
        w.writeheader()
        for r in rows:
            w.writerow(r)


def _sysinfo() -> dict[str, Any]:
    return {
        "timestamp": _now_iso(),
        "platform": platform.platform(),
        "python": sys.version.replace("\n", " "),
        "cwd": os.getcwd(),
    }


def main(argv: Optional[list[str]] = None) -> int:
    p = argparse.ArgumentParser(
        description="Run findkey benchmarks across a config matrix and write CSV."
    )
    p.add_argument(
        "--mode", choices=["bench", "stats", "both"], default="both")
    p.add_argument("--findkey-bin", type=Path, default=FINDKEY_DEFAULT)
    p.add_argument("--gen-keys-bin", type=Path, default=GENKEYS_DEFAULT)
    p.add_argument("--out-dir", type=Path, default=Path("bench_out"))

    p.add_argument("--json", action="append", type=Path,
                   required=True, help="JSON input file (repeatable)")
    p.add_argument("--key-type", action="append", default=None,
                   help="gen_keys type (repeatable)")
    p.add_argument("--num-keys", action="append", type=int,
                   default=None, help="number of keys (repeatable)")
    p.add_argument("--seed", action="append", type=int,
                   default=None, help="RNG seed for gen_keys (repeatable)")

    p.add_argument("--algo", action="append",
                   default=None, help="algo (repeatable)")
    p.add_argument("--grouping", action="append", default=None,
                   help="teddy grouping (repeatable)")
    p.add_argument("--hash-algo", action="append",
                   default=None, help="hash algo (repeatable)")
    p.add_argument("--suffix-mode", action="append",
                   default=None, help="suffix mode (repeatable)")
    p.add_argument("--sigma", action="append", type=int,
                   default=None, help="sigma (repeatable)")

    p.add_argument("--repeats", type=int, default=5,
                   help="number of measured repeats per case")
    p.add_argument("--warmup", type=int, default=1,
                   help="warmup runs per case (discarded)")
    p.add_argument("--jobs", type=int, default=max(1, os.cpu_count() or 1))
    p.add_argument("--timeout-s", type=float, default=120.0)
    p.add_argument("--dry-run", action="store_true",
                   help="print planned run counts and exit")
    p.add_argument("--write-sysinfo", action="store_true",
                   help="write sysinfo.json into out dir")

    args = p.parse_args(argv)

    # Defaults for repeatable options (so user-specified values replace, not append-to, defaults).
    if args.key_type is None:
        args.key_type = ["highest", "mixed", "no_match"]
    if args.num_keys is None:
        args.num_keys = [16, 32, 60]
    if args.seed is None:
        args.seed = [1]
    if args.algo is None:
        args.algo = ["scalar", "teddy", "teddy_baseline"]
    if args.grouping is None:
        args.grouping = ["paper_greedy", "improved_greedy", "hash"]
    if args.hash_algo is None:
        args.hash_algo = ["std", "xxhash", "crc32", "fnv1a"]
    if args.suffix_mode is None:
        args.suffix_mode = ["raw", "quote-suffix"]
    if args.sigma is None:
        args.sigma = [1, 2, 3, 4]

    _ensure_file(args.findkey_bin, "findkey binary")
    _ensure_file(args.gen_keys_bin, "gen_keys binary")
    for jp in args.json:
        _ensure_file(jp, "json input")

    out_dir: Path = args.out_dir
    out_dir.mkdir(parents=True, exist_ok=True)

    if args.write_sysinfo:
        (out_dir / "sysinfo.json").write_text(
            json.dumps(_sysinfo(), ensure_ascii=False, indent=2) + "\n",
            encoding="utf-8",
        )

    # Expand matrix
    key_cases = _iter_key_cases(
        args.json, args.key_type, args.num_keys, args.seed)
    teddy_cfgs = _iter_teddy_configs(
        args.grouping, args.hash_algo, args.suffix_mode, args.sigma)

    def want_mode(m: str) -> bool:
        return args.mode in (m, "both")

    # Count runs without materializing the full list (can get huge).
    rep_total = args.warmup + args.repeats
    bench_runs = 0
    stats_runs = 0
    if want_mode("bench"):
        if "scalar" in args.algo:
            bench_runs += len(key_cases) * rep_total
        non_scalar_algos = [a for a in args.algo if a != "scalar"]
        bench_runs += len(key_cases) * len(non_scalar_algos) * \
            len(teddy_cfgs) * rep_total
    if want_mode("stats"):
        # stats always uses teddy_baseline internally; we run it once per teddy config
        stats_runs += len(key_cases) * len(teddy_cfgs) * rep_total

    if args.dry_run:
        print(f"Key cases: {len(key_cases)}")
        print(f"Teddy configs: {len(teddy_cfgs)}")
        print(
            f"Total runs: {bench_runs + stats_runs} (bench={bench_runs}, stats={stats_runs})")
        print(
            f"Warmup per case: {args.warmup}, repeats per case: {args.repeats}")
        return 0

    # Pre-generate key files once per key case.
    key_files: dict[KeyCase, Path] = {}
    for kc in key_cases:
        key_files[kc] = _gen_keys_if_needed(args.gen_keys_bin, out_dir, kc)

    # Execute in parallel, write rows separated by mode.
    bench_rows: list[dict[str, Any]] = []
    stats_rows: list[dict[str, Any]] = []

    # Warmups are the first indices [0..warmup-1] for each unique case key,
    # but we generated warmup+repeats linearly; simplest is to drop rows where
    # repeat_index < warmup after parsing.
    timeout_s = float(
        args.timeout_s) if args.timeout_s and args.timeout_s > 0 else None

    def iter_runs() -> Iterable[FindkeyCase]:
        for kc in key_cases:
            keys_file = key_files[kc]
            if want_mode("bench") and "scalar" in args.algo:
                for r in range(rep_total):
                    yield FindkeyCase(
                        mode="bench",
                        algo="scalar",
                        key_case=kc,
                        keys_file=keys_file,
                        teddy=TeddyConfig("paper_greedy", "std", "raw", 3),
                        repeat_index=r,
                    )

            non_scalar_algos = [a for a in args.algo if a != "scalar"]
            for algo in non_scalar_algos:
                for tc in teddy_cfgs:
                    if want_mode("bench"):
                        for r in range(rep_total):
                            yield FindkeyCase(
                                mode="bench",
                                algo=algo,
                                key_case=kc,
                                keys_file=keys_file,
                                teddy=tc,
                                repeat_index=r,
                            )

            if want_mode("stats"):
                for tc in teddy_cfgs:
                    for r in range(rep_total):
                        yield FindkeyCase(
                            mode="stats",
                            algo="teddy_baseline",
                            key_case=kc,
                            keys_file=keys_file,
                            teddy=tc,
                            repeat_index=r,
                        )

    max_in_flight = max(1, args.jobs * 4)

    with ProcessPoolExecutor(max_workers=args.jobs) as ex:
        in_flight = set()

        def submit_one(run: FindkeyCase) -> None:
            in_flight.add(ex.submit(_run_one, str(
                args.findkey_bin), run, timeout_s))

        run_iter = iter_runs()
        # Prime the queue
        for _ in range(max_in_flight):
            try:
                submit_one(next(run_iter))
            except StopIteration:
                break

        while in_flight:
            for fut in as_completed(in_flight, timeout=None):
                in_flight.remove(fut)
                row = fut.result()

                # discard warmups
                if int(row.get("repeat_index", 0)) < args.warmup:
                    pass
                else:
                    row["repeat_index"] = int(
                        row.get("repeat_index", 0)) - args.warmup
                    if row.get("mode") == "bench":
                        bench_rows.append(row)
                    else:
                        stats_rows.append(row)

                # Refill
                try:
                    submit_one(next(run_iter))
                except StopIteration:
                    pass
                break

    bench_rows.sort(key=lambda r: (r["json_path"], r["key_type"], r["num_keys"], r["seed"], r["algo"],
                    r["grouping_strategy"], r["hash_algo"], r["suffix_mode"], str(r["sigma"]), r["repeat_index"]))
    stats_rows.sort(key=lambda r: (r["json_path"], r["key_type"], r["num_keys"], r["seed"],
                    r["grouping_strategy"], r["hash_algo"], r["suffix_mode"], str(r["sigma"]), r["repeat_index"]))

    if want_mode("bench"):
        _write_csv(out_dir / "bench.csv", bench_rows)
    if want_mode("stats"):
        _write_csv(out_dir / "stats.csv", stats_rows)

    print(f"Wrote: {out_dir / 'bench.csv' if want_mode('bench') else ''} {out_dir / 'stats.csv' if want_mode('stats') else ''}".strip())
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
