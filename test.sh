#!/bin/bash

# Usage: ./codexion <n_coders> <time_to_burnout> <time_to_compile>
#                   <time_to_debug> <time_to_refactor>
#                   <number_of_compiles_required> <dongle_cooldown> <scheduler>

BIN="./codexion"
PASS=0
FAIL=0

run() {
	local desc="$1"
	local expect="$2"   # "exit0", "exit1", "burnout", "alive"
	local timeout="$3"
	shift 3
	local args="$*"

	output=$(timeout "$timeout" $BIN $args 2>&1)
	code=$?

	if   [ "$expect" = "exit1"   ] && [ $code -ne 0 ] && [ $code -ne 124 ]; then
		echo "[PASS] $desc"
		PASS=$((PASS + 1))
	elif [ "$expect" = "exit0"   ] && [ $code -eq 0 ]; then
		echo "[PASS] $desc"
		PASS=$((PASS + 1))
	elif [ "$expect" = "burnout" ] && echo "$output" | grep -qi "burned\|died\|burnout"; then
		echo "[PASS] $desc"
		PASS=$((PASS + 1))
	elif [ "$expect" = "alive"   ] && [ $code -eq 0 ] && ! echo "$output" | grep -qi "burned\|died\|burnout"; then
		echo "[PASS] $desc"
		PASS=$((PASS + 1))
	else
		echo "[FAIL] $desc  (exit=$code, expected=$expect)"
		echo "       args: $args"
		FAIL=$((FAIL + 1))
	fi
}

make -C "$(dirname "$BIN")" re 2>/dev/null || { echo "Build failed"; exit 1; }
echo ""
echo "═══════════════════════════ INVALID ARGS ═══════════════════════════"

run "no args"                      exit1  2
run "too few args"                 exit1  2  5 800 200 200 200
run "too many args"                exit1  2  5 800 200 200 200 3 50 fifo extra
run "negative n_coders"            exit1  2  -1 800 200 200 200 3 50 fifo
run "zero n_coders"                exit1  2  0 800 200 200 200 3 50 fifo
run "zero time_to_burnout"         exit1  2  4 0 200 200 200 3 50 fifo
run "zero time_to_compile"         exit1  2  4 800 0 200 200 3 50 fifo
run "zero time_to_debug"           exit1  2  4 800 200 0 200 3 50 fifo
run "zero time_to_refactor"        exit1  2  4 800 200 200 0 3 50 fifo
run "zero compiles_required"       exit1  2  4 800 200 200 200 0 50 fifo
run "negative cooldown"            exit1  2  4 800 200 200 200 3 -1 fifo
run "invalid scheduler"            exit1  2  4 800 200 200 200 3 50 rr
run "float arg"                    exit1  2  4 800.5 200 200 200 3 50 fifo
run "letter arg"                   exit1  2  abc 800 200 200 200 3 50 fifo
run "overflow arg"                 exit1  2  4 9999999999 200 200 200 3 50 fifo

echo ""
echo "════════════════════════════ 1 CODER ═══════════════════════════════"
# 1 coder can never grab both dongles → always burns out

run "1 coder fifo burns out"       burnout  5   1 800 200 200 200 3 50 fifo
run "1 coder edf burns out"        burnout  5   1 800 200 200 200 3 50 edf

echo ""
echo "═══════════════════════════ MUST DIE ═══════════════════════════════"
# burnout < compile+debug+refactor → coder can never finish a cycle in time

run "2 coders burnout too tight fifo"   burnout  5   2 100 200 200 200 3 0 fifo
run "2 coders burnout too tight edf"    burnout  5   2 100 200 200 200 3 0 edf
run "5 coders burnout too tight fifo"   burnout  5   5 100 200 200 200 3 0 fifo
run "5 coders burnout too tight edf"    burnout  5   5 100 200 200 200 3 0 edf

echo ""
echo "═══════════════════════════ MUST LIVE ══════════════════════════════"
# burnout comfortably > compile+debug+refactor, compiles_required low → should finish

run "2 coders fifo 1 compile"      alive   10   2 800 200 200 200 1 0 fifo
run "2 coders edf  1 compile"      alive   10   2 800 200 200 200 1 0 edf
run "4 coders fifo 3 compiles"     alive   15   4 800 200 200 200 3 50 fifo
run "4 coders edf  3 compiles"     alive   15   4 800 200 200 200 3 50 edf
run "5 coders fifo 5 compiles"     alive   20   5 900 200 200 200 5 50 fifo
run "5 coders edf  5 compiles"     alive   20   5 900 200 200 200 5 50 edf

echo ""
echo "═══════════════════════════ EDGE TIMING ════════════════════════════"
# burnout == compile+debug+refactor → borderline, expect burnout

run "burnout equals cycle fifo"    burnout  8   4 600 200 200 200 3 0 fifo
run "burnout equals cycle edf"     burnout  8   4 600 200 200 200 3 0 edf

# very generous burnout

run "huge burnout 4 fifo"          alive   15   4 9000 200 200 200 2 0 fifo
run "huge burnout 4 edf"           alive   15   4 9000 200 200 200 2 0 edf

echo ""
echo "════════════════════════════ COOLDOWN ══════════════════════════════"

run "zero cooldown 4 fifo"         alive   15   4 800 200 200 200 3 0 fifo
run "zero cooldown 4 edf"          alive   15   4 800 200 200 200 3 0 edf
run "large cooldown 4 fifo"        burnout  8   4 400 200 200 200 3 300 fifo
run "large cooldown 4 edf"         burnout  8   4 400 200 200 200 3 300 edf

echo ""
echo "════════════════════════════ STRESS ════════════════════════════════"

run "100 coders fifo 1 compile"    alive   30  100 800 200 200 200 1 0 fifo
run "100 coders edf  1 compile"    alive   30  100 800 200 200 200 1 0 edf
run "200 coders fifo 1 compile"    alive   45  200 800 200 200 200 1 0 fifo
run "200 coders edf  1 compile"    alive   45  200 800 200 200 200 1 0 edf

echo ""
echo "═══════════════════════════ SUMMARY ════════════════════════════════"
echo "  PASSED: $PASS"
echo "  FAILED: $FAIL"
echo "  TOTAL:  $((PASS + FAIL))"
echo ""
[ $FAIL -eq 0 ] && exit 0 || exit 1