#include <iostream>
#include <vector>
#include <cmath>
#include <chrono>
#include <omp.h>
#include <random>
#include <iomanip>

const long long FILAS = 50000000;
const int COLUMNAS = 16;

struct ResultadoBenchmark {
    int hilos;
    double tiempo;
    double speedup;
    double eficiencia;
};

void generar_datos_sinteticos(std::vector<double>& X) {
    std::mt19937 generator(42); 
    std::uniform_real_distribution<double> distribution(10.0, 100.0);
    std::uniform_real_distribution<double> nan_prob(0.0, 1.0);

    for (long long i = 0; i < FILAS * COLUMNAS; ++i) {
        if (nan_prob(generator) < 0.05) { 
            X[i] = NAN;
        } else {
            X[i] = distribution(generator);
        }
    }
}

double ejecutar_algoritmo(int num_hilos, const std::vector<double>& X, std::vector<double>& Z,
                          std::vector<double>& media, std::vector<double>& varianza, 
                          std::vector<long long>& validos, std::vector<long long>& outliers) {
    
    omp_set_num_threads(num_hilos);

    // Reiniciar los contenedores globales en cada ejecucion del benchmark
    std::fill(media.begin(), media.end(), 0.0);
    std::fill(varianza.begin(), varianza.end(), 0.0);
    std::fill(validos.begin(), validos.end(), 0);
    std::fill(outliers.begin(), outliers.end(), 0);

    auto inicio = std::chrono::high_resolution_clock::now();

    #pragma omp parallel
    {
        std::vector<double> suma_local(COLUMNAS, 0.0);
        std::vector<long long> validos_local(COLUMNAS, 0);

        // Fase 1: Sumas parciales y conteo de validos
        #pragma omp for schedule(static)
        for (long long i = 0; i < FILAS; ++i) {
            for (int j = 0; j < COLUMNAS; ++j) {
                double val = X[i * COLUMNAS + j];
                if (!std::isnan(val)) {
                    suma_local[j] += val;
                    validos_local[j]++;
                }
            }
        }

        #pragma omp critical
        {
            for (int j = 0; j < COLUMNAS; ++j) {
                media[j] += suma_local[j];
                validos[j] += validos_local[j];
            }
        }

        #pragma omp barrier 

        #pragma omp single
        {
            for (int j = 0; j < COLUMNAS; ++j) {
                if (validos[j] > 0) media[j] /= validos[j];
            }
        }

        // Fase 2: Diferencias cuadraticas para Varianza
        std::vector<double> var_local(COLUMNAS, 0.0);

        #pragma omp for schedule(static)
        for (long long i = 0; i < FILAS; ++i) {
            for (int j = 0; j < COLUMNAS; ++j) {
                double val = X[i * COLUMNAS + j];
                if (!std::isnan(val)) {
                    double diff = val - media[j];
                    var_local[j] += diff * diff;
                }
            }
        }

        #pragma omp critical
        {
            for (int j = 0; j < COLUMNAS; ++j) varianza[j] += var_local[j];
        }

        #pragma omp barrier

        #pragma omp single
        {
            for (int j = 0; j < COLUMNAS; ++j) {
                if (validos[j] > 1) varianza[j] /= (validos[j] - 1);
            }
        }

        // Fase 3: Estandarizacion Z y conteo de outliers por columna
        std::vector<long long> outliers_local(COLUMNAS, 0);

        #pragma omp for schedule(static)
        for (long long i = 0; i < FILAS; ++i) {
            for (int j = 0; j < COLUMNAS; ++j) {
                double val = X[i * COLUMNAS + j];
                if (!std::isnan(val)) {
                    double desviacion = std::sqrt(varianza[j]);
                    double z_score = (desviacion > 0) ? (val - media[j]) / desviacion : 0.0;
                    Z[i * COLUMNAS + j] = z_score;

                    if (std::abs(z_score) > 3.0) {
                        outliers_local[j]++;
                    }
                } else {
                    Z[i * COLUMNAS + j] = NAN;
                }
            }
        }

        #pragma omp critical
        {
            for (int j = 0; j < COLUMNAS; ++j) {
                outliers[j] += outliers_local[j];
            }
        }
    }

    auto fin = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> tiempo = fin - inicio;
    return tiempo.count();
}

int main() {
    std::cout << "Asignando memoria base (12.8 GB)..." << std::endl;
    std::vector<double> X(FILAS * COLUMNAS);
    std::vector<double> Z(FILAS * COLUMNAS);

    std::cout << "Generando datos sinteticos..." << std::endl;
    generar_datos_sinteticos(X);

    // Estructuras globales para almacenar los datos finales calculados
    std::vector<double> media_final(COLUMNAS, 0.0);
    std::vector<double> varianza_final(COLUMNAS, 0.0);
    std::vector<long long> validos_final(COLUMNAS, 0);
    std::vector<long long> outliers_final(COLUMNAS, 0);

    std::vector<int> configuracion_hilos = {1, 2, 4, 8};
    std::vector<ResultadoBenchmark> tabla_resultados;
    double tiempo_secuencial = 0.0;

    std::cout << "\nIniciando Ciclo de Pruebas Automatizado (Benchmark)..." << std::endl;

    for (int hilos : configuracion_hilos) {
        std::cout << ">> Ejecutando con " << hilos << " hilo(s)... " << std::flush;
        
        double t_ejecucion = ejecutar_algoritmo(hilos, X, Z, media_final, varianza_final, validos_final, outliers_final);
        std::cout << "Completado en " << t_ejecucion << " segundos." << std::endl;

        if (hilos == 1) {
            tiempo_secuencial = t_ejecucion;
        }

        double speedup = tiempo_secuencial / t_ejecucion;
        double eficiencia = speedup / hilos;

        tabla_resultados.push_back({hilos, t_ejecucion, speedup, eficiencia});
    }

    // Reporte 1: Tabla de Rendimiento (Métricas de infraestructura)
    std::cout << "\n====================================================================\n";
    std::cout << "TABLA 1: RENDIMIENTO Y ESCALABILIDAD DEL ALGORITMO\n";
    std::cout << "====================================================================\n\n";
    std::cout << "| Cantidad de Hilos ($p$) | Tiempo de Ejecucion ($T_p$) [s] | Speedup ($S_p$) | Eficiencia ($E_p$) |\n";
    std::cout << "| :---: | :---: | :---: | :---: |\n";
    for (const auto& res : tabla_resultados) {
        std::cout << "| **" << res.hilos << "** | " 
                  << std::fixed << std::setprecision(5) << res.tiempo << " s | " 
                  << res.speedup << "x | " 
                  << (res.eficiencia * 100.0) << "% |\n";
    }
    
    // Reporte 2: Tabla de Resultados Funcionales (Lo que exige el enunciado por columna)
    std::cout << "\n====================================================================\n";
    std::cout << "TABLA 2: RESULTADOS ESTADÍSTICOS OBTENIDOS POR COLUMNA\n";
    std::cout << "====================================================================\n\n";
    std::cout << "| Columna | Valores Válidos | Media (Ignorando NaN) | Varianza (Muestral) | Atípicos ($|z| > 3$) |\n";
    std::cout << "| :---: | :---: | :---: | :---: | :---: |\n";
    for (int j = 0; j < COLUMNAS; ++j) {
        std::cout << "| **Columna " << j + 1 << "** | " 
                  << validos_final[j] << " | " 
                  << std::fixed << std::setprecision(4) << media_final[j] << " | " 
                  << varianza_final[j] << " | " 
                  << outliers_final[j] << " |\n";
    }
    std::cout << "\n====================================================================\n";

    return 0;
}