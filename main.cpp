#include "graph.hpp"
// compilacion: g++ main.cpp graph.hpp dijkstra.cpp -o test
int main(){

    GraphIP grafoIP;
    GraphProteinas grafoProteinas;
    if (!grafoIP.loadDataset("train_test_network.csv")) {
         std::cout << "Error cargando CSV\n";
         return 1;
    }
    if (!grafoProteinas.loadDataset("yeast.edgelist")) {
         std::cout << "Error cargando edgelist\n";
         return 1;
    } 
    grafoProteinas.imprimirClosenessCentrality();
    return 0;
}