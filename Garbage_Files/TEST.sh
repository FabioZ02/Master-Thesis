#!/bin/bash

# ==========
START_FACTOR=0.0013       # start_temp_factor (config 551 WIDE)
MIN_FACTOR=0.0001         # min_temp_factor
COOLING=0.9962            # cooling_rate
NAR=0.278                 # neighbors_accepted_ratio
SWAP=0.6132               # swap_rate
MAX_EVAL=3000000          # budget di deployment (piu' alto del tuning)
# ==========

INSTANCE="$1"
[ -z "$INSTANCE" ] && { echo "uso: $0 <istanza.json>"; exit 1; }

# bigM dell'istanza
BIGM=$(python3 -c "
import json
d=json.load(open('$INSTANCE'))
o=d['Orders']; R=d['Resources']; ot=d['OrderTypes'][0]
T=len(o); M=len(R); sq=sum(x['Quantity'] for x in o)
pr=[x['Priority'] for x in o]
print(sq*(M-1)+T*M*max(ot['TargetGroupSize'],ot['MaxGroupSize']-ot['TargetGroupSize'])+T*(max(pr)-min(pr))+1)")

START_ABS=$(python3 -c "print($START_FACTOR*$BIGM)")
MIN_ABS=$(python3 -c "s=$START_FACTOR*$BIGM; m=$MIN_FACTOR*$BIGM; print(m if m<s else s*0.001)")

NAME=$(basename "$INSTANCE" .json)
OUT="/tmp/${NAME}_out.json"

echo "istanza=$NAME  bigM=$BIGM  start_T=$START_ABS  min_T=$MIN_ABS"
./bt_main --main::instance "$INSTANCE" \
          --main::method BSA --main::seed 1 \
          --main::swap_rate "$SWAP" \
          --main::output_file "$OUT" \
          --BSA::start_temperature "$START_ABS" \
          --BSA::min_temperature "$MIN_ABS" \
          --BSA::cooling_rate "$COOLING" \
          --BSA::neighbors_accepted_ratio "$NAR" \
          --BSA::max_evaluations "$MAX_EVAL"

echo "--- risultato ---"
echo -n "periodi: "; grep -c '"Rank"' "$OUT"
grep -E "^Cost:" "$OUT"
echo "--- validazione ---"
sed '/^Cost:/,$d' "$OUT" > "${OUT%.json}_clean.json"
dotnet ~/Master_Thesis/btsp-validator-binary/btsp-validator.dll \
  -i "$INSTANCE" -s "${OUT%.json}_clean.json" | grep -iE "MinGroupSize|aggregated" | tail -3