# analyze_irace.R
# Estrae i risultati principali da irace.Rdata (compatibile irace >= 3.x / 4.x)
# Uso:  Rscript analyze_irace.R
# Lancialo dalla cartella che contiene irace.Rdata

load("irace.Rdata")

# --- helper: estrae righe di configurazione per ID dal data frame allConfigurations ---
get_conf <- function(res, ids) {
  conf <- res$allConfigurations
  conf[conf$.ID. %in% ids, , drop = FALSE]
}

cat("==========================================================\n")
cat(" ELITE FINALI (dalla migliore alla peggiore per somma ranghi)\n")
cat("==========================================================\n")
last.iter  <- length(iraceResults$allElites)
elite.ids  <- iraceResults$allElites[[last.iter]]
elite.conf <- get_conf(iraceResults, elite.ids)
# riordina secondo l'ordine di allElites (gia' dalla migliore)
elite.conf <- elite.conf[match(elite.ids, elite.conf$.ID.), ]
print(elite.conf)

cat("\n==========================================================\n")
cat(" CONFIGURAZIONE VINCENTE (la prima elite)\n")
cat("==========================================================\n")
best.id <- elite.ids[1]
print(get_conf(iraceResults, best.id))

cat("\n==========================================================\n")
cat(" MATRICE DEI COSTI (righe = istanze, colonne = configurazioni)\n")
cat(" valori = costo grezzo prodotto dal solver per ogni run\n")
cat("==========================================================\n")
exp <- iraceResults$experiments
elite.cols <- as.character(elite.ids)
elite.cols <- elite.cols[elite.cols %in% colnames(exp)]
print(exp[, elite.cols, drop = FALSE])

cat("\n--- Costo medio per configurazione elite (sulle istanze valutate) ---\n")
print(round(colMeans(exp[, elite.cols, drop = FALSE], na.rm = TRUE), 2))

cat("\n--- Numero di istanze su cui ogni elite e' stata valutata ---\n")
print(colSums(!is.na(exp[, elite.cols, drop = FALSE])))

write.csv(exp, "irace_costs_matrix.csv")
cat("\nMatrice completa dei costi salvata in: irace_costs_matrix.csv\n")