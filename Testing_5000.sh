#!/usr/bin/env bash
# -----------------------------------------------------------------------------
# Confronto head-to-head dei 3 config finali di irace per il metodo BSA
# (Simulated Annealing standard). Config: 163 (vincitore post-selection),
# 573 e 602 (riserve, ramo discendente di 527).
# Criterio di scelta = somma dei ranghi per istanza (lo stesso di irace).
# -----------------------------------------------------------------------------

# ====================== DA IMPOSTARE ======================
INSTANCE_DIR="$HOME/Master_Thesis/Instances/small"
BINARY="$HOME/Master_Thesis/bt_main"
N_INSTANCES=50                         # le prime N istanze (ordine naturale)
N_REPS=10                               # run per istanza (deterministic=FALSE)

# Budget di deployment: USA LO STESSO budget del tuning, se possibile
MAX_EVAL=500000
SWAP_RATE=0.5
METHOD=BSA

# ---- I 3 config finali (parametri --BSA::) ----
declare -A CFG
CFG[163]="--BSA::start_temperature 12.8287 --BSA::min_temperature 0.032  --BSA::cooling_rate 0.9955 --BSA::neighbors_accepted_ratio 0.2165"
CFG[573]="--BSA::start_temperature 11.393  --BSA::min_temperature 0.0206 --BSA::cooling_rate 0.9976 --BSA::neighbors_accepted_ratio 0.1723"
CFG[602]="--BSA::start_temperature 11.7834 --BSA::min_temperature 0.0216 --BSA::cooling_rate 0.9978 --BSA::neighbors_accepted_ratio 0.1573"

# ---- Validatore BTPP (opzionale) ----
VALIDATE=0                              # 1 per controllare la feasibility
VALIDATOR_DLL="$HOME/Master_Thesis/Validator/Validator.dll"

# ---- Output ----
OUTDIR="$HOME/Master_Thesis/validation_BSA_3config/medium"
RAW_CSV="$OUTDIR/runs_raw.csv"
SUMMARY_CSV="$OUTDIR/summary.csv"
RANK_CSV="$OUTDIR/ranking.csv"
SOL_DIR="$OUTDIR/solutions"
# ==========================================================

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
echo "Metodo: $METHOD | Config: 163, 573, 602 | Run per istanza: $N_REPS | max_evaluations=$MAX_EVAL"
echo "Output in: $OUTDIR"
echo

echo "config,instance,seed,cost,time,feasible" > "$RAW_CSV"

# -----------------------------------------------------------------------------
# Loop principale: config -> istanza -> ripetizioni
# -----------------------------------------------------------------------------
for cfg in 163 573 602; do
    echo "######## CONFIG $cfg ########"
    inst_idx=0
    for inst_path in "${INSTANCES[@]}"; do
        inst_idx=$((inst_idx + 1))
        inst_name="$(basename "$inst_path" .json)"
        printf "[%2d/%d] %s\n" "$inst_idx" "${#INSTANCES[@]}" "$inst_name"

        for (( rep=1; rep<=N_REPS; rep++ )); do
            SEED=$RANDOM
            SOL_FILE="$SOL_DIR/sol_${cfg}_${inst_name}_rep${rep}.txt"

            # Cost/Time vengono scritti dal binario DENTRO il file soluzione, non su stdout
            "$BINARY" \
                --main::instance "$inst_path" \
                --main::method "$METHOD" \
                --main::seed "$SEED" \
                --main::swap_rate "$SWAP_RATE" \
                ${CFG[$cfg]} \
                --BSA::max_evaluations "$MAX_EVAL" \
                --main::output_file "$SOL_FILE" >/dev/null 2>&1

            COST=$(grep -oP 'Cost:\s*\K\d+'     "$SOL_FILE" 2>/dev/null || echo "NA")
            TIME=$(grep -oP 'Time:\s*\K[0-9.]+' "$SOL_FILE" 2>/dev/null || echo "NA")
            [[ -z "$COST" ]] && COST="NA"
            [[ -z "$TIME" ]] && TIME="NA"

            FEAS="NA"
            if [[ "$VALIDATE" == "1" && -s "$SOL_FILE" ]]; then
                JSON="${SOL_FILE%.txt}.json"
                grep -vE '^(Cost|Time):' "$SOL_FILE" > "$JSON"   # JSON pulito per il validatore
                if dotnet "$VALIDATOR_DLL" -i "$inst_path" -s "$JSON" >/dev/null 2>&1; then
                    FEAS=1
                else
                    FEAS=0
                fi
            fi

            echo "$cfg,$inst_name,$SEED,$COST,$TIME,$FEAS" >> "$RAW_CSV"
        done
    done
done

# -----------------------------------------------------------------------------
# Statistiche per (config, istanza)
# -----------------------------------------------------------------------------
echo
echo "Computing stats -> $SUMMARY_CSV"
{
    echo "config,instance,runs,mean_cost,std_cost,min_cost,max_cost,mean_time"
    awk -F, 'NR>1 && $4!="NA" {
        k=$1","$2;
        n[k]++; s[k]+=$4; ss[k]+=$4*$4; t[k]+=$5;
        if (!(k in mn) || $4 < mn[k]) mn[k]=$4;
        if (!(k in mx) || $4 > mx[k]) mx[k]=$4;
    }
    END {
        for (k in n) {
            m = s[k]/n[k];
            if (n[k] > 1) {
                var = (ss[k] - n[k]*m*m) / (n[k]-1);
                if (var < 0) var = 0;
                sd = sqrt(var);
            } else sd = 0;
            split(k, a, ",");
            printf "%s,%s,%d,%.2f,%.2f,%.0f,%.0f,%.3f\n", a[1], a[2], n[k], m, sd, mn[k], mx[k], t[k]/n[k];
        }
    }' "$RAW_CSV" | sort -V
} > "$SUMMARY_CSV"

# -----------------------------------------------------------------------------
# Verdetto: rank per istanza (sul mean_cost) -> somma dei ranghi (criterio irace)
# -----------------------------------------------------------------------------
echo "Computing ranking -> $RANK_CSV"
{
    echo "config,sum_rank,wins,mean_of_means"
    awk -F, 'NR>1 {
        mean[$1,$2]=$4; cfgs[$1]=1; insts[$2]=1; gsum[$1]+=$4; gn[$1]++;
    }
    END {
        for (i in insts) {
            for (c in cfgs) {
                r=1;
                for (d in cfgs) if (mean[d,i] < mean[c,i]) r++;
                rank[c]+=r;
                if (r==1) wins[c]++;
            }
        }
        for (c in cfgs)
            printf "%s,%d,%d,%.2f\n", c, rank[c], (c in wins?wins[c]:0), gsum[c]/gn[c];
    }' "$SUMMARY_CSV" | sort -t, -k2,2n
} > "$RANK_CSV"

echo
echo "================= Summary per istanza ================="
column -t -s, "$SUMMARY_CSV"
echo
echo "================= Verdetto (somma rank) ==============="
column -t -s, "$RANK_CSV"
echo "  (sum_rank piu' basso = migliore; pareggi -> rango condiviso)"
echo "======================================================="
echo
echo "File grezzo:  $RAW_CSV"
echo "Riepilogo:    $SUMMARY_CSV"
echo "Ranking:      $RANK_CSV"
echo "Soluzioni in: $SOL_DIR"