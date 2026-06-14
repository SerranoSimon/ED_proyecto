#include <vector>
#include <string>
#include <unordered_map>
#include <fstream>
#include <sstream>
#include <iostream>
#include <iomanip>
// Estructura que representa una arista (u -> v) con su peso
struct Edge {
    int u;
    int v;
    double w;
};
/// @brief Clase que representa un grafo base. Implementado con listas de adyacencia.
class Graph {
    protected:
    // label que en el caso de las IP's es una IP y en las proteinas es el nombre de aquella.
    std::unordered_map<std::string,int> labelToId;
    std::vector<std::string> idToLabel;
    std::vector<std::vector<std::pair<int,double>>> listasAdj; //listas de adyacencia

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
    int size(){
        return listasAdj.size();
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






};

class GraphIP : public Graph{
    public:
    /// @brief En esta implementación se revisa si ya existe una conexion en la misma dirección
    ///        de los mismos dispositivos, de ser así, sumamos la duracion, es decir, el peso
    ///        es la duración acumulada. Si no es una arista repetida, la agregamos.
    /// @param from 
    /// @param to 
    /// @param weight 
    void agregarArista(const std::string& from, const std::string& to, double weight) override{
        int u = getId(from);
        int v = getId(to);
        for (auto& [vecino, peso] : listasAdj[u]) {
             if (vecino == v) {
                peso+=weight;
                return;
            }
        }
        listasAdj[u].push_back({v, weight});
    }

    /// @brief Cargamos desde el csv la ip origen, ip destino y duracion de cada conexion.
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
            double duracion;

            ip_origen = f[0];
            ip_destino = f[2];


            duracion = f[6].empty() ? 0.0 : std::stod(f[6]);
            
            agregarArista(ip_origen,ip_destino,duracion);
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
};