#include "graph.hpp"
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
    grafoIP.imprimirAristas();
    //grafoProteinas.imprimirAristas();
    return 0;
}