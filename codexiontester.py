#!/usr/bin/env python3
"""
Codexion simulation replay & validator.

Usage:
    ./codexion 3 530 200 100 100 10 60 edf | python3 codexion_replay.py 530
    ./codexion 5 800 200 200 200 7 60 fifo | python3 codexion_replay.py 800

    First argument: time_to_burnout (ms) — used to draw burnout bars.

Or replay from a file:
    python3 codexion_replay.py 530 < output.txt
"""

import sys
import time
import os

# ── ANSI colors ──────────────────────────────────────────────────────
RESET  = "\033[0m"
BOLD   = "\033[1m"
DIM    = "\033[2m"
RED    = "\033[91m"
GREEN  = "\033[92m"
YELLOW = "\033[93m"
BLUE   = "\033[94m"
CYAN   = "\033[96m"
GRAY   = "\033[90m"
BG_RED = "\033[41m"
WHITE  = "\033[97m"

STATE_COLORS = {
    "idle":        GRAY,
    "waiting":     YELLOW,
    "compiling":   GREEN,
    "debugging":   RED,
    "refactoring": BLUE,
    "burned_out":  BG_RED + WHITE,
}

STATE_ICONS = {
    "idle":        "○",
    "waiting":     "◔",
    "compiling":   "⚙",
    "debugging":   "🔍",
    "refactoring": "♻",
    "burned_out":  "✖",
}

# ── Parse events ─────────────────────────────────────────────────────
def parse_events(lines):
    events = []
    for line in lines:
        line = line.strip()
        if not line:
            continue
        # Strip ANSI escape codes
        import re
        clean = re.sub(r'\x1b\[[0-9;]*m', '', line)
        parts = clean.split(None, 2)
        if len(parts) < 3:
            continue
        try:
            t = int(parts[0])
        except ValueError:
            continue
        coder_id = int(parts[1])
        msg = parts[2]
        events.append((t, coder_id, msg))
    return events


# ── Build state snapshots ────────────────────────────────────────────
def build_coders(events):
    ids = set()
    for _, cid, _ in events:
        ids.add(cid)
    return sorted(ids)


def state_at(events, t, coder_ids):
    states = {}
    for cid in coder_ids:
        states[cid] = {"state": "idle", "last_compile": 0, "dongles": 0, "compiles": 0}

    for et, cid, msg in events:
        if et > t:
            break
        c = states[cid]
        if msg == "has taken a dongle":
            c["dongles"] = c.get("dongles", 0) + 1
            c["state"] = "waiting"
        elif msg == "is compiling":
            c["state"] = "compiling"
            c["last_compile"] = et
            c["dongles"] = 0
        elif msg == "is debugging":
            c["state"] = "debugging"
        elif msg == "is refactoring":
            c["state"] = "refactoring"
        elif msg == "burned out":
            c["state"] = "burned_out"
    return states


# ── Validation ───────────────────────────────────────────────────────
def validate(events, burnout_ms):
    errors = []
    warnings = []
    coder_ids = build_coders(events)

    prev_t = -1
    for i, (t, cid, msg) in enumerate(events):
        # Timestamps must be non-decreasing
        if t < prev_t:
            errors.append(f"Line {i+1}: timestamp {t} < previous {prev_t} (out of order)")
        prev_t = t

    # Check burnout accuracy
    states = {}
    for cid in coder_ids:
        states[cid] = {"last_compile": 0}

    for et, cid, msg in events:
        if msg == "is compiling":
            states[cid]["last_compile"] = et
        elif msg == "burned out":
            elapsed = et - states[cid]["last_compile"]
            if elapsed < burnout_ms:
                errors.append(
                    f"Coder {cid} burned out at {et}ms but only {elapsed}ms "
                    f"since last compile (burnout={burnout_ms}ms) — too early!"
                )
            elif elapsed > burnout_ms + 10:
                warnings.append(
                    f"Coder {cid} burned out at {et}ms, {elapsed}ms since last compile "
                    f"(burnout={burnout_ms}ms) — {elapsed - burnout_ms}ms late (>10ms tolerance)"
                )

    # Check that each "is compiling" is preceded by exactly 2 "has taken a dongle"
    dongle_count = {}
    for cid in coder_ids:
        dongle_count[cid] = 0
    for et, cid, msg in events:
        if msg == "has taken a dongle":
            dongle_count[cid] += 1
        elif msg == "is compiling":
            if dongle_count[cid] != 2:
                errors.append(
                    f"Coder {cid} at {et}ms: 'is compiling' with {dongle_count[cid]} "
                    f"dongle(s) taken (expected 2)"
                )
            dongle_count[cid] = 0

    # Check no messages after burnout
    burned = False
    burn_time = None
    for et, cid, msg in events:
        if msg == "burned out":
            burned = True
            burn_time = et
        elif burned and et > burn_time:
            errors.append(f"Coder {cid} at {et}ms: message after burnout at {burn_time}ms")

    return errors, warnings


# ── Render frame ─────────────────────────────────────────────────────
def render_frame(t, states, coder_ids, burnout_ms, total_ms):
    try:
        cols = os.get_terminal_size().columns
    except OSError:
        cols = 80
    bar_width = min(30, (cols - 30) // len(coder_ids))

    lines = []
    lines.append(f"\033[2J\033[H")  # clear screen

    # Header
    progress = t / max(total_ms, 1)
    prog_bar_w = min(cols - 20, 50)
    filled = int(progress * prog_bar_w)
    prog_bar = f"{'█' * filled}{'░' * (prog_bar_w - filled)}"
    lines.append(f" {BOLD}Codexion Replay{RESET}  {CYAN}{t:>6}ms{RESET}  {DIM}{prog_bar}{RESET}")
    lines.append("")

    # Coders
    for cid in coder_ids:
        c = states[cid]
        st = c["state"]
        col = STATE_COLORS.get(st, RESET)
        icon = STATE_ICONS.get(st, "?")

        # Burnout bar
        elapsed = t - c["last_compile"]
        pct = min(elapsed / max(burnout_ms, 1), 1.0)
        filled = int(pct * bar_width)
        empty = bar_width - filled

        if pct > 0.8:
            bar_col = RED
        elif pct > 0.5:
            bar_col = YELLOW
        else:
            bar_col = GREEN

        bar = f"{bar_col}{'█' * filled}{DIM}{'░' * empty}{RESET}"
        pct_str = f"{pct * 100:5.1f}%"

        lines.append(
            f"  {col}{icon} Coder {cid:>2}{RESET}  "
            f"{col}{st:<13}{RESET}  "
            f"[{bar}] {pct_str}  "
            f"{DIM}last_compile={c['last_compile']}ms{RESET}"
        )

    lines.append("")
    return "\n".join(lines)


# ── Main ─────────────────────────────────────────────────────────────
def main():
    if len(sys.argv) < 2:
        print("Usage: ./codexion <args> | python3 codexion_replay.py <time_to_burnout>")
        print("   or: python3 codexion_replay.py <time_to_burnout> < output.txt")
        sys.exit(1)

    burnout_ms = int(sys.argv[1])

    # Read all input
    raw_lines = sys.stdin.readlines()
    events = parse_events(raw_lines)

    if not events:
        print("No events parsed. Check your input.")
        sys.exit(1)

    coder_ids = build_coders(events)
    total_ms = events[-1][0]

    # ── Validate ──
    errors, warnings = validate(events, burnout_ms)

    # ── Animate ──
    speed = 0.01  # 1ms of simulation = 0.01s real time (100x slower)
    unique_times = sorted(set(e[0] for e in events))

    try:
        for t in unique_times:
            states = state_at(events, t, coder_ids)
            frame = render_frame(t, states, coder_ids, burnout_ms, total_ms)
            sys.stdout.write(frame)

            # Print events at this timestamp
            for et, cid, msg in events:
                if et == t:
                    col = STATE_COLORS.get("compiling" if "compil" in msg else
                                           "debugging" if "debug" in msg else
                                           "refactoring" if "refact" in msg else
                                           "waiting" if "dongle" in msg else
                                           "burned_out" if "burned" in msg else
                                           "idle", RESET)
                    sys.stdout.write(f"  {col}{et:>6} {cid} {msg}{RESET}\n")

            sys.stdout.flush()

            # Wait proportional to time gap
            if t < total_ms:
                next_t = total_ms
                for ut in unique_times:
                    if ut > t:
                        next_t = ut
                        break
                gap = next_t - t
                time.sleep(gap * speed)

    except KeyboardInterrupt:
        pass

    # ── Final summary ──
    print()
    print(f" {BOLD}{'═' * 50}{RESET}")
    print(f" {BOLD}Validation summary{RESET}")
    print(f" {BOLD}{'═' * 50}{RESET}")
    print(f"  Events parsed: {len(events)}")
    print(f"  Coders:        {len(coder_ids)} ({', '.join(str(c) for c in coder_ids)})")
    print(f"  Duration:      {total_ms}ms")
    print()

    if errors:
        print(f"  {RED}{BOLD}ERRORS ({len(errors)}):{RESET}")
        for e in errors:
            print(f"    {RED}✖ {e}{RESET}")
    else:
        print(f"  {GREEN}✔ No errors{RESET}")

    if warnings:
        print(f"  {YELLOW}WARNINGS ({len(warnings)}):{RESET}")
        for w in warnings:
            print(f"    {YELLOW}⚠ {w}{RESET}")
    else:
        print(f"  {GREEN}✔ No warnings{RESET}")

    print()

    # Print raw log with colors
    print(f" {BOLD}Raw log:{RESET}")
    for et, cid, msg in events:
        col = STATE_COLORS.get("compiling" if "compil" in msg else
                               "debugging" if "debug" in msg else
                               "refactoring" if "refact" in msg else
                               "waiting" if "dongle" in msg else
                               "burned_out" if "burned" in msg else
                               "idle", RESET)
        print(f"  {col}{et:>6} {cid} {msg}{RESET}")

    if errors:
        sys.exit(1)


if __name__ == "__main__":
    main()