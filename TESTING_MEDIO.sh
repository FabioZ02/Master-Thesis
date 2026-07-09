#!/bin/bash
################################################################################
# Benchmark config 223 (medium abs) su 25 istanze medie.
# min / media / mediana / std sui costi FEASIBLE.
################################################################################

# ============================ DA IMPOSTARE ==================================
INSTDIR="$HOME/Master_Thesis/Instances/medium/tuning_set"
N_INSTANCES=25            # prime N istanze (ordine naturale sort -V)
SEEDS=5                   # run per istanza
MAX_EVAL=1000000          # 1M valutazioni
VALIDATOR="$HOME/Master_Thesis/btsp-validator-binary/btsp-validator.dll"
BIN="$HOME/Master_Thesis/bt_main"
OUTCSV="$HOME/Master_Thesis/bench223_medium.csv"

# --- config 223 ---
START_TEMP=11.2173
MIN_TEMP=0.0078
COOLING=0.9917
NAR=0.2766
SWAP=0.6463
# ===========================================================================

# seleziona le prime N istanze in ordine naturale
mapfile -t ALL < <(ls "$INSTDIR"/*.json | sort -V)
INSTANCES=("${ALL[@]:0:$N_INSTANCES}")

echo "istanza,min,mean,median,std,feasible_runs,total_runs" > "$OUTCSV"

printf "%-18s %8s %10s %10s %9s %s\n" "istanza" "min" "mean" "median" "std" "feas/tot"
printf "%-18s %8s %10s %10s %9s %s\n" "-------" "---" "----" "------" "---" "--------"

for INST in "${INSTANCES[@]}"; do
    NAME=$(basename "$INST" .json)
    COSTS=()
    FEAS=0

    for ((s=1; s<=SEEDS; s++)); do
        SEED=$RANDOM
        OUT="/tmp/${NAME}_s${s}.json"

        "$BIN" --main::instance "$INST" \
               --main::method BSA --main::seed "$SEED" \
               --main::swap_rate "$SWAP" \
               --main::output_file "$OUT" \
               --BSA::start_temperature "$START_TEMP" \
               --BSA::min_temperature "$MIN_TEMP" \
               --BSA::cooling_rate "$COOLING" \
               --BSA::neighbors_accepted_ratio "$NAR" \
               --BSA::max_evaluations "$MAX_EVAL" >/dev/null 2>&1

        COST=$(grep -E "^Cost:" "$OUT" | grep -oE "[0-9]+" | head -1)

        # valida feasibility
        CLEAN="${OUT%.json}_clean.json"
        sed '/^Cost:/,$d' "$OUT" > "$CLEAN"
        PEN=$(dotnet "$VALIDATOR" -i "$INST" -s "$CLEAN" 2>/dev/null \
              | grep -iE "exact absolute.*MinGroupSizePenalty" \
              | grep -oE "is: [0-9]+" | grep -oE "[0-9]+" | head -1)

        if [ "$PEN" = "0" ] && [ -n "$COST" ]; then
            COSTS+=("$COST")
            FEAS=$((FEAS+1))
        fi
    done

    # statistiche sui costi feasible
    if [ "${#COSTS[@]}" -gt 0 ]; then
        STATS=$(printf "%s\n" "${COSTS[@]}" | sort -n | awk '
            { a[NR]=$1; sum+=$1 }
            END {
                n=NR; mean=sum/n;
                # mediana
                if (n%2==1) med=a[(n+1)/2];
                else med=(a[n/2]+a[n/2+1])/2;
                # std (popolazione)
                for(i=1;i<=n;i++){d=a[i]-mean; ss+=d*d}
                std=(n>1)?sqrt(ss/n):0;
                printf "%d,%.1f,%.1f,%.1f", a[1], mean, med, std
            }')
        MIN=$(echo "$STATS" | cut -d, -f1)
        MEAN=$(echo "$STATS" | cut -d, -f2)
        MED=$(echo "$STATS" | cut -d, -f3)
        STD=$(echo "$STATS" | cut -d, -f4)
    else
        MIN="-"; MEAN="-"; MED="-"; STD="-"
    fi

    printf "%-18s %8s %10s %10s %9s %d/%d\n" "$NAME" "$MIN" "$MEAN" "$MED" "$STD" "$FEAS" "$SEEDS"
    echo "$NAME,$MIN,$MEAN,$MED,$STD,$FEAS,$SEEDS" >> "$OUTCSV"
done

echo ""
echo "CSV salvato in: $OUTCSV"