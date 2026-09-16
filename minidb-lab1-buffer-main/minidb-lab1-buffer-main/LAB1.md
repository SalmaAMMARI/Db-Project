# Lab 1 — Getting started + Buffer Manager (CLOCK)

## Goals

1. **Getting started** — build MiniDB, run it, execute a few SQL queries.
2. **Buffer manager** — replace the provided **LRU** baseline with the **CLOCK** (second-chance) page replacement policy.

## Build

```bash
mkdir -p build && cd build
cmake -DENABLE_THREADPOOL=OFF ..
cmake --build . -j
```

## Part 1 — Getting started (manual + automated)

### Manual

From `build/`:

```bash
./bin/minidb_bin --datadir ../demo-data
```

Try:

```sql
CREATE TABLE students (id INT, name STRING);
INSERT INTO students (id, name) VALUES (1, Alice);
SELECT id, name FROM students;
```

Exit with `Ctrl+D`.

### Automated smoke test (used in CI)

```bash
chmod +x scripts/getting_started.sh
./scripts/getting_started.sh
```

This builds the DB and checks that CREATE / INSERT / SELECT work.

## Part 2 — Implement CLOCK

### Read

- `include/minidb/buffer_manager/buffer_manager.hpp`
- `src/buffer_manager/buffer_manager.cpp`
- `tests/test_buffer_manager.cpp`
- Overview: [CLOCK page replacement](https://en.wikipedia.org/wiki/Page_replacement_algorithm#Clock)

### Requirements

- Keep `fetch_page` / `unpin_page` / `flush_*` behavior.
- **Never** evict a frame with `pin_count > 0`.
- Flush dirty frames before reclaiming them.
- On hit / insert set the reference bit; eviction clears the bit (second chance) then evicts when the bit is already clear.
- Keep hot-path work O(1) / amortized O(1).

The handout ships a working **LRU** baseline so Part 1 and most buffer tests pass before you finish CLOCK. The test `test_clock_policy_second_chance` **fails on LRU on purpose** — it passes only with a correct CLOCK.

### Run buffer tests

```bash
cd build
./bin/test_buffer_manager
# or
ctest -R test_buffer_manager --output-on-failure
```

## Submit

1. Open a **Pull Request** into this repository’s `main`.
2. CI must be green: getting-started script + unit tests (including CLOCK).

## Grading signal (CI)

| Check | What it covers |
|-------|----------------|
| Getting started | `scripts/getting_started.sh` |
| Buffer tests | `test_buffer_manager` (basic, dirty eviction, pinned, CLOCK) |
| Full suite | `ctest --output-on-failure` |
