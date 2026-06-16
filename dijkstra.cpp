#include "graph.hpp"
#include <vector>
#include <queue>
#include <limits>
#include <iostream>
std::vector<double> Graph::dijkstra(const Graph& g, int source){
    // PQ usando min heap. Por defecto C++ lee el primer elemento del par pesoAcumulado, por ende
    // compara por peso.
    using pesoAcumulado = std::pair<double, int>; // (peso acumulado, hasta vertice v)
    std::priority_queue<pesoAcumulado, std::vector<pesoAcumulado>, std::greater<pesoAcumulado>> PQ;
    // inicializamos vector de distancias
    std::vector<double> dist(g.size(), std::numeric_limits<double>::infinity());
    dist[source] = 0; // la distancia al vertice raiz es 0.
    PQ.push({dist[source], source});

    while(!PQ.empty()){
        int u = PQ.top().second; // obtenemos el vertice
        double peso_acumulado = PQ.top().first; // obtenemos el peso
        PQ.pop(); // eliminamos
        // si encontramos un peso acumulado mayor a la distancia menor ya registrada,
        // lo ignoramos.
        if(peso_acumulado > dist[u]) continue;
        for( const Adyacencia& vecino : g.vecinos(u)){
            int z = vecino.destino;
            double peso = vecino.peso;
            if(dist[u]+peso < dist[z]){
                dist[z] = dist[u] + peso;
                PQ.push({dist[z],z}); // agregamos el nuevo camino
            }
        }
    }
    return dist;
}