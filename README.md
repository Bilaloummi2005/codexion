*This project has been created as part of the 42 curriculum by <your_login>.*

## Description

Codexion is a concurrency simulation inspired by the Dining Philosophers problem, re-themed around coders competing for shared USB dongles in a co-working hub. Each coder is a thread that cycles through three phases — compiling (requiring two adjacent dongles), debugging, and refactoring — while a monitor thread watches for burnout. The simulation supports two scheduling policies for dongle arbitration: FIFO (First In, First Out) and EDF (Earliest Deadline First). The goal is to coordinate resource access so that no coder starves or burns out, using POSIX threads, mutexes, and condition variables.

## Instructions

### Compilation

```bash
make
```

This produces the `codexion` binary. The Makefile compiles all source files with `cc -Wall -Wextra -Werror -pthread`.

### Usage

```bash
./codexion number_of_coders time_to_burnout time_to_compile time_to_debug time_to_refactor number_of_compiles_required dongle_cooldown scheduler
```

All arguments are mandatory and must be positive integers, except `scheduler` which must be either `fifo` or `edf`.

**Parameters:**

- `number_of_coders` — number of coders (and dongles) in the simulation.
- `time_to_burnout` (ms) — maximum time a coder can go without starting a compile before burning out.
- `time_to_compile` (ms) — duration of the compiling phase (requires two dongles).
- `time_to_debug` (ms) — duration of the debugging phase.
- `time_to_refactor` (ms) — duration of the refactoring phase.
- `number_of_compiles_required` — simulation ends successfully when every coder has compiled at least this many times.
- `dongle_cooldown` (ms) — minimum time a dongle must rest after being released before it can be taken again.
- `scheduler` — arbitration policy: `fifo` or `edf`.

### Examples

```bash
# 5 coders, 800ms burnout, 200ms compile, 200ms debug, 200ms refactor, 7 compiles each, 0ms cooldown, FIFO
./codexion 5 800 200 200 200 7 0 fifo

# 4 coders with EDF scheduling and 60ms dongle cooldown
./codexion 4 500 200 100 100 10 60 edf
```

## Blocking cases handled

### Deadlock prevention

Deadlock requires four Coffman conditions to hold simultaneously: mutual exclusion, hold and wait, no preemption, and circular wait. This implementation breaks the circular wait condition by imposing a consistent lock ordering on dongle acquisition. Odd-numbered coders acquire their right dongle first, then their left; even-numbered coders acquire left first, then right. Since adjacent coders always contend in the same global order, no circular dependency can form.

For the single-coder edge case, the coder can only ever acquire one dongle (since left and right point to the same dongle), so `lock_dongle` detects `n_coders == 1` and returns early after taking the first dongle, preventing the thread from attempting to lock the same mutex twice.

### Starvation prevention

Under FIFO scheduling, each dongle maintains a queue that grants access strictly in arrival order. Under EDF scheduling, a min-heap priority queue grants access to the coder with the earliest burnout deadline (`last_compile_start + time_to_burnout`). Deadlines are recalculated dynamically while waiting, so a coder that has gone longer without compiling naturally rises in priority, preventing indefinite starvation.

### Dongle cooldown

Each dongle tracks the timestamp of its last release (`last_release`). A waiting coder can only acquire the dongle once `get_time() - last_release >= dongle_cooldown`. This is checked inside the wait loop, and the coder sleeps via `pthread_cond_timedwait` until the condition is met.

### Precise burnout detection

A dedicated monitor thread polls every coder's `last_compile` timestamp once per millisecond. When it detects that `get_time() - last_compile > time_to_burnout` for any coder, it sets a global `burned_out` flag and prints the burnout message within the 10ms precision requirement. All coder threads check this flag at every decision point and exit cleanly.

### Log serialization

All log output is protected by a single `log_mutex`. Before printing, `log_state` checks whether the simulation has already ended (via `burned_out` flag) to suppress stray messages after burnout. The "burned out" message itself bypasses this check so it is always printed.

## Thread synchronization mechanisms

### Mutexes

- **Dongle mutexes** (`pthread_mutex_t` per dongle): Protect each dongle's queue/heap state, `last_release` timestamp, and `queue_size`/`edf_size`. A coder holds a dongle's mutex for the entire duration of waiting in the queue and only releases it after acquiring the dongle or aborting due to burnout.
- **Log mutex** (`log_mutex`): Serializes all `printf` output so that no two log lines interleave on a single line.
- **Burned mutex** (`burned_mutex`): Protects the `burned_out` and `completed` flags, which are written by the monitor and read by all coder threads. All reads and writes go through `is_burned_out()`/`set_burned_out()` and `is_completed()`/`set_completed()` accessor functions.
- **State mutex** (`state_mutex` per coder): Protects each coder's `last_compile` and `compile_count` fields, which are written by the coder thread and read by the monitor thread.

### Condition variables

Each dongle has a `pthread_cond_t` used with `pthread_cond_timedwait` so that waiting coders sleep efficiently instead of busy-spinning. When a dongle is released, `pthread_cond_broadcast` wakes all waiters so they can re-evaluate queue position and cooldown. The timed wait uses a 1ms timeout to ensure responsive burnout checks even if no signal arrives.

### Race condition prevention

The monitor reads `last_compile` and `compile_count` under each coder's `state_mutex`, while coder threads write these fields under the same lock. The `burned_out` flag is protected by `burned_mutex` and checked atomically at every critical decision point in the coder routine: before entering the dongle queue, after acquiring dongles, and between each phase (compile, debug, refactor). This ensures no coder continues operating after burnout is declared.

### EDF heap thread safety

The EDF min-heap is always accessed while holding the parent dongle's mutex, so no additional locking is needed for heap operations. `heap_push`, `heap_pop`, `heap_update`, and `heap_remove_by_id` all execute within the critical section of `lock_dongle`, ensuring consistent heap state across concurrent coder threads.

## Resources

- [The Dining Philosophers Problem — Wikipedia](https://en.wikipedia.org/wiki/Dining_philosophers_problem)
- [POSIX Threads Programming — LLNL](https://hpc-tutorials.llnl.gov/posix/)
- [pthread_cond_timedwait — Open Group](https://pubs.opengroup.org/onlinepubs/009695399/functions/pthread_cond_timedwait.html)
- [Earliest Deadline First Scheduling — Wikipedia](https://en.wikipedia.org/wiki/Earliest_deadline_first_scheduling)
- [Binary Heap — Wikipedia](https://en.wikipedia.org/wiki/Binary_heap)
- [Coffman Conditions for Deadlock — Wikipedia](https://en.wikipedia.org/wiki/Deadlock#Necessary_conditions)

### AI usage

AI (Claude) was used as a debugging assistant during development: identifying concurrency issues (lock ordering problems, stray log messages after burnout), reviewing heap operation correctness (`heap_update` index bug after `heap_bubble_up`), and clarifying POSIX threading semantics. All code was written, understood, and validated by the author. AI was not used to generate complete functions or modules.