#!/usr/bin/env python3
"""
analizar_resultados.py — versión corregida
Corrección: el filtro `repeticion != 0` eliminaba incorrectamente la primera
repetición (rep 0) de cada medida de tiempo. Las filas de construcción se
separan por nombre de medida, no por número de repetición.
"""

import pandas as pd
import matplotlib.pyplot as plt
import sys

# ──────────────────────────────────────────────
#  1. CARGAR CSV
# ──────────────────────────────────────────────
try:
    df_med = pd.read_csv("resultados_medidas.csv")
    df_ari = pd.read_csv("resultados_aristas.csv")
except FileNotFoundError as e:
    sys.exit(f"ERROR: {e}")

# Separar por nombre de medida (corrección: no filtrar por repeticion == 0)
construccion_medidas = ["vertices", "aristas", "construccion_ms", "memoria_delta_KB"]
df_const = df_med[df_med["medida"].isin(construccion_medidas)].copy()
df_timed = df_med[~df_med["medida"].isin(construccion_medidas)].copy()

# ──────────────────────────────────────────────
#  2. TABLA DE CONSTRUCCIÓN
# ──────────────────────────────────────────────
print("\n===== CONSTRUCCIÓN DEL GRAFO =====")
tabla_const = df_const.pivot_table(
    index="dataset", columns="medida", values="valor", aggfunc="first"
)
print(tabla_const.to_string())
print()

# ──────────────────────────────────────────────
#  3. TABLA DE ESTADÍSTICAS POR MEDIDA
# ──────────────────────────────────────────────
stats = (
    df_timed
    .groupby(["dataset", "medida"])["valor"]
    .agg(media="mean", varianza="var", std="std", min_ms="min", max_ms="max")
    .reset_index()
)
stats["cv_%"] = (stats["std"] / stats["media"] * 100).round(2)

print("===== ESTADÍSTICAS DE TIEMPOS (ms) — 10 repeticiones completas =====")
print(stats.to_string(index=False, float_format="{:.4f}".format))
print()

stats.to_csv("tabla_stats.csv", index=False, float_format="%.6f")
print("→ tabla_stats.csv guardada")

# Orden consistente de medidas para los gráficos
orden_medidas = [
    "averageShortestPath", "betweenness", "closeness", "eccentricity",
    "inDegreeCentrality", "localClusteringCoeff", "outDegreeCentrality", "pageRank",
]
datasets = ["GraphIP", "GraphProteinas"]

# ──────────────────────────────────────────────
#  4. GRÁFICO 1: Tiempos promedio por medida (barras, escala log)
# ──────────────────────────────────────────────
fig, ax = plt.subplots(figsize=(12, 6))

bar_width = 0.35
colores   = ["steelblue", "darkorange"]
x         = range(len(orden_medidas))

for i, ds in enumerate(datasets):
    sub = (
        stats[stats["dataset"] == ds]
        .set_index("medida")
        .reindex(orden_medidas)
    )
    offset = [xi + i * bar_width for xi in x]
    ax.bar(
        offset, sub["media"], bar_width,
        yerr=sub["std"], label=ds,
        color=colores[i], alpha=0.85, capsize=4,
    )

ax.set_yscale("log")
ax.set_ylabel("Tiempo promedio (ms) — escala log")
ax.set_title("Tiempo promedio por medida (10 repeticiones, ±1σ)")
ax.set_xticks([xi + bar_width / 2 for xi in x])
ax.set_xticklabels(orden_medidas, rotation=30, ha="right", fontsize=9)
ax.legend()
ax.grid(axis="y", linestyle="--", alpha=0.5)
plt.tight_layout()
plt.savefig("grafico_tiempos.png", dpi=150)
print("→ grafico_tiempos.png guardado")
plt.close()

# ──────────────────────────────────────────────
#  5. GRÁFICO 2: Box plot — distribución de las 10 repeticiones
# ──────────────────────────────────────────────
fig, axes = plt.subplots(1, 2, figsize=(14, 5), sharey=False)

for ax, ds in zip(axes, datasets):
    sub     = df_timed[df_timed["dataset"] == ds]
    grupos  = [sub[sub["medida"] == m]["valor"].values for m in orden_medidas]
    bp = ax.boxplot(grupos, tick_labels=orden_medidas, patch_artist=True)
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
fig, axes = plt.subplots(1, 2, figsize=(14, 5))

for ax, ds in zip(axes, datasets):
    sub = df_ari[df_ari["dataset"] == ds].copy()
    if sub.empty:
        ax.set_title(f"{ds} — sin datos")
        continue
    sub["etiqueta"] = sub["operacion"] + "_" + sub["tipo_arista"]
    pivot = sub.pivot_table(
        index="medida", columns="etiqueta", values="delta", aggfunc="mean"
    ).reindex(orden_medidas)
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
