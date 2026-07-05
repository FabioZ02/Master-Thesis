#!/bin/bash
###############################################################################
# Benchmark su tutte le istanze: N run con seed diversi per istanza, config
# scelta per tensione (Smin/Smax). Riporta feasible/N e media|mediana|std del
# costo sui run feasible.
#   Tabella: instance | tension | class | feasible | periods | min | mean | median | std
###############################################################################

# ============================ DA IMPOSTARE ==================================
INSTDIR="$HOME/Master_Thesis/Instances/small/tuning_set"
BINARY="$HOME/Master_Thesis/bt_main"
CUT=0.60                 # <CUT = wide, >=CUT = tight
MAX_EVAL=3000000          # eval per run
SEEDS=5                  # run per istanza (seed 1..SEEDS)
OUT_CSV="$HOME/Master_Thesis/benchmark_results.csv"

# --- config WIDE (irace, id 551) ---
W_START=0.0013 ; W_MIN=0.0001 ; W_COOL=0.9962 ; W_NAR=0.278  ; W_SWAP=0.6132
# --- config TIGHT (irace, id 489) ---
T_START=0.1039 ; T_MIN=0.0001 ; T_COOL=0.9988 ; T_NAR=0.2392 ; T_SWAP=0.5491
# ===========================================================================

echo "instance,tension,class,feasible,periods,cost_min,cost_mean,cost_median,cost_std" > "$OUT_CSV"
printf "%-20s %5s %5s %5s %4s %7s %8s %8s %7s\n" istanza tens cls feas per min mean median std

for INSTANCE in "$INSTDIR"/*.json; do
    NAME=$(basename "$INSTANCE" .json)

    read TENS BIGM < <(python3 -c "
import json
d=json.load(open('$INSTANCE'))
o=d['Orders']; R=d['Resources']; ot=d['OrderTypes'][0]
T=len(o); M=len(R); sq=sum(x['Quantity'] for x in o)
pr=[x['Priority'] for x in o]
tens=ot['MinGroupSize']/ot['MaxGroupSize']
bigM=sq*(M-1)+T*M*max(ot['TargetGroupSize'],ot['MaxGroupSize']-ot['TargetGroupSize'])+T*(max(pr)-min(pr))+1
print(f'{tens:.4f} {bigM}')")

    if python3 -c "exit(0 if $TENS < $CUT else 1)"; then
        CLASS=wide;  SF=$W_START; MF=$W_MIN; CO=$W_COOL; NA=$W_NAR; SW=$W_SWAP
    else
        CLASS=tight; SF=$T_START; MF=$T_MIN; CO=$T_COOL; NA=$T_NAR; SW=$T_SWAP
    fi

    START_ABS=$(python3 -c "print($SF*$BIGM)")
    MIN_ABS=$(python3 -c "s=$SF*$BIGM; m=$MF*$BIGM; print(m if m<s else s*0.001)")

    COSTS=""; BEST=999999999; BESTPER=0
    for s in $(seq 1 $SEEDS); do
        OUT="/tmp/${NAME}_s${s}.json"
        "$BINARY" --main::instance "$INSTANCE" --main::method BSA --main::seed "$s" \
            --main::swap_rate "$SW" --main::output_file "$OUT" \
            --BSA::start_temperature "$START_ABS" --BSA::min_temperature "$MIN_ABS" \
            --BSA::cooling_rate "$CO" --BSA::neighbors_accepted_ratio "$NA" \
            --BSA::max_evaluations "$MAX_EVAL" >/dev/null 2>&1
        c=$(grep -E "^Cost:" "$OUT" | awk '{print $2}')
        p=$(grep -c '"Rank"' "$OUT")
        COSTS="$COSTS $c"
        if [ -n "$c" ] && [ "$c" -lt "$BEST" ] 2>/dev/null; then BEST=$c; BESTPER=$p; fi
        rm -f "$OUT"
    done

    read FEAS CMIN CMEAN CMED CSTD < <(python3 -c "
import statistics as st
costs=[float(x) for x in '$COSTS'.split() if x]
bigM=$BIGM
feas=[c for c in costs if c < bigM]
n=len(feas)
if n>0:
    print(f'{n}/{len(costs)} {min(feas):.0f} {st.mean(feas):.1f} {st.median(feas):.1f} {(st.pstdev(feas) if n>1 else 0.0):.1f}')
else:
    print(f'0/{len(costs)} {min(costs):.0f} {st.mean(costs):.0f} {st.median(costs):.0f} 0.0')")

    echo "$NAME,$TENS,$CLASS,$FEAS,$BESTPER,$CMIN,$CMEAN,$CMED,$CSTD" >> "$OUT_CSV"
    printf "%-20s %5s %5s %5s %4s %7s %8s %8s %7s\n" "$NAME" "$TENS" "$CLASS" "$FEAS" "$BESTPER" "$CMIN" "$CMEAN" "$CMED" "$CSTD"
done

echo
echo "Tabella salvata in: $OUT_CSV"
awk -F, 'NR>1{tot++; split($4,a,"/"); if(a[1]==a[2])allok++} END{print "Istanze con tutti i run feasible: "allok"/"tot}' "$OUT_CSV"