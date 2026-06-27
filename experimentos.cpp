#include "graph.hpp"     // Aquí se definene las clases Graph, GraphIP y GraphProteinas.
#include <chrono>        // Proporciona el reloj de alta resolución (high_resolution_clock) para medir tiempos en milisegundos de algoritmos y carga.
#include <fstream>       // Flujo de entrada/salida de archivos; se usa para dirigir las métricas en "resultados_medidas.csv" y "resultados_aristas.csv".
#include <iostream>      // Entrada y salida estándar.
#include <sstream>       // Streams basados en strings; para en el formato y parseo interno de streams antes de guardarlos.
#include <numeric>       // Introduce std::accumulate, que sirve para sumar los vectores de tiempo de las 10 repeticiones antes de promediarlos.
#include <cmath>         // Funciones matemáticas; aqui se usa para calcular la varianza y desviación estándar de los tiempos.
#include <climits>       // Define límites de tipos de datos enteros fundamentales (ej. INT_MAX), que se usan en la lógica de control o vecindad.
#include <map>           // Implementa std::map (árbol balanceado); se utiliza para estructurar internamente el diccionario de funciones de medición.
#include <functional>    // Permite usar std::function para encapsular las llamadas a los métodos de centralidad y pasarlos como callbacks en los loops.
#include <algorithm>     // Proporciona algoritmos genéricos optimizados como std::find, std::min o std::max para procesar arreglos y colecciones.



/// UTILIDADES
/// @brief Mide la cantidad de memoria RAM usada al cargar el grafo. Consiera el RSS (Resident Set Size).
/// @return Cantidad de memoria RAM en kilobytes (KB) utilizada por el programa
///         o -1 en caso de que el entorno no sea compatible con Linux.
static long getRSS() {
    std::ifstream f("/proc/self/status");
    std::string line;
    while (std::getline(f, line)) {
        if (line.rfind("VmRSS", 0) == 0) {
            // Formato: "VmRSS:   12345 kB"
            // Se extraen solo los dígitos
            std::string digits;
            for (char c : line)
                if (std::isdigit(c)) digits += c;
            if (!digits.empty()) return std::stol(digits);
        }
    }
    return -1; // no compatible (macOS, Windows)
}

// Redefinición de tipo para utilizar el reloj de alta resolución de la STL.
using Clock = std::chrono::high_resolution_clock;

/// @brief Calcula la diferencia de tiempo transcurrido en milisegundos respecto a un punto de control.
/// @param t0 Instante de tiempo inicial capturado previamente por el reloj.
/// @return Cantidad de milisegundos transcurridos desde t0 hasta el momento de llamado.
static double elapsed_ms(Clock::time_point t0) {
    return std::chrono::duration<double, std::milli>(Clock::now() - t0).count();
}

/// @brief Struct para mitigar el impacto del overhead de entrada/salida, producido por ClosenessCentrality
///        que imprime un mensaje por cada nodo lo que distorciona el tiempo medido para el algoritmo.
struct SuppressCout {
    std::streambuf* old;     // Almacena el puntero a la pantalla.
    std::ofstream   devNull; // Stream de archivo de salida dirigido al /dev/null.

    /// @brief Constructor de la estructura. Hace la redirección de std::cout.
    SuppressCout() : devNull("/dev/null") {
        old = std::cout.rdbuf(devNull.rdbuf());
    }

    /// @brief Destructor de la estructura.
    ~SuppressCout() {
        std::cout.rdbuf(old);
    }
};

/// @brief Entrega la varianza y la media muestral de un vector de tiempos.
/// @param v Referencia constante a un vector con las muestras de tiempo.
/// @return Un par ordenado (std::pair) que contiene en su primera componente la media aritmética 
///         y en su segunda componente la varianza muestral insesgada (osea dividida por n-1).
static std::pair<double, double> stats(const std::vector<double>& v) {
    double mean = std::accumulate(v.begin(), v.end(), 0.0) / v.size();
    double var  = 0.0;
    for (double x : v) var += (x - mean) * (x - mean);
    if (v.size() > 1) var /= (v.size() - 1);
    return {mean, var};
}



/// SUBCLASES EXPERIMENTALES (EXTENSIONES POR HERENCIA)
/// @brief Extensión de la clase GraphIP para permitir modificaciones dinámicas.
class GraphIPExp : public GraphIP {
public:
    /// @brief Elimina una arista dirigida (from -> to) basándose en sus etiquetas textuales.
    /// @param from Etiqueta (IP) del vértice origen.
    /// @param to Etiqueta (IP) del vértice destino.
    /// @return true si la arista fue encontrada y eliminada con éxito; false en caso contrario.
    bool removeArista(const std::string& from, const std::string& to) {
        auto it = labelToId.find(from);
        if (it == labelToId.end()) return false;
        int u = it->second;
        auto jt = labelToId.find(to);
        if (jt == labelToId.end()) return false;
        int v = jt->second;
        auto& adj = listasAdj[u];
        for (auto i = adj.begin(); i != adj.end(); ++i) {
            if (i->destino == v) { adj.erase(i); return true; }
        }
        return false;
    }
};

/// @brief Extensión de la clase GraphProteinas para permitir modificaciones dinámicas.
class GraphProteinasExp : public GraphProteinas {
public:
    std::vector<double> outDegreeCentrality() { return degreeCentrality(); }
    std::vector<double> inDegreeCentrality()  { return degreeCentrality(); }

    /// @brief Elimina una arista dirigida (from -> to) basándose en sus etiquetas textuales.
    /// @note Al simular un grafo no dirigido mediante aristas dirigidas duplicadas, 
    ///       este método se debe llamar en ambos sentidos.
    /// @param from Etiqueta (Nombre) de la proteína origen.
    /// @param to Etiqueta (Nombre) de la proteína destino.
    /// @return true si la arista fue encontrada y eliminada con éxito; false en caso contrario.
    bool removeArista(const std::string& from, const std::string& to) {
        auto it = labelToId.find(from);
        if (it == labelToId.end()) return false;
        int u = it->second;
        auto jt = labelToId.find(to);
        if (jt == labelToId.end()) return false;
        int v = jt->second;
        auto& adj = listasAdj[u];
        for (auto i = adj.begin(); i != adj.end(); ++i) {
            if (i->destino == v) { adj.erase(i); return true; }
        }
        return false;
    }
};



/// SELECCIÓN DE ARISTAS PARA EL EXPERIMENTO
/// @brief Calcula la media aritmética de un vector de valores de tipo double.
/// @param v Referencia constante a un vector con valores reales.
/// @return El valor promedio de los elementos del vector; devuelve 0.0 si el vector está vacío.
static double promedioVector(const std::vector<double>& v) {
    if (v.empty()) return 0.0;
    return std::accumulate(v.begin(), v.end(), 0.0) / v.size();
}

/// @brief Selecciona tres aristas existentes en el grafo con diferentes caracteristicas.
/// @details Recorre el grafo para identificar tres categorías de aristas basándose en el grado de salida:
///          1. Hub (arista que parte desde el nodo con mayor conectividad de la red).
///          2. Periférica (arista que parte desde el nodo con la menor conectividad mayor a cero).
///          3. Aleatoria (arista en una posición fija intermedia de la lista global).
/// @param g Referencia al objeto grafo del cual se extraerán las aristas.
/// @return Un vector que contiene exactamente tres estructuras Edge [hub, periférica, aleatoria].
template<typename G>
std::vector<Edge> seleccionarAristas(G& g) {
    std::vector<Edge> todas = g.getAristas();
    if (todas.empty()) return {};

    // Hub: nodo con mayor grado de salida
    int hub = 0;
    for (int i = 1; i < g.size(); i++)
        if (g.outDegree(i) > g.outDegree(hub)) hub = i;

    // Periférico: nodo con menor grado de salida > 0
    int periferico = -1;
    int minDeg = INT_MAX;
    for (int i = 0; i < g.size(); i++) {
        int d = g.outDegree(i);
        if (d > 0 && d < minDeg) { minDeg = d; periferico = i; }
    }

    Edge eHub       = {-1, -1, 0.0};
    Edge ePeriferica= {-1, -1, 0.0};
    Edge eAleatoria = todas[todas.size() / 2]; // centro de la lista

    for (const Edge& e : todas) {
        if (e.u == hub        && eHub.u       < 0) eHub        = e;
        if (e.u == periferico && ePeriferica.u < 0) ePeriferica = e;
    }

    // Se verifica que las tres sean distintas
    if (ePeriferica.u == eHub.u && ePeriferica.v == eHub.v)
        ePeriferica = todas[todas.size() / 3];
    if (eAleatoria.u == eHub.u && eAleatoria.v == eHub.v)
        eAleatoria = todas[todas.size() / 4];

    return { eHub, ePeriferica, eAleatoria };
}

/// @brief Busca un vértice de destino a distancia geodésica exacta de 2 para simular la adición de una nueva arista puente.
/// @details Ejecuta el algoritmo de Dijkstra desde el nodo fuente para encontrar un vértice 't' con costo 
///          acumulado igual a 2.0, verificando que no exista un enlace directo preexistente.
///          Esto para asegurar que la nueva arista sea un enlace puente medible que acorte distancias reales en el sistema.
/// @template G Tipo de la clase del grafo (debe heredar de la clase base Graph).
/// @param g Referencia al objeto grafo sobre el cual se realiza la exploración.
/// @param fuente Identificador numérico del nodo raíz origen de la búsqueda.
/// @return Una Struct Arista representativa de la arista candidata; 
///         si no se halla ningún candidato elegible, retorna una Arista con identificador origen u = -1.
template<typename G>
Edge buscarAristaNueva(G& g, int fuente) {
    std::vector<double> dist = g.dijkstra(g, fuente);
    for (int t = 0; t < g.size(); t++) {
        if (t == fuente) continue;
        if (dist[t] == 2.0) {                      // a distancia 2, luego no hay arista directa
            bool directa = false;
            for (const Adyacencia& a : g.vecinos(fuente))
                if (a.destino == t) { directa = true; break; }
            if (!directa) return {fuente, t, 1.0};
        }
    }
    return {-1, -1, 0.0};
}



/// MEDICIÓN
/// @brief Constante que define el número de réplicas.
static const int REPS = 10;

/// @brief Test Bench que ejecuta la función métrica un número fijo de veces.
/// @param fn Invocable que encapsula el algoritmo de centralidad a evaluar.
/// @return Un vector de tamaño REPS conteniendo los tiempos individuales de cada réplica en milisegundos.
template<typename F>
std::vector<double> medir(F fn) {
    std::vector<double> tiempos;
    tiempos.reserve(REPS);
    for (int i = 0; i < REPS; i++) {
        SuppressCout sc;               // silencia cout en cada repetición
        auto t0 = Clock::now();
        fn();
        tiempos.push_back(elapsed_ms(t0));
    }
    return tiempos;
}



/// OBTENER VALOR ESCALAR DE CADA MEDIDA (EXPERIMENTO ARISTAS)
/// @brief Evalúa y colapsa una medida de centralidad específica en un único indicador escalar representativo para la red.
/// @param g Referencia al objeto grafo evaluado.
/// @param nombre Identificador textual de la métrica de centralidad a calcular.
/// @return El valor promedio representativo de la métrica en todo el grafo. 
///         Retorna 0.0 si la cadena proporcionada no coincide con ninguna métrica.
template<typename G>
double valorMedida(G& g, const std::string& nombre) {
    SuppressCout sc;
    if (nombre == "outDegreeCentrality")
        return promedioVector(g.outDegreeCentrality());
    if (nombre == "inDegreeCentrality")
        return promedioVector(g.inDegreeCentrality());
    if (nombre == "closeness")
        return promedioVector(g.closenessCentrality());
    if (nombre == "betweenness")
        return promedioVector(g.betweennessCentrality());
    if (nombre == "pageRank")
        return promedioVector(g.pageRank());
    if (nombre == "averageShortestPath")
        return g.averageShortestPath();
    if (nombre == "eccentricity")
        return promedioVector(g.eccentricity());
    if (nombre == "localClusteringCoeff")
        return promedioVector(g.localClusteringCoefficient());
    return 0.0;
}



/// BLOQUE A: CONSTRUCCIÓN + MEMORIA
template<typename G>
void medirConstruccion(const std::string& nombre_dataset,
                    const std::string& archivo,
                    std::ofstream&     csv)
{
    std::cerr << "\n[" << nombre_dataset << "] midiendo construcción...\n";

    long antes = getRSS();
    auto t0    = Clock::now();
    G g;
    g.loadDataset(archivo);
    double t_ms   = elapsed_ms(t0);
    long despues  = getRSS();
    long delta_kb = (despues >= 0 && antes >= 0) ? (despues - antes) : -1;

    int V = g.size();
    int E = (int)g.getAristas().size();

    // Fila en CSV: dataset, medida, repeticion, valor
    csv << nombre_dataset << ",vertices,0,"         << V       << "\n";
    csv << nombre_dataset << ",aristas,0,"          << E       << "\n";
    csv << nombre_dataset << ",construccion_ms,0,"  << t_ms    << "\n";
    csv << nombre_dataset << ",memoria_delta_KB,0," << delta_kb<< "\n";

    std::cerr << "  vertices: "   << V
            << " | aristas: "   << E
            << " | tiempo: "    << t_ms    << " ms"
            << " | mem delta: " << delta_kb<< " KB\n";
}



///  BLOQUE B: 8 MEDIDAS × 10 REPETICIONES
template<typename G>
void medirMedidas(const std::string& nombre_dataset,
                const std::string& archivo,
                std::ofstream&     csv)
{
    std::cerr << "\n[" << nombre_dataset << "] cargando grafo para medidas...\n";
    G g;
    {
        SuppressCout sc;   // loadDataset puede imprimir mensajes
        g.loadDataset(archivo);
    }
    std::cerr << "  V=" << g.size()
            << " E=" << g.getAristas().size() << "\n";

    // Lista de (nombre, lambda), misma para ambos grafos.
    // Para GraphProteinas outDegree == inDegree porque es bidireccional.
    std::vector<std::pair<std::string, std::function<void()>>> medidas = {
        {"outDegreeCentrality",        [&]{ g.outDegreeCentrality(); }},
        {"inDegreeCentrality",         [&]{ g.inDegreeCentrality(); }},
        {"closeness",                  [&]{ g.closenessCentrality(); }},
        {"betweenness",                [&]{ g.betweennessCentrality(); }},
        {"pageRank",                   [&]{ g.pageRank(); }},
        {"averageShortestPath",        [&]{ g.averageShortestPath(); }},
        {"eccentricity",               [&]{ g.eccentricity(); }},
        {"localClusteringCoeff",       [&]{ g.localClusteringCoefficient(); }},
    };

    for (auto& [nombre, fn] : medidas) {
        std::cerr << "  midiendo " << nombre << " (" << REPS << " reps)...\n";
        auto tiempos = medir(fn);
        auto [media, var] = stats(tiempos);

        // Una fila por repetición
        for (int i = 0; i < REPS; i++)
            csv << nombre_dataset << "," << nombre << "," << i << "," << tiempos[i] << "\n";

        std::cerr << "    media=" << media << " ms  |  var=" << var << "\n";
    }
}



//  BLOQUE C: EXPERIMENTO DE ARISTAS
template<typename G>   // G debe tener removeArista (GraphIPExp / GraphProteinasExp)
void experimentoAristas(const std::string& nombre_dataset,
                        const std::string& archivo,
                        std::ofstream&     csv)
{
    std::cerr << "\n[" << nombre_dataset << "] experimento de aristas...\n";
    G g;
    { SuppressCout sc; g.loadDataset(archivo); }

    const std::vector<std::string> nombres_medidas = {
        "outDegreeCentrality", "inDegreeCentrality", "closeness",
        "betweenness", "pageRank", "averageShortestPath",
        "eccentricity", "localClusteringCoeff"
    };

    const std::vector<std::string> tipos = {"hub", "periferica", "aleatoria"};

    /// EXPERIMENTO: QUITAR una arista existente
    std::vector<Edge> existentes = seleccionarAristas(g);

    for (int k = 0; k < (int)existentes.size(); k++) {
        const Edge& e = existentes[k];
        if (e.u < 0) { std::cerr << "  (arista " << tipos[k] << " no encontrada)\n"; continue; }

        const std::string& from = g.getLabel(e.u);
        const std::string& to   = g.getLabel(e.v);
        std::cerr << "  quitar " << tipos[k] << ": " << from << " -> " << to << "\n";

        // Medir ANTES
        std::map<std::string, double> antes;
        for (const auto& m : nombres_medidas)
            antes[m] = valorMedida(g, m);

        // Quitar la arista
        g.removeArista(from, to);

        // Medir DESPUÉS y escribir CSV
        for (const auto& m : nombres_medidas) {
            double despues = valorMedida(g, m);
            double delta   = despues - antes[m];
            csv << nombre_dataset << ",quitar," << tipos[k] << ","
                << m << "," << antes[m] << "," << despues << "," << delta << "\n";
        }

        // Restaurar la arista para no afectar las siguientes iteraciones
        g.agregarArista(from, to, e.w);
        std::cerr << "    arista restaurada.\n";
    }

    /// EXPERIMENTO: AGREGAR una arista que no existe
    // Se buscan 3 pares de nodos sin arista directa usando 3 nodos fuente distintos
    std::vector<int> fuentes = {0, g.size() / 3, 2 * g.size() / 3};

    for (int k = 0; k < (int)fuentes.size(); k++) {
        Edge nueva = buscarAristaNueva(g, fuentes[k]);
        if (nueva.u < 0) {
            std::cerr << "  (no se encontró par sin arista para fuente " << fuentes[k] << ")\n";
            continue;
        }
        const std::string& from = g.getLabel(nueva.u);
        const std::string& to   = g.getLabel(nueva.v);
        std::cerr << "  agregar " << tipos[k] << ": " << from << " -> " << to << "\n";

        // Medir ANTES
        std::map<std::string, double> antes;
        for (const auto& m : nombres_medidas)
            antes[m] = valorMedida(g, m);

        // Agregar la arista nueva
        g.agregarArista(from, to, 1.0);

        // Medir DESPUÉS
        for (const auto& m : nombres_medidas) {
            double despues = valorMedida(g, m);
            double delta   = despues - antes[m];
            csv << nombre_dataset << ",agregar," << tipos[k] << ","
                << m << "," << antes[m] << "," << despues << "," << delta << "\n";
        }

        // Restaurar el estado original
        g.removeArista(from, to);
        std::cerr << "    arista temporal eliminada.\n";
    }
}



/// MAIN
int main(int argc, char* argv[]) {

    if (argc < 3) {
        std::cerr << "Uso: ./experimentos <archivo_IP.csv> <archivo_proteinas.edgelist>\n";
        std::cerr << "Ejemplo: ./experimentos train_test_network.csv yeast.edgelist\n";
        return 1;
    }

    const std::string archivoIP  = argv[1];
    const std::string archivoProt= argv[2];

    // Archivos de salida
    std::ofstream csvMedidas("resultados_medidas.csv");
    std::ofstream csvAristas("resultados_aristas.csv");

    // Headers
    csvMedidas << "dataset,medida,repeticion,valor\n";
    csvAristas << "dataset,operacion,tipo_arista,medida,valor_antes,valor_despues,delta\n";

    /// BLOQUE A: CONSTRUCCIÓN + MEMORIA
    std::cerr << "BLOQUE A: CONSTRUCCIÓN + MEMORIA\n";
    medirConstruccion<GraphIPExp>       ("GraphIP",        archivoIP,   csvMedidas);
    medirConstruccion<GraphProteinasExp>("GraphProteinas", archivoProt, csvMedidas);

    ///  BLOQUE B: MEDIDAS
    std::cerr << "\nBLOQUE B: MEDIDAS\n";
    medirMedidas<GraphIPExp>       ("GraphIP",        archivoIP,   csvMedidas);
    medirMedidas<GraphProteinasExp>("GraphProteinas", archivoProt, csvMedidas);

    /// BLOQUE C: EXPERIMENTO DE ARISTAS
    std::cerr << "\nBLOQUE C: EXPERIMENTO DE ARISTAS\n";
    experimentoAristas<GraphIPExp>       ("GraphIP",        archivoIP,   csvAristas);
    experimentoAristas<GraphProteinasExp>("GraphProteinas", archivoProt, csvAristas);

    csvMedidas.close();
    csvAristas.close();

    std::cerr << "Archivos generados:\n";
    std::cerr << "  resultados_medidas.csv  (construccion + 8 medidas x 10 reps)\n";
    std::cerr << "  resultados_aristas.csv  (quitar/agregar aristas)\n";
    return 0;
}
