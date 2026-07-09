#!/bin/bash
################################################################################
# Test della config 276 (best-so-far irace medium TIGHT, iter 4) su un'istanza.
# strada B: fattori -> temperature assolute via bigM.
# Uso:  ./run_config_276.sh <istanza.json> [max_eval]
################################################################################

# ==== CONFIG 276 (best-so-far irace medium tight) ====
START_FACTOR=0.1509       # start_temp_factor
MIN_FACTOR=0.0001         # min_temp_factor  (1e-04)
COOLING=0.9923            # cooling_rate
NAR=0.1498                # neighbors_accepted_ratio
SWAP=0.5897               # swap_rate
# =====================================================
MAX_EVAL="${2:-1250000}"  
INSTANCE="$1"
[ -z "$INSTANCE" ] && { echo "Uso: $0 <istanza.json> [max_eval]"; exit 1; }

# bigM dell'istanza (identico a BT_Input::ComputeBigM)
BIGM=$(python3 -c "import json
d=json.load(open('$INSTANCE'))
o=d['Orders']; R=d['Resources']; ot=d['OrderTypes'][0]
T=len(o); M=len(R); sq=sum(x['Quantity'] for x in o)
pr=[x['Priority'] for x in o]
print(sq*(M-1)+T*M*max(ot['TargetGroupSize'],ot['MaxGroupSize']-ot['TargetGroupSize'])+T*(max(pr)-min(pr))+1)")

# fattori -> temperature assolute, con guardia min < start
START_ABS=$(python3 -c "print($START_FACTOR*$BIGM)")
MIN_ABS=$(python3 -c "s=$START_FACTOR*$BIGM; m=$MIN_FACTOR*$BIGM; print(m if m<s else s*0.001)")

NAME=$(basename "$INSTANCE" .json)
OUT="/tmp/${NAME}_out.json"

echo "=================================================="
echo "Esecuzione per istanza : $NAME"
echo "Config                 : 276  (best-so-far)"
echo "BigM calcolato         : $BIGM"
echo "Temperatura iniziale   : $START_ABS"
echo "Temperatura minima     : $MIN_ABS"
echo "Max evaluations        : $MAX_EVAL"
echo "=================================================="

/home/fabio/Master_Thesis/bt_main --main::instance "$INSTANCE" \
          --main::method BSA --main::seed 1 \
          --main::swap_rate "$SWAP" \
          --main::output_file "$OUT" \
          --BSA::start_temperature "$START_ABS" \
          --BSA::min_temperature "$MIN_ABS" \
          --BSA::cooling_rate "$COOLING" \
          --BSA::neighbors_accepted_ratio "$NAR" \
          --BSA::max_evaluations "$MAX_EVAL"

echo ""
echo "--- RISULTATO ---"
echo -n "Periodi schedulati: "; grep -c '"Rank"' "$OUT"
grep -E "^Cost:" "$OUT"

echo ""
echo "--- VALIDAZIONE ---"
sed '/^Cost:/,$d' "$OUT" > "${OUT%.json}_clean.json"
dotnet ~/Master_Thesis/btsp-validator-binary/btsp-validator.dll \
  -i "$INSTANCE" -s "${OUT%.json}_clean.json" | grep -iE "MinGroupSize|aggregated" | tail -3