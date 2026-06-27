#!/usr/bin/env python3
"""
analizar_resultados.py
Procesa resultados_medidas.csv y resultados_aristas.csv
Genera: tabla de stats (media, varianza, std) y gráficos PNG listos para el informe.

Uso:
    python3 analizar_resultados.py
"""

import pandas as pd
import matplotlib.pyplot as plt
import matplotlib.ticker as ticker
import math
import sys

# ──────────────────────────────────────────────
#  1. CARGAR CSV
# ──────────────────────────────────────────────
try:
    df_med = pd.read_csv("resultados_medidas.csv")
    df_ari = pd.read_csv("resultados_aristas.csv")
except FileNotFoundError as e:
    sys.exit(f"ERROR: {e}\nAsegúrate de haber corrido ./experimentos primero.")

# Separar filas de construcción de filas de medidas
construccion_medidas = ["vertices", "aristas", "construccion_ms", "memoria_delta_KB"]
df_const = df_med[df_med["medida"].isin(construccion_medidas)].copy()
df_timed = df_med[~df_med["medida"].isin(construccion_medidas)].copy()
df_timed = df_timed[df_timed["repeticion"] != 0]  # filtrar filas no temporales

# ──────────────────────────────────────────────
#  2. TABLA DE CONSTRUCCIÓN
# ──────────────────────────────────────────────
print("\n===== CONSTRUCCIÓN DEL GRAFO =====")
tabla_const = df_const.pivot_table(index="dataset", columns="medida", values="valor", aggfunc="first")
print(tabla_const.to_string())
print()

# ──────────────────────────────────────────────
#  3. TABLA DE ESTADÍSTICAS POR MEDIDA
# ──────────────────────────────────────────────
stats = (
    df_timed
    .groupby(["dataset", "medida"])["valor"]
    .agg(
        media="mean",
        varianza="var",
        std="std",
        min_ms="min",
        max_ms="max",
    )
    .reset_index()
)
stats["cv_%"] = (stats["std"] / stats["media"] * 100).round(2)  # coeficiente de variación

print("===== ESTADÍSTICAS DE TIEMPOS (ms) =====")
print(stats.to_string(index=False, float_format="{:.4f}".format))
print()

# Exportar para el informe
stats.to_csv("tabla_stats.csv", index=False, float_format="%.4f")
print("→ tabla_stats.csv guardada")

# ──────────────────────────────────────────────
#  4. GRÁFICO 1: Comparación de tiempos por medida y dataset
# ──────────────────────────────────────────────
fig, ax = plt.subplots(figsize=(12, 6))

datasets   = stats["dataset"].unique()
medidas    = stats["medida"].unique()
x          = range(len(medidas))
bar_width  = 0.35
colores    = ["steelblue", "darkorange"]

for i, ds in enumerate(datasets):
    sub = stats[stats["dataset"] == ds].set_index("medida").reindex(medidas)
    offset = [xi + i * bar_width for xi in x]
    bars = ax.bar(
        offset,
        sub["media"],
        bar_width,
        yerr=sub["std"],
        label=ds,
        color=colores[i],
        alpha=0.85,
        capsize=4,
    )

ax.set_yscale("log")  # escala log para ver medidas rápidas y lentas juntas
ax.set_ylabel("Tiempo promedio (ms) — escala log")
ax.set_title("Tiempo promedio por medida (10 repeticiones, ±1σ)")
ax.set_xticks([xi + bar_width / 2 for xi in x])
ax.set_xticklabels(medidas, rotation=30, ha="right", fontsize=9)
ax.legend()
ax.grid(axis="y", linestyle="--", alpha=0.5)
plt.tight_layout()
plt.savefig("grafico_tiempos.png", dpi=150)
print("→ grafico_tiempos.png guardado")
plt.close()

# ──────────────────────────────────────────────
#  5. GRÁFICO 2: Distribución de 10 reps por medida (box plot)
#     Útil para detectar varianza alta o outliers
# ──────────────────────────────────────────────
fig, axes = plt.subplots(1, len(datasets), figsize=(14, 5), sharey=False)
if len(datasets) == 1:
    axes = [axes]

for ax, ds in zip(axes, datasets):
    sub = df_timed[df_timed["dataset"] == ds]
    grupos = [sub[sub["medida"] == m]["valor"].values for m in medidas]
    bp = ax.boxplot(grupos, tick_labels=medidas, patch_artist=True)
    for patch in bp["boxes"]:
        patch.set_facecolor("lightsteelblue")
    ax.set_yscale("log")
    ax.set_title(f"Distribución tiempos — {ds}")
    ax.set_ylabel("ms (log)")
    ax.tick_params(axis="x", rotation=40, labelsize=8)
    ax.grid(axis="y", linestyle="--", alpha=0.4)

plt.tight_layout()
plt.savefig("grafico_boxplot.png", dpi=150)
print("→ grafico_boxplot.png guardado")
plt.close()

# ──────────────────────────────────────────────
#  6. GRÁFICO 3: Impacto de añadir/quitar aristas
# ──────────────────────────────────────────────
fig, axes = plt.subplots(1, len(datasets), figsize=(14, 5))
if len(datasets) == 1:
    axes = [axes]

for ax, ds in zip(axes, datasets):
    sub = df_ari[df_ari["dataset"] == ds].copy()
    if sub.empty:
        ax.set_title(f"{ds} — sin datos")
        continue

    # Pivot: medida en x, delta en y, color por operacion+tipo
    sub["etiqueta"] = sub["operacion"] + "_" + sub["tipo_arista"]
    pivot = sub.pivot_table(index="medida", columns="etiqueta", values="delta", aggfunc="mean")
    pivot.plot(kind="bar", ax=ax, colormap="tab10", alpha=0.85)
    ax.axhline(0, color="black", linewidth=0.8, linestyle="--")
    ax.set_title(f"Δ medidas al modificar aristas — {ds}")
    ax.set_ylabel("Δ valor promedio")
    ax.set_xlabel("")
    ax.tick_params(axis="x", rotation=40, labelsize=8)
    ax.legend(fontsize=7, loc="upper right")
    ax.grid(axis="y", linestyle="--", alpha=0.4)

plt.tight_layout()
plt.savefig("grafico_aristas.png", dpi=150)
print("→ grafico_aristas.png guardado")
plt.close()

print("\nListo. Archivos generados:")
print("  tabla_stats.csv")
print("  grafico_tiempos.png")
print("  grafico_boxplot.png")
print("  grafico_aristas.png")
