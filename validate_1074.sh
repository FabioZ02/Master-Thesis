#!/usr/bin/env bash
#
# validate_1074.sh
# -----------------------------------------------------------------------------
# Validazione della configurazione irace vincente (ID 1074, tuning su Medium)
# su un set di istanze. Esegue N_REPS run con seed diversi per ciascuna delle
# prime N_INSTANCES istanze, raccoglie costo e tempo di ogni run, e calcola
# media / deviazione standard (campionaria) / min / max per istanza.
#
# Config 1074:  start_temp=13.4266  min_temp=0.0075  cooling_rate=0.9512
# Budget per run: max_evaluations=500000 (lo stesso del tuning -> coerenza)
# -----------------------------------------------------------------------------

# ====================== DA IMPOSTARE ======================
# Cartella con le istanze .json da validare.
# >>> Punta questa riga al set Large / "medium-large" che vuoi testare <<<
INSTANCE_DIR="$HOME/Master_Thesis/Instances/small"

BINARY="$HOME/Master_Thesis/bt_main"   # eseguibile compilato
N_INSTANCES=25                          # quante istanze (le prime, ordine naturale)
N_REPS=10                               # run per istanza (seed = 1..N_REPS)
# ==========================================================

# ---- Parametri SA della config 1074 (NON modificare per la validazione) ----
START_TEMP=13.4266
MIN_TEMP=0.0075
COOLING=0.9512
MAX_EVAL=500000
SWAP_RATE=0.5
METHOD=BSA

# ---- Output ----
OUTDIR="$HOME/Master_Thesis/validation_1074/small"
RAW_CSV="$OUTDIR/runs_raw_small.csv"          # una riga per ogni run
SUMMARY_CSV="$OUTDIR/summary_small.csv"       # una riga per istanza (media, std, ...)
SOL_DIR="$OUTDIR/solutions_small"             # soluzioni salvate (per il validatore)

# -----------------------------------------------------------------------------
set -o pipefail

# Controlli preliminari
if [[ ! -x "$BINARY" ]]; then
    echo "ERRORE: binario non trovato o non eseguibile: $BINARY" >&2
    exit 1
fi
if [[ ! -d "$INSTANCE_DIR" ]]; then
    echo "ERRORE: cartella istanze inesistente: $INSTANCE_DIR" >&2
    exit 1
fi

mkdir -p "$OUTDIR" "$SOL_DIR"

# Seleziona le prime N_INSTANCES istanze in ordine naturale (test_2 prima di test_10)
mapfile -t INSTANCES < <(ls "$INSTANCE_DIR"/*.json 2>/dev/null | sort -V | head -n "$N_INSTANCES")

if [[ ${#INSTANCES[@]} -eq 0 ]]; then
    echo "ERRORE: nessun file .json trovato in $INSTANCE_DIR" >&2
    exit 1
fi

echo "Istanze trovate: ${#INSTANCES[@]} (richieste: $N_INSTANCES)"
echo "Run per istanza: $N_REPS  |  budget: max_evaluations=$MAX_EVAL"
echo "Output in: $OUTDIR"
echo

# Intestazione CSV grezzo
echo "instance,seed,cost,time" > "$RAW_CSV"

# -----------------------------------------------------------------------------
# Loop principale
# -----------------------------------------------------------------------------
inst_idx=0
for inst_path in "${INSTANCES[@]}"; do
    inst_idx=$((inst_idx + 1))
    inst_name="$(basename "$inst_path" .json)"
    printf "[%2d/%d] %s\n" "$inst_idx" "${#INSTANCES[@]}" "$inst_name"

    for seed in $(seq 1 "$N_REPS"); do
        sol_file="$SOL_DIR/${inst_name}_seed${seed}.sol"

        # stdout in modalita output_file = riga singola: "nome: costo (time: T)"
        line="$("$BINARY" \
            --main::instance   "$inst_path" \
            --main::method     "$METHOD" \
            --main::seed       "$seed" \
            --main::swap_rate  "$SWAP_RATE" \
            --main::output_file "$sol_file" \
            --BSA::start_temperature "$START_TEMP" \
            --BSA::min_temperature   "$MIN_TEMP" \
            --BSA::cooling_rate      "$COOLING" \
            --BSA::max_evaluations   "$MAX_EVAL" \
            2>/dev/null)"
        status=$?

        # Estrae costo e tempo dalla riga "nome: 12345 (time: 2.34)"
        cost="$(sed -nE 's/.*:[[:space:]]*([0-9]+)[[:space:]]*\(time.*/\1/p' <<<"$line")"
        rtime="$(sed -nE 's/.*time:[[:space:]]*([0-9.]+)\).*/\1/p' <<<"$line")"

        if [[ $status -ne 0 || -z "$cost" ]]; then
            echo "    seed $seed: FALLITO (status=$status, output='$line')" >&2
            echo "$inst_name,$seed,NA,NA" >> "$RAW_CSV"
            continue
        fi

        printf "    seed %-2d  cost=%-10s time=%ss\n" "$seed" "$cost" "${rtime:-?}"
        echo "$inst_name,$seed,$cost,${rtime:-NA}" >> "$RAW_CSV"
    done
done

# -----------------------------------------------------------------------------
# Statistiche per istanza (media, std campionaria, min, max, tempo medio)
# -----------------------------------------------------------------------------
echo
echo "Calcolo statistiche -> $SUMMARY_CSV"

{
    echo "instance,runs,mean_cost,std_cost,min_cost,max_cost,mean_time"
    awk -F, 'NR>1 && $3!="NA" {
        n[$1]++; s[$1]+=$3; ss[$1]+=$3*$3; t[$1]+=$4;
        if (!($1 in mn) || $3 < mn[$1]) mn[$1]=$3;
        if (!($1 in mx) || $3 > mx[$1]) mx[$1]=$3;
    }
    END {
        for (i in n) {
            m = s[i]/n[i];
            if (n[i] > 1) {
                var = (ss[i] - n[i]*m*m) / (n[i]-1);
                if (var < 0) var = 0;          # guardia per errori float
                sd = sqrt(var);
            } else { sd = 0; }
            printf "%s,%d,%.2f,%.2f,%.0f,%.0f,%.3f\n", i, n[i], m, sd, mn[i], mx[i], t[i]/n[i];
        }
    }' "$RAW_CSV" | sort -V
} > "$SUMMARY_CSV"

echo
echo "================= RIEPILOGO ================="
column -t -s, "$SUMMARY_CSV"
echo "============================================="
echo
echo "File grezzo (tutte le run): $RAW_CSV"
echo "Riepilogo per istanza:      $SUMMARY_CSV"
echo "Soluzioni salvate in:       $SOL_DIR"