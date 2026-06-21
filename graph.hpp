#include <vector>
#include <string>
#include <unordered_map>
#include <fstream>
#include <sstream>
#include <iostream>
#include <iomanip>
#include <limits>
#include <algorithm>
#include <cmath>
#include <unordered_set>
// Estructura que representa una arista (u -> v) con su peso
struct Edge {
    int u;
    int v;
    double w;
};
// Estructura que representa una arista (u -> v) con su peso con u vertice cualquiera.
// es util cuando queremos devolver los vecinos de un vertice (no repertimos el vertice de origen)
struct Adyacencia {
    int destino;
    double peso;
};

/// @brief Clase que representa un grafo base. Implementado con listas de adyacencia.
class Graph {
    protected:
    // label que en el caso de las IP's es una IP y en las proteinas es el nombre de aquella.
    std::unordered_map<std::string,int> labelToId;
    std::vector<std::string> idToLabel;
    std::vector<std::vector<Adyacencia>> listasAdj; //listas de adyacencia

    public: 
        virtual ~Graph() = default;

        /// @brief Método para obtener el id a partir del label en el mapa. Si 
        //  no se encuentra, el nodo se crea.
        /// @param label 
        /// @return el id
        int getId(const std::string& label) {
        //it es un iterador que nos devuelve (label, id) si lo encuentra y end() si no.
        auto it = labelToId.find(label);                                     
        if (it != labelToId.end())
            return it->second; // retornamos el id.
        // si no se encuentra, es un nuevo nodo, por ende lo insertamos en el mapa,
        // lo agregamos al fimal del vector de labels y lo añadimos a la lista de adyacencia.
        int id = (int)idToLabel.size();
        labelToId[label] = id;
        idToLabel.push_back(label);
        listasAdj.push_back({}); // crear lista de adj vacía para el nuevo nodo
        return id;
    }
    /// @brief Método para obtener el label de un nodo
    /// @param id el id del nodo
    /// @return el label del nodo
    const std::string& getLabel(int id) const {
        return idToLabel[id];
    }
    /// @brief Método para cargar el dataset en un grafo
    /// @param filename el nombre del archivo
    /// @return verdadero si se cargó correctamente, falso caso contrario.
    virtual bool loadDataset(const std::string& filename) = 0;

    /// @brief Representa el tamaño del grafo
    /// @return número de vertices
    int size() const{
        return listasAdj.size();
    }

    /// @brief Metodo para encontrar los vertices (mas el peso) vecinos de un vertice.
    /// @param v vertice del cual se buscan sus vecinos.
    /// @return un vector que contiene el par (vertice, peso)
    const std::vector<Adyacencia>& vecinos(int v) const {
    return listasAdj[v];
    }
    /// @brief Metodo que retorna el numero de vecinos salientes de un nodo v. 
    ///        Notar que en un grafo no dirigido es lo mismo que in degree.
    /// @param v nodo al cual se le busca el numero de vecinos
    /// @return la cantidad de vecinos
    int outDegree(int v){
       return vecinos(v).size();
    }
    /// @brief Método para calcular los grados de entrada de cada vertice
    ///       de id respectivo al indice en el vector.
    /// @return vector de grados de entrada.
    std::vector<int> inDegrees() const {
        std::vector<int> grados(size(), 0); // inicializamos en 0 por si hay nodos que son fuente.
        for (int u = 0; u < size(); u++)
            for (const Adyacencia& a : listasAdj[u])
                grados[a.destino]++; // como u apunta a a.destino, sabemos que tiene +1 grado de entrada
        return grados;
    }




    /// @brief Metodo para agregar una arista al grafo
    /// @param from el label del nodo origen
    /// @param to el label del nodo destino
    /// @param weight el peso de la arista
    virtual void agregarArista(const std::string& from, const std::string& to, double weight) = 0;

    
    /// @brief Metodo para obtener todas las aristas en el grafo. Recorre cada
    //  lista de adyacencia de un nodo u obteniendo el par (nodo, peso).
    /// @return un vector con todas las aristas.
    std::vector<Edge> getAristas() const {
        std::vector<Edge> result;
        for (int u = 0; u < listasAdj.size(); u++)
            for (const auto& [v, w] : listasAdj[u])
                result.push_back({u, v, w});
        return result;
    }
    /// @brief Método para mostrar las aristas en consola.
    virtual void imprimirAristas() = 0;
    /// @brief Método del algoritmo de Dijkstra para calcular el camino más corto en un grafo
    /// @param g el grafo, siempre será el mismo, pero dado que separamos el codigo
    ///           en otro archivo necesitamos el parametro.
    /// @param source el nodo raiz.
    /// @return un vector con la distancia mas corta a cualquier otro vertice i, donde i es el indice 
    ///         en el vector.
    std::vector<double> dijkstra(const Graph& g, int source);
    /// @brief Metodo que calcula la distancia entre dos nodos usando dijkstra.
    /// @param v nodo inicial
    /// @param u nodo final
    /// @return la distancia
    double distancia(int v, int u){
        std::vector<double> result = dijkstra(*this,v);
        return result[u];
    }
    /// @brief Método que calcula la medida 5. ClosenessCentrality. 
    ///        Por cada nodo, calcula la sumatoria de distancias a cualquier otro
    ///        usando distancia(v,u), el cual es el denominador.
    ///        Hubo un cambio en la formula ya que si v no podia visitar u, quedaba size - 1 / inf = 0 
    ///        (REVISAR ESO)
    /// @return retorna un arreglo donde el resultado para un nodo v es el indice v del arreglo.
    std::vector<double> closenessCentrality(){
        std::vector<double> result(size());
        for(int v = 0 ; v < size() ; v++){
            double sum_distancias = 0;
            double nodos_alcanzados = 0;
            std::vector<double> distancias = dijkstra(*this,v);
            for(int u = 0 ; u < size() ; u++){
                if(v == u) continue;
                double dist = distancias[u]; // no usamos la funcion distancia() ya que eso 
                                             // haria correr muchas veces dijkstra.
                if(dist != std::numeric_limits<double>::infinity()){
                    sum_distancias+=dist;
                    nodos_alcanzados++;
                }
            }
            if(nodos_alcanzados > 0 && sum_distancias > 0){
                result[v] = nodos_alcanzados / sum_distancias;
                std::cout << "obtenido para nodo " << v << std::endl;
            }
        }
        return result;
    }

    void imprimirClosenessCentrality() {
        std::vector<double> closeness = closenessCentrality();

        std::cout << "\n--- Resultados de Closeness Centrality ---\n";
        
        std::cout << std::fixed << std::setprecision(6);

        for (int i = 0; i < size(); i++) {
            std::cout << "Nodo: " << getLabel(i) 
                      << " (ID: " << i << ") -> " 
                      << closeness[i] << "\n";
        }

        std::cout << "------------------------------------------\n";
    }

    /**
     * @brief Método que calcula Betweenness Centrality.
     * Mide la fracción de caminos más cortos entre cada par de nodos (s,t) que pasan por un nodo v.
     * @return un vector donde el resultado para el nodo v está en el índice v.
     * El valor está normalizado a [0,1] dividiendo por la cantidad de pares posibles (size-1)(size-2).
     */
    std::vector<double> betweennessCentrality(){
        int n = size();
        std::vector<double> CB(n, 0.0); // resultado acumulado para cada nodo

        // por cada nodo fuente s usamos dijkstra y luego contamos caminos
        for(int s = 0; s < n; s++){
            // distancias mas cortas desde s 
            std::vector<double> dist = dijkstra(*this, s);

            // nodos alcanzables ordenados por distancia, lo recorremos como una pila
            std::vector<int> orden;
            for(int v = 0; v < n; v++)
                if(dist[v] != std::numeric_limits<double>::infinity())
                    orden.push_back(v);
            std::sort(orden.begin(), orden.end(),
                      [&](int a, int b){ return dist[a] < dist[b]; });

            // numero de caminos mas cortos de s a cada nodo
            std::vector<double> sigma(n, 0.0);
            sigma[s] = 1.0;
            // nodos previos a w en algun camino mas corto desde s
            std::vector<std::vector<int>> predecesores(n);

            // u es predecesor de w si esta justo antes en un camino mínimo
            for(int u : orden){
                for(const Adyacencia& a : vecinos(u)){
                    int w = a.destino;
                    if(dist[u] + a.peso == dist[w]){
                        sigma[w] += sigma[u];
                        predecesores[w].push_back(u);
                    }
                }
            }

            // recorremos orden al reves repartiendo la dependencia delta de cada nodo hacia sus predecesores
            std::vector<double> delta(n, 0.0);
            for(int i = (int)orden.size() - 1; i >= 0; i--){
                int w = orden[i];
                for(int v : predecesores[w]){
                    delta[v] += (sigma[v] / sigma[w]) * (1.0 + delta[w]);
                }
                if(w != s){
                    CB[w] += delta[w];
                }
            }
        }

        // normalizamos por la cantidad de pares (s,t) posibles distintos de v
        double pares = (double)(n - 1) * (n - 2);
        if(pares > 0){
            for(int v = 0; v < n; v++)
                CB[v] /= pares;
        }
        return CB;
    }

    void imprimirBetweennessCentrality() {
        std::vector<double> betweenness = betweennessCentrality();

        std::cout << "\n--- Resultados de Betweenness Centrality ---\n";

        std::cout << std::fixed << std::setprecision(6);

        for (int i = 0; i < size(); i++) {
            std::cout << "Nodo: " << getLabel(i)
                      << " (ID: " << i << ") -> "
                      << betweenness[i] << "\n";
        }

        std::cout << "------------------------------------------\n";
    }

    /**
     * @brief Método que calcula PageRank.
     * Mide la importancia de un nodo según la importancia de los nodos que lo apuntan.
     * @return un vector donde el resultado para el nodo v está en el índice v.
     */
    std::vector<double> pageRank(){
        int n = size();
        std::vector<double> pr(n, 1.0 / n); // rank inicial uniforme
        if(n == 0) return pr;

        const double d = 0.85;   // factor de amortiguacion habitual
        const int maxIter = 100; // tope de iteraciones
        const double tol = 1e-9; // corte cuando el rank ya casi no cambia

        for(int iter = 0; iter < maxIter; iter++){
            // salto aleatorio
            std::vector<double> nuevo(n, (1.0 - d) / n);

            double dangling = 0.0; // rank de los nodos sin salida
            for(int u = 0; u < n; u++){
                int grado = outDegree(u);
                if(grado == 0){
                    dangling += pr[u];
                    continue;
                }
                double aporte = d * pr[u] / grado;
                for(const Adyacencia& a : vecinos(u))
                    nuevo[a.destino] += aporte;
            }

            // el rank de los nodos sin salida se reparte por igual entre todos
            double aporteDangling = d * dangling / n;
            for(int v = 0; v < n; v++)
                nuevo[v] += aporteDangling;

            // diferencia total 
            double diff = 0.0;
            for(int v = 0; v < n; v++)
                diff += std::abs(nuevo[v] - pr[v]);

            pr = nuevo;
            if(diff < tol) break;
        }
        return pr;
    }

    void imprimirPageRank() {
        std::vector<double> pr = pageRank();

        std::cout << "\n--- Resultados de PageRank ---\n";

        std::cout << std::fixed << std::setprecision(6);

        for (int i = 0; i < size(); i++) {
            std::cout << "Nodo: " << getLabel(i)
                      << " (ID: " << i << ") -> "
                      << pr[i] << "\n";
        }

        std::cout << "------------------------------------------\n";
    }

    /**
     * @brief Método que calcula el Average Shortest Path.
     * el promedio de las distancias más cortas entre todos los pares de nodos.
     * Como el grafo puede no ser conexo, promediamos solo sobre los pares que sí se alcanzan.
     * @return el largo promedio, o 0 si no hay ningún par alcanzable.
     */
    double averageShortestPath(){
        int n = size();
        double sumaDistancias = 0.0;   // suma de d(s,t) sobre los pares alcanzables
        double paresAlcanzables = 0.0; // cantidad de pares (s,t) con camino

        // por cada nodo fuente s usamos dijkstra y sumamos sus distancias finitas
        for(int s = 0; s < n; s++){
            std::vector<double> dist = dijkstra(*this, s);
            for(int t = 0; t < n; t++){
                if(s == t) continue;
                if(dist[t] != std::numeric_limits<double>::infinity()){
                    sumaDistancias += dist[t];
                    paresAlcanzables++;
                }
            }
        }

        if(paresAlcanzables == 0) return 0.0;
        return sumaDistancias / paresAlcanzables;
    }

    void imprimirAverageShortestPath() {
        double L = averageShortestPath();

        std::cout << "\n--- Resultado de Average Shortest Path ---\n";

        std::cout << std::fixed << std::setprecision(6);

        std::cout << "Largo promedio del camino mas corto: " << L << "\n";

        std::cout << "------------------------------------------\n";
    }

    /**
     * @brief Método que calcula la excentricidad de cada nodo.
     * La excentricidad de v es la distancia más corta máxima desde v hacia cualquier otro nodo.
     * Como el grafo puede no ser conexo, tomamos el máximo solo sobre los nodos alcanzables.
     * @return un vector donde el resultado para el nodo v está en el índice v. Vale 0 si v no alcanza a nadie.
     */
    std::vector<double> eccentricity(){
        int n = size();
        std::vector<double> result(n, 0.0);

        // por cada nodo v usamos dijkstra y nos quedamos con su distancia mayor
        for(int v = 0; v < n; v++){
            std::vector<double> dist = dijkstra(*this, v);
            double maxDist = 0.0; // mayor distancia hacia un nodo alcanzable
            for(int u = 0; u < n; u++){
                if(v == u) continue;
                if(dist[u] != std::numeric_limits<double>::infinity() && dist[u] > maxDist)
                    maxDist = dist[u];
            }
            result[v] = maxDist;
        }
        return result;
    }

    void imprimirEccentricity() {
        std::vector<double> ecc = eccentricity();

        std::cout << "\n--- Resultados de Eccentricity ---\n";

        std::cout << std::fixed << std::setprecision(6);

        for (int i = 0; i < size(); i++) {
            std::cout << "Nodo: " << getLabel(i)
                      << " (ID: " << i << ") -> "
                      << ecc[i] << "\n";
        }

        std::cout << "------------------------------------------\n";
    }

    /**
     * @brief Método que calcula el Local Clustering Coefficient de cada nodo.
     * Mide la probabilidad de que los vecinos de v estén conectados entre sí.
     * LCC es una medida no dirigida, así que primero armamos la versión no dirigida del grafo.
     * @return un vector donde el resultado para el nodo v está en el índice v. Vale 0 si v tiene menos de 2 vecinos.
     */
    std::vector<double> localClusteringCoefficient(){
        int n = size();
        std::vector<double> result(n, 0.0);

        // a y b son vecinos si hay arista en cualquier sentido
        std::vector<std::unordered_set<int>> nbr(n);
        for(int u = 0; u < n; u++)
            for(const Adyacencia& a : vecinos(u))
                if(a.destino != u){
                    nbr[u].insert(a.destino);
                    nbr[a.destino].insert(u);
                }

        std::vector<char> esVecino(n, 0); // marca los vecinos del nodo actual
        for(int v = 0; v < n; v++){
            int grado = nbr[v].size();
            if(grado < 2) continue; // sin al menos 2 vecinos no hay pares posibles

            // marcamos los vecinos de v
            for(int a : nbr[v]) esVecino[a] = 1;

            // contamos aristas entre vecinos de v 
            long enlaces = 0;
            for(int a : nbr[v])
                for(int b : nbr[a])
                    if(b != a && esVecino[b])
                        enlaces++;

            result[v] = (double)enlaces / ((double)grado * (grado - 1));

            // limpiamos las marcas para el siguiente nodo
            for(int a : nbr[v]) esVecino[a] = 0;
        }
        return result;
    }

    void imprimirLocalClusteringCoefficient() {
        std::vector<double> lcc = localClusteringCoefficient();

        std::cout << "\n--- Resultados de Local Clustering Coefficient ---\n";

        std::cout << std::fixed << std::setprecision(6);

        for (int i = 0; i < size(); i++) {
            std::cout << "Nodo: " << getLabel(i)
                      << " (ID: " << i << ") -> "
                      << lcc[i] << "\n";
        }

        std::cout << "------------------------------------------\n";
    }




};

class GraphIP : public Graph{
    public:
    /// @brief la distancia se mide en saltos, así que cada arista tiene peso 1. Si la conexion ya existe en la misma dirección no acumulamos
    /// @param from 
    /// @param to 
    /// @param weight 
    void agregarArista(const std::string& from, const std::string& to, double weight) override{
        int u = getId(from);
        int v = getId(to);
        for (Adyacencia& a : listasAdj[u]) {
             if (a.destino == v) {
                return; // ya existe, no acumulamos 
            }
        }
        listasAdj[u].push_back(Adyacencia{v, 1.0});
    }

    /// @brief Cargamos desde el csv la ip origen y la ip destino de cada conexion.
    ///        implementación sacada del lab11 del curso.
    /// @param filename 
    /// @return 
    bool loadDataset(const std::string& filename) override{

        std::ifstream file(filename);

        if (!file.is_open()) {
            std::cout << "No pude abrir el archivo\n";
            return false;
        }

        std::string line;

        getline(file, line);
        while (getline(file, line)) {

            std::stringstream ss(line);

            std::vector<std::string> f;
            std::string field;

            while (getline(ss, field, ',')) {
                f.push_back(field);
            }

            if (f.size() < 44)
                continue;

            std::string ip_origen, ip_destino;

            ip_origen = f[0];
            ip_destino = f[2];


            // pasamos peso 1 
            
            agregarArista(ip_origen, ip_destino, 1.0);
        }

    return true;
    }
    void imprimirAristas() override{
        std::cout << std::left
                << std::setw(45) << "Origen"
                << " | "
                << std::setw(45) << "Destino"
                << " | "
                << "Peso" << '\n';

        for (const Edge& a : getAristas()) {

            std::cout << std::left
                    << std::setw(45) << getLabel(a.u)
                    << " | "
                    << std::setw(45) << getLabel(a.v)
                    << " | "
                    << a.w << '\n';
        }
    }

    /// @brief Método para calcular la medida 1. out Degree Centrality. Dado que usamos 
    ///        el indice para representar los vertices, usamos outDegree(i) con i en
    ///        {0,.., size - 1} para pasar por todos los vertices.
    /// @return un vector con el resultado de la medida.
    std::vector<double> outDegreeCentrality(){
        std::vector<double> result(size());
        for(int i = 0; i < size() ; i++){
            result[i] = (double) outDegree(i)/(size() - 1);
        }
        return result;
    }
    /// @brief Método para calcular la medida 1 . in Degree Centrality.
    ///       Es similar a outDegree, salvo que usamos la funcion inDegrees() para obtener
    ///       todos los grados de entrada previamente.
    /// @return 
    std::vector<double> inDegreeCentrality(){
        std::vector<double> result(size());
        std::vector<int> gradosEntrada = inDegrees();
        for(int i = 0; i < size() ; i++){
            result[i] = (double) gradosEntrada[i]/(size() - 1);
        }
        return result;
    }

    void imprimirInDegreeCentrality() {
        // 1. Obtenemos el vector con todos los resultados
        std::vector<double> result = inDegreeCentrality();

        std::cout << "\n--- Resultados de in Degree Centrality ---\n";
        
        // Opcional: Forzamos a que los doubles se impriman con 4 decimales
        std::cout << std::fixed << std::setprecision(4);

        // 2. Recorremos el vector. El índice 'i' corresponde al ID del nodo.
        for (int i = 0; i < size(); i++) {
            // Usamos tu método getLabel() para mostrar el nombre del nodo
            std::cout << "Nodo: " << getLabel(i) 
                      << " (ID: " << i << ") -> " 
                      << result[i] << "\n";
        }
        
        std::cout << "------------------------------------------\n";
    } 
    void imprimirOutDegreeCentrality() {
        // 1. Obtenemos el vector con todos los resultados
        std::vector<double> result = outDegreeCentrality();

        std::cout << "\n--- Resultados de out Degree Centrality ---\n";
        
        // Opcional: Forzamos a que los doubles se impriman con 4 decimales
        std::cout << std::fixed << std::setprecision(4);

        // 2. Recorremos el vector. El índice 'i' corresponde al ID del nodo.
        for (int i = 0; i < size(); i++) {
            // Usamos tu método getLabel() para mostrar el nombre del nodo
            std::cout << "Nodo: " << getLabel(i) 
                      << " (ID: " << i << ") -> " 
                      << result[i] << "\n";
        }
        
        std::cout << "------------------------------------------\n";
    }    



};

class GraphProteinas: public Graph{
    public:
    /// @brief Para cargar este dataset, leemos la mitad de las lineas, ya que por cada
    ///        arista (u,v), encontramos la que va en la otra dirección (v,u).
    /// @param filename 
    /// @return 
    bool loadDataset(const std::string& filename) override{
        std::ifstream file(filename);

        if (!file.is_open()) {
            std::cout << "No pude abrir el archivo\n";
            return false;
        }
        std::string line, discard;
        while (getline(file, line)) {
            if (!line.empty()) {
                        std::stringstream ss(line);
                        std::string p1, p2;

                        if (getline(ss, p1, '\t') && getline(ss, p2, '\t')) {
                            agregarArista(p1, p2);
                            agregarArista(p2, p1); 
                        }
                    }

                    getline(file, discard); // descartamos la siguiente linea
                }

        return true;       
    }
    void imprimirAristas() override{
        
        std::cout << std::left
                << std::setw(30) << "Origen"
                << " | "
                << std::setw(30) << "Destino" << "\n";
        for (const Edge& a : getAristas()) {

            std::cout << std::left
                    << std::setw(30) << getLabel(a.u)
                    << std::setw(30) << getLabel(a.v) << "\n";
        }
    }
    /// @brief 
    /// @param from 
    /// @param to 
    /// @param weight Asumimos un peso igual a 1.
    void agregarArista(const std::string& from, const std::string& to, double weight = 1.0) override{
        int u = getId(from);
        int v = getId(to);
        listasAdj[u].push_back({v, weight});
    }
    /// @brief Método para calcular la medida 1. Degree Centrality. Dado que usamos 
    ///        el indice para representar los vertices, usamos outDegree(i) con i en
    ///        {0,.., size - 1} para pasar por todos los vertices.
    /// @return un vector con el resultado de la medida.
    std::vector<double> degreeCentrality(){
        std::vector<double> result(size());
        for(int i = 0; i < size() ; i++){
            result[i] = (double) outDegree(i)/(size() - 1);
        }
        return result;
    }
    void imprimirDegreeCentrality() {
        // 1. Obtenemos el vector con todos los resultados
        std::vector<double> result = degreeCentrality();

        std::cout << "\n--- Resultados de  Degree Centrality ---\n";
        
        // Opcional: Forzamos a que los doubles se impriman con 4 decimales
        std::cout << std::fixed << std::setprecision(4);

        // 2. Recorremos el vector. El índice 'i' corresponde al ID del nodo.
        for (int i = 0; i < size(); i++) {
            // Usamos tu método getLabel() para mostrar el nombre del nodo
            std::cout << "Nodo: " << getLabel(i) 
                      << " (ID: " << i << ") -> " 
                      << result[i] << "\n";
        }
        
        std::cout << "------------------------------------------\n";
    } 

};