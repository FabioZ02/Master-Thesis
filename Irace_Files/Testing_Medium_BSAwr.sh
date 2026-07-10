#!/usr/bin/env bash
# -----------------------------------------------------------------------------
# Validazione della configurazione irace vincente (ID 1304, tuning su Medium)
# per il metodo BSAwr (Simulated Annealing with Reheating)
# -----------------------------------------------------------------------------

# ====================== DA IMPOSTARE ======================
# Punta alla cartella delle istanze medie
INSTANCE_DIR="$HOME/Master_Thesis/Instances/medium"
BINARY="$HOME/Master_Thesis/bt_main"   
N_INSTANCES=25                          # le prime 25 istanze
N_REPS=10                               # 10 run per istanza (robusteza statistica)

# ==========================================================
# ---- Parametri SAwr della config 1304 ----
START_TEMP=14.2753
MIN_TEMP=0.1434
COOLING=0.9275
MAX_REHEATS=1
REHEAT_RATIO=2.8610
FIRST_SHARE=0.2348

# Il budget colossale usato per il fine-tuning del Reheating
MAX_EVAL=300000 
SWAP_RATE=0.5
METHOD=BSAwr

# ---- Output ----
OUTDIR="$HOME/Master_Thesis/validation_BSAwr_1304/medium"
RAW_CSV="$OUTDIR/runs_raw_medium.csv"          
SUMMARY_CSV="$OUTDIR/summary_medium.csv"       
SOL_DIR="$OUTDIR/solutions_medium"             

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

# Seleziona le prime N_INSTANCES istanze in ordine naturale
mapfile -t INSTANCES < <(ls "$INSTANCE_DIR"/*.json 2>/dev/null | sort -V | head -n "$N_INSTANCES")

if [[ ${#INSTANCES[@]} -eq 0 ]]; then
    echo "ERRORE: nessun file .json trovato in $INSTANCE_DIR" >&2
    exit 1
fi

echo "Istanze trovate: ${#INSTANCES[@]} (richieste: $N_INSTANCES)"
echo "Metodo: $METHOD | Run per istanza: $N_REPS | budget: max_evaluations=$MAX_EVAL"
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

    # Esegue N_REPS run per l'istanza corrente
    for (( rep=1; rep<=N_REPS; rep++ )); do
        SEED=$RANDOM
        SOL_FILE="$SOL_DIR/sol_${inst_name}_rep${rep}.txt"

        # Esecuzione del binario C++ con i nuovi flag per il Reheating
        # (Cost/Time vengono scritti dal binario DENTRO il file soluzione, non su stdout)
        "$BINARY" \
            --main::instance "$inst_path" \
            --main::method "$METHOD" \
            --main::seed "$SEED" \
            --main::swap_rate "$SWAP_RATE" \
            --BSAwr::max_evaluations "$MAX_EVAL" \
            --BSAwr::start_temperature "$START_TEMP" \
            --BSAwr::min_temperature "$MIN_TEMP" \
            --BSAwr::cooling_rate "$COOLING" \
            --BSAwr::max_reheats "$MAX_REHEATS" \
            --BSAwr::reheat_ratio "$REHEAT_RATIO" \
            --BSAwr::first_descent_evaluations_share "$FIRST_SHARE" \
            --main::output_file "$SOL_FILE" >/dev/null 2>&1

        # Parsing dal file soluzione per estrarre Costo e Tempo
        COST=$(grep -oP 'Cost:\s*\K\d+'     "$SOL_FILE" 2>/dev/null || echo "NA")
        TIME=$(grep -oP 'Time:\s*\K[0-9.]+' "$SOL_FILE" 2>/dev/null || echo "NA")
        [[ -z "$COST" ]] && COST="NA"
        [[ -z "$TIME" ]] && TIME="NA"

        # Salva la run nel file raw
        echo "$inst_name,$SEED,$COST,$TIME" >> "$RAW_CSV"
    done
done

echo
echo "Computing stats -> $SUMMARY_CSV"
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
            } else {
                sd = 0;
            }
            printf "%s,%d,%.2f,%.2f,%.0f,%.0f,%.3f\n", i, n[i], m, sd, mn[i], mx[i], t[i]/n[i];
        }
    }' "$RAW_CSV" | sort -V
} > "$SUMMARY_CSV"

echo
echo "================= Summary ================="
column -t -s, "$SUMMARY_CSV"
echo "============================================="
echo
echo "File grezzo (tutte le run): $RAW_CSV"
echo "Riepilogo per istanza:      $SUMMARY_CSV"
echo "Soluzioni salvate in:       $SOL_DIR"