#!/bin/bash
# compare_configs.sh
# Confronta testa a testa le due configurazioni trovate dai due tuning irace,
# sulle stesse istanze, su piu' seed. Dice quale vince.
#
# Uso:  bash compare_configs.sh [cartella_istanze] [n_seed] [max_evaluations]
# Default: ./Instances/small 5 500000

set -u
INSTANCE_DIR="${1:-./Instances/small}"
N_SEEDS="${2:-5}"
MAX_EVAL="${3:-500000}"
SOLVER="./bt_main"

# Le due configurazioni candidate
CFG_A="--BSA::start_temperature 17.4003 --BSA::min_temperature 0.1425 --BSA::cooling_rate 0.9617"  # Tuning 1 (caldo)
CFG_B="--BSA::start_temperature 7.4919  --BSA::min_temperature 0.126  --BSA::cooling_rate 0.9082"   # Tuning 2 (freddo)

OUT_CSV="compare_configs.csv"

if [ ! -x "$SOLVER" ]; then echo "ERRORE: $SOLVER non eseguibile"; exit 1; fi
if [ ! -d "$INSTANCE_DIR" ]; then echo "ERRORE: cartella '$INSTANCE_DIR' inesistente"; exit 1; fi

run_solver() {
  local inst="$1"; local seed="$2"; shift 2
  "$SOLVER" --main::instance "$inst" --main::method BSA --main::seed "$seed" \
            --BSA::max_evaluations "$MAX_EVAL" --main::irace-enable "$@" 2>/dev/null \
    | grep -Eo '^-?[0-9]+(\.[0-9]+)?$' | tail -n 1
}

echo "instance,seed,cfgA_caldo,cfgB_freddo" > "$OUT_CSV"
shopt -s nullglob
instances=( "$INSTANCE_DIR"/*.json )
[ ${#instances[@]} -eq 0 ] && { echo "ERRORE: nessun .json in '$INSTANCE_DIR'"; exit 1; }

echo "Istanze: ${#instances[@]} | seed: $N_SEEDS | max_eval: $MAX_EVAL"
echo "A = caldo (17.4/0.14/0.96)   B = freddo (7.5/0.13/0.91)"
echo "-------------------------------------------------------------"
for inst in "${instances[@]}"; do
  name=$(basename "$inst" .json)
  for seed in $(seq 1 "$N_SEEDS"); do
    a=$(run_solver "$inst" "$seed" $CFG_A); a="${a:-NA}"
    b=$(run_solver "$inst" "$seed" $CFG_B); b="${b:-NA}"
    echo "$name,$seed,$a,$b" >> "$OUT_CSV"
    printf "  %-26s seed %-3s  A=%-9s B=%-9s\n" "$name" "$seed" "$a" "$b"
  done
done

echo "============================================================="
echo " RIEPILOGO"
echo "============================================================="
awk -F, 'NR>1 && $3!="NA" && $4!="NA" {
  sa+=$3; sb+=$4; n++;
  if ($3<$4) wa++; else if ($4<$3) wb++; else tie++;
}
END {
  if (n==0){print "Nessun dato valido"; exit}
  printf "Run validi: %d\n", n;
  printf "Costo medio  A (caldo)  = %.2f\n", sa/n;
  printf "Costo medio  B (freddo) = %.2f\n", sb/n;
  printf "Vittorie per-run: A=%d  B=%d  pareggi=%d\n", wa, wb, tie;
  d=(sa-sb)/n;
  if (d>0) printf "\n=> B (freddo) e migliore in media di %.2f (%.1f%%)\n", d, d/(sa/n)*100;
  else if (d<0) printf "\n=> A (caldo) e migliore in media di %.2f (%.1f%%)\n", -d, -d/(sb/n)*100;
  else printf "\n=> Pari in media\n";
}' "$OUT_CSV"
echo "Dati completi in $OUT_CSV"
