#!/usr/bin/env bash
set -euo pipefail

BIN_DIR="${1:?usage: smoke.sh <directory-containing-ecu_sim-and-tester>}"

"${BIN_DIR}/ecu_sim" &
ECU_PID=$!

cleanup() {
  kill "${ECU_PID}" 2>/dev/null || true
}
trap cleanup EXIT

sleep 0.5
"${BIN_DIR}/tester"
