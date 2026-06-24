#!/bin/bash
# validate_tuning.sh
# Confronta la configurazione DEFAULT vs quella TUNED da irace su una cartella
# di istanze, ripetendo su piu' seed (il SA e' stocastico) e mediando i costi.
#
# Uso:
#   ./validate_tuning.sh [cartella_istanze] [n_seed] [max_evaluations]
# Esempi:
#   ./validate_tuning.sh ./Instances/medium 5 500000
#   ./validate_tuning.sh ./Instances/large 10
#
# Default: cartella ./Instances/medium, 5 seed, 500000 valutazioni.

set -u

INSTANCE_DIR="${1:-./Instances/medium}"
N_SEEDS="${2:-5}"
MAX_EVAL="${3:-500000}"
SOLVER="./bt_main"

# --- Configurazione vincente trovata da irace ---
TUNED_ARGS="--BSA::start_temperature 17.4003 --BSA::min_temperature 0.1425 --BSA::cooling_rate 0.9617"
# La DEFAULT non passa parametri di temperatura: EasyLocal usa i suoi default interni.

OUT_CSV="validation_results.csv"

if [ ! -x "$SOLVER" ]; then
  echo "ERRORE: $SOLVER non trovato o non eseguibile (lancia dalla cartella del solver)."
  exit 1
fi
if [ ! -d "$INSTANCE_DIR" ]; then
  echo "ERRORE: cartella istanze '$INSTANCE_DIR' inesistente."
  exit 1
fi

# Esegue il solver in modalita' irace (stampa solo il costo) e ritorna l'ultimo numero.
run_solver() {
  local instance="$1"; local seed="$2"; shift 2
  "$SOLVER" --main::instance "$instance" \
            --main::method BSA \
            --main::seed "$seed" \
            --BSA::max_evaluations "$MAX_EVAL" \
            --main::irace-enable \
            "$@" 2>/dev/null | grep -Eo '^-?[0-9]+(\.[0-9]+)?$' | tail -n 1
}

echo "instance,seed,default_cost,tuned_cost" > "$OUT_CSV"

shopt -s nullglob
instances=( "$INSTANCE_DIR"/*.json )
if [ ${#instances[@]} -eq 0 ]; then
  echo "ERRORE: nessun file .json in '$INSTANCE_DIR'."
  exit 1
fi

echo "Istanze: ${#instances[@]} | seed per istanza: $N_SEEDS | max_eval: $MAX_EVAL"
echo "Output grezzo -> $OUT_CSV"
echo "-------------------------------------------------------------"

for inst in "${instances[@]}"; do
  name=$(basename "$inst" .json)
  for seed in $(seq 1 "$N_SEEDS"); do
    def=$(run_solver "$inst" "$seed")                 # default: nessun parametro temperatura
    tun=$(run_solver "$inst" "$seed" $TUNED_ARGS)     # tuned
    def="${def:-NA}"; tun="${tun:-NA}"
    echo "$name,$seed,$def,$tun" >> "$OUT_CSV"
    printf "  %-28s seed %-3s  default=%-10s tuned=%-10s\n" "$name" "$seed" "$def" "$tun"
  done
done

echo "============================================================="
echo " RIEPILOGO (media e best sui $N_SEEDS seed, per istanza)"
echo "============================================================="

awk -F, 'NR>1 {
  inst=$1; d=$3; t=$4;
  if (d=="NA" || t=="NA") next;
  sumd[inst]+=d; sumt[inst]+=t; n[inst]++;
  if (!(inst in mind) || d<mind[inst]) mind[inst]=d;
  if (!(inst in mint) || t<mint[inst]) mint[inst]=t;
  order[inst]=1;
}
END {
  printf "%-28s %12s %12s %12s %12s %10s\n", "istanza","def_media","tun_media","def_best","tun_best","gain%";
  tot_d=0; tot_t=0; cnt=0;
  for (i in order) {
    md=sumd[i]/n[i]; mt=sumt[i]/n[i];
    gain=(md>0)?(md-mt)/md*100:0;
    printf "%-28s %12.2f %12.2f %12.2f %12.2f %+9.2f%%\n", i, md, mt, mind[i], mint[i], gain;
    tot_d+=md; tot_t+=mt; cnt++;
  }
  if (cnt>0) {
    od=tot_d/cnt; ot=tot_t/cnt; og=(od>0)?(od-ot)/od*100:0;
    printf "%-28s %12.2f %12.2f %12s %12s %+9.2f%%\n", "--- MEDIA COMPLESSIVA ---", od, ot, "", "", og;
  }
}' "$OUT_CSV"

echo "-------------------------------------------------------------"
echo "gain% positivo = la configurazione tuned costa MENO (meglio)."
echo "Dati completi in $OUT_CSV"