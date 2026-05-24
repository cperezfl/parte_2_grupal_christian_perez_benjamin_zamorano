#include <iostream>
#include <vector>
#include <cmath>
#include <chrono>
#include <omp.h>
#include <random>
#include <queue>
#include <iomanip>

// Configuracion estipulada en los requisitos minimos del enunciado
const int VECTORES = 20000;
const int DIMENSION = 128;
const int TOP_K = 10;

// Estructura para almacenar los datos de los vecinos cercanos
struct Vecino {
    double similitud;
    int id;

    // Operador de comparacion invertido para estructurar un Max-Heap en la cola de prioridad
    bool operator<(const Vecino& otro) const {
        return similitud > otro.similitud; 
    }
};

// Estructura para registrar las metricas del benchmark automatizado
struct ResultadoBenchmark {
    int hilos;
    double tiempo;
    double speedup;
    double eficiencia;
};

// Generacion sintetica de vectores normalizados a norma 1 (Similitud de Coseno nativa)
void generar_embeddings_normalizados(std::vector<double>& X) {
    std::mt19937 generator(42);
    std::uniform_real_distribution<double> dist(-1.0, 1.0);

    for (int i = 0; i < VECTORES; ++i) {
        double norma = 0.0;
        long long base_idx = (long long)i * DIMENSION;

        // 1. Asignar valores aleatorios orientados a embeddings
        for (int d = 0; d < DIMENSION; ++d) {
            X[base_idx + d] = dist(generator);
            norma += X[base_idx + d] * X[base_idx + d];
        }

        norma = std::sqrt(norma);

        // 2. Normalizar el vector a norma 1
        for (int d = 0; d < DIMENSION; ++d) {
            if (norma > 0.0) {
                X[base_idx + d] /= norma;
            } else {
                X[base_idx + d] = 0.0;
            }
        }
    }
}

// Algoritmo de procesamiento por bloques y busqueda in-place de candidatos cercanos
double ejecutar_similitud_bloques(int num_hilos, const std::vector<double>& X, 
                                  std::vector<std::vector<Vecino>>& top_k_resultados) {
    
    omp_set_num_threads(num_hilos);
    auto inicio = std::chrono::high_resolution_clock::now();

    // Region paralela estructurada
    #pragma omp parallel
    {
        // schedule(dynamic, 64) mitiga el desbalance de la matriz triangular de comparacion
        #pragma omp for schedule(dynamic, 64)
        for (int i = 0; i < VECTORES; ++i) {
            
            // Cola de prioridad local por fila (Max-Heap privado por hilo para evitar carreras de datos)
            std::priority_queue<Vecino> heap_local;
            long long idx_i = (long long)i * DIMENSION;

            for (int j = 0; j < VECTORES; ++j) {
                if (i == j) continue; // Ignorar la auto-similitud

                long long idx_j = (long long)j * DIMENSION;
                double producto_punto = 0.0;

                // Computo del producto punto de vectores con norma unitaria
                for (int d = 0; d < DIMENSION; ++d) {
                    producto_punto += X[idx_i + d] * X[idx_j + d];
                }

                // Administracion dinamica de la memoria del Top-10 sobre la marcha
                if (heap_local.size() < TOP_K) {
                    heap_local.push({producto_punto, j});
                } else if (producto_punto > heap_local.top().similitud) {
                    heap_local.pop(); // Descartar el peor candidato del set actual
                    heap_local.push({producto_punto, j}); // Insertar el nuevo vecino mas cercano
                }
            }

            // Volcar los elementos ordenados del heap privado en la matriz global compartida
            int idx_insercion = TOP_K - 1;
            while (!heap_local.empty()) {
                top_k_resultados[i][idx_insercion] = heap_local.top();
                heap_local.pop();
                idx_insercion--;
            }
        }
    }

    auto fin = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> tiempo = fin - inicio;
    return tiempo.count();
}

int main() {
    std::cout << "Asignando memoria para los embeddings de entrada (Aprox. 20.48 MB)..." << std::endl;
    std::vector<double> X((long long)VECTORES * DIMENSION);

    std::cout << "Generando y normalizando embeddings artificiales..." << std::endl;
    generar_embeddings_normalizados(X);

    // Estructura de salida acotada: 20,000 filas por 10 columnas 
    // Evita por completo la reserva inviable de los 3.2 GB de la matriz completa
    std::vector<std::vector<Vecino>> top_k_final(VECTORES, std::vector<Vecino>(TOP_K, {-1.0, -1}));

    std::vector<int> configuracion_hilos = {1, 2, 4, 8};
    std::vector<ResultadoBenchmark> tabla_resultados;
    double tiempo_secuencial = 0.0;

    std::cout << "\nIniciando Ciclo de Pruebas Automatizado (Benchmark)..." << std::endl;

    for (int hilos : configuracion_hilos) {
        std::cout << ">> Ejecutando analisis con " << hilos << " hilo(s)... " << std::flush;
        
        double t_ejecucion = ejecutar_similitud_bloques(hilos, X, top_k_final);
        std::cout << "Completado en " << t_ejecucion << " segundos." << std::endl;

        if (hilos == 1) {
            tiempo_secuencial = t_ejecucion;
        }

        double speedup = tiempo_secuencial / t_ejecucion;
        double eficiencia = speedup / hilos;

        tabla_resultados.push_back({hilos, t_ejecucion, speedup, eficiencia});
    }

    // Reporte 1: Tabla de Metricas de Rendimiento Automatica
    std::cout << "\n====================================================================\n";
    std::cout << "TABLA 1: METRICAS DE INFRAESTRUCTURA PARALELA (EJERCICIO 3)\n";
    std::cout << "====================================================================\n\n";
    std::cout << "| Cantidad de Hilos ($p$) | Tiempo de Ejecucion ($T_p$) [s] | Speedup ($S_p$) | Eficiencia ($E_p$) |\n";
    std::cout << "| :---: | :---: | :---: | :---: |\n";
    for (const auto& res : tabla_resultados) {
        std::cout << "| **" << res.hilos << "** | " 
                  << std::fixed << std::setprecision(5) << res.tiempo << " s | " 
                  << res.speedup << "x | " 
                  << (res.eficiencia * 100.0) << "% |\n";
    }

    // Reporte 2: Evidencia funcional para verificar el procesamiento por bloques
    std::cout << "\n====================================================================\n";
    std::cout << "TABLA 2: MUESTRA DE RESULTADOS COMPROBATORIOS (TOP-3 VECINOS)\n";
    std::cout << "====================================================================\n\n";
    std::cout << "| Vector Origen | Vecino Cercano 1 (ID / Sim) | Vecino Cercano 2 (ID / Sim) | Vecino Cercano 3 (ID / Sim) |\n";
    std::cout << "| :---: | :---: | :---: | :---: |\n";
    
    // Mostrar una franja pequeña de vectores (ej. primeros 5) para no saturar la consola
    for (int i = 0; i < 10; ++i) {
        std::cout << "| **Vector " << i << "** | "
                  << "ID: " << top_k_final[i][0].id << " (" << std::fixed << std::setprecision(4) << top_k_final[i][0].similitud << ") | "
                  << "ID: " << top_k_final[i][1].id << " (" << top_k_final[i][1].similitud << ") | "
                  << "ID: " << top_k_final[i][2].id << " (" << top_k_final[i][2].similitud << ") |\n";
    }
    std::cout << "\n====================================================================\n";

    return 0;
}