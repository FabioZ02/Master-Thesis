#!/bin/bash

CONFIG_ID=$1
INSTANCE_ID=$2
SEED=$3
INSTANCE=$4

shift 4

START_ABS=""
MIN_ABS=""
PASS_ARGS=()
while [ $# -gt 0 ]; do
    case "$1" in
        --BSA::start_temperature) START_ABS="$2"; shift 2 ;;
        --BSA::min_temperature)   MIN_ABS="$2";   shift 2 ;;
        *) PASS_ARGS+=("$1" "$2"); shift 2 ;;
    esac
done


MIN_SAFE=$(python3 -c "s=$START_ABS; m=$MIN_ABS; print(m if m < s else s*0.001)")

OUT=$(/home/fabio/Master_Thesis/bt_main --main::instance "$INSTANCE" \
                --main::method BSA \
                --main::seed "$SEED" \
                --BSA::max_evaluations 10000000 \
                --main::irace-enable \
                --BSA::start_temperature "$START_ABS" \
                --BSA::min_temperature "$MIN_SAFE" \
                "${PASS_ARGS[@]}")

echo "$OUT" | tail -n 1
