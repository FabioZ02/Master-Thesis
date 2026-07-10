#!/bin/bash
# ============================ DA IMPOSTARE ==================================
INSTDIR="$HOME/Master_Thesis/Instances/small/tuning_set"
START_IDX=1
END_IDX=100              
SEEDS=5
MAX_EVAL=1000000
VALIDATOR="$HOME/Master_Thesis/btsp-validator-binary/btsp-validator.dll"
BIN="$HOME/Master_Thesis/bt_main"
OUTCSV="$HOME/Master_Thesis/bench99_Small.csv"

START_TEMP=16.5194
MIN_TEMP=0.0123
COOLING=0.9933
NAR=0.2702
SWAP=0.5712
# ===========================================================================

mapfile -t ALL < <(ls "$INSTDIR"/*.json | sort -V)
COUNT=$((END_IDX - START_IDX + 1))
INSTANCES=("${ALL[@]:$((START_IDX-1)):$COUNT}")

# header solo se il file non esiste (per permettere ripresa in append)
[ -f "$OUTCSV" ] || echo "istanza,min,mean,median,std,feasible_runs,total_runs" > "$OUTCSV"

printf "%-18s %8s %10s %10s %9s %s\n" "istanza" "min" "mean" "median" "std" "feas/tot"

for INST in "${INSTANCES[@]}"; do
    NAME=$(basename "$INST" .json)
    COSTS=(); FEAS=0
    for ((s=1; s<=SEEDS; s++)); do
        SEED=$RANDOM
        OUT="/tmp/${NAME}_L_s${s}.json"
        "$BIN" --main::instance "$INST" --main::method BSA --main::seed "$SEED" \
               --main::swap_rate "$SWAP" --main::output_file "$OUT" \
               --BSA::start_temperature "$START_TEMP" --BSA::min_temperature "$MIN_TEMP" \
               --BSA::cooling_rate "$COOLING" --BSA::neighbors_accepted_ratio "$NAR" \
               --BSA::max_evaluations "$MAX_EVAL" >/dev/null 2>&1
        COST=$(grep -E "^Cost:" "$OUT" | grep -oE "[0-9]+" | head -1)
        CLEAN="${OUT%.json}_clean.json"
        sed '/^Cost:/,$d' "$OUT" > "$CLEAN"
        PEN=$(dotnet "$VALIDATOR" -i "$INST" -s "$CLEAN" 2>/dev/null \
              | grep -iE "exact absolute.*MinGroupSizePenalty" | grep -oE "is: [0-9]+" | grep -oE "[0-9]+" | head -1)
        if [ "$PEN" = "0" ] && [ -n "$COST" ]; then COSTS+=("$COST"); FEAS=$((FEAS+1)); fi
    done
    if [ "${#COSTS[@]}" -gt 0 ]; then
        STATS=$(printf "%s\n" "${COSTS[@]}" | sort -n | awk '
            { a[NR]=$1; sum+=$1 } END {
                n=NR; mean=sum/n;
                if (n%2==1) med=a[(n+1)/2]; else med=(a[n/2]+a[n/2+1])/2;
                for(i=1;i<=n;i++){d=a[i]-mean; ss+=d*d}
                std=(n>1)?sqrt(ss/n):0;
                printf "%d,%.1f,%.1f,%.1f", a[1], mean, med, std }')
        MIN=$(echo "$STATS"|cut -d, -f1); MEAN=$(echo "$STATS"|cut -d, -f2)
        MED=$(echo "$STATS"|cut -d, -f3); STD=$(echo "$STATS"|cut -d, -f4)
    else
        MIN="-"; MEAN="-"; MED="-"; STD="-"
    fi
    printf "%-18s %8s %10s %10s %9s %d/%d\n" "$NAME" "$MIN" "$MEAN" "$MED" "$STD" "$FEAS" "$SEEDS"
    echo "$NAME,$MIN,$MEAN,$MED,$STD,$FEAS,$SEEDS" >> "$OUTCSV"
done
echo ""
echo "Completato. CSV: $OUTCSV"