#!/usr/bin/env bash
# Getting-started smoke test: build MiniDB (stdin mode) and run a few SQL statements.
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"
BUILD_DIR="${REPO_ROOT}/build-getting-started"
BIN="${BUILD_DIR}/bin/minidb_bin"
DATA_DIR="${REPO_ROOT}/demo-data-getting-started"
OUT_FILE="${BUILD_DIR}/getting_started_out.txt"

rm -rf "${BUILD_DIR}" "${DATA_DIR}"
mkdir -p "${BUILD_DIR}" "${DATA_DIR}"

cmake -S "${REPO_ROOT}" -B "${BUILD_DIR}" -DENABLE_THREADPOOL=OFF >/dev/null
cmake --build "${BUILD_DIR}" -j >/dev/null

{
	echo "CREATE TABLE students (id INT, name STRING);"
	echo "INSERT INTO students (id, name) VALUES (1, Alice);"
	echo "INSERT INTO students (id, name) VALUES (2, Bob);"
	echo "SELECT id, name FROM students;"
} | "${BIN}" --datadir "${DATA_DIR}" >"${OUT_FILE}" 2>/dev/null

grep -q "TABLE students CREATED" "${OUT_FILE}"
grep -q "Alice" "${OUT_FILE}"
grep -q "Bob" "${OUT_FILE}"

echo "[PASS] Getting started: build + CREATE/INSERT/SELECT"
