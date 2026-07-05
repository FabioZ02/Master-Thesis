#!/bin/bash
# Divide il tuning set in "larghe" (Smin/Smax < 0.6) e "tese" (>= 0.6)
# e crea due file-lista per irace (trainInstancesFile).

# ============================ DA IMPOSTARE ==================================
SRC="$HOME/Master_Thesis/Instances/small/tuning_set"
CUT=0.60
OUT_WIDE="$HOME/Master_Thesis/instances_wide.txt"
OUT_TIGHT="$HOME/Master_Thesis/instances_tight.txt"
# ===========================================================================

> "$OUT_WIDE"; > "$OUT_TIGHT"
for f in "$SRC"/*.json; do
    r=$(python3 -c "import json;d=json.load(open('$f'));ot=d['OrderTypes'][0];print(ot['MinGroupSize']/ot['MaxGroupSize'])")
    if python3 -c "exit(0 if $r < $CUT else 1)"; then
        echo "$f" >> "$OUT_WIDE"
    else
        echo "$f" >> "$OUT_TIGHT"
    fi
done
echo "larghe (Smin/Smax < $CUT): $(wc -l < "$OUT_WIDE")  -> $OUT_WIDE"
echo "tese   (Smin/Smax >= $CUT): $(wc -l < "$OUT_TIGHT") -> $OUT_TIGHT"