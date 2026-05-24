# INF8090: Computación Paralela y Distribuida — Prueba 1
**Universidad Tecnológica Metropolitana (UTEM)**
**Ingeniería Civil en Ciencia de Datos**

* **Profesor:** Dr. Ing. Michael Miranda Sandoval
* **Integrantes:** Christian Pérez, Benjamín Zamorano
* **Sección:** 412
* **Fecha de entrega:** 28 de mayo, 2026

---

## Descripción del Proyecto
Este repositorio contiene el desarrollo e implementación de la **Parte II (Grupal)** de la Prueba 1. El objetivo central es optimizar el uso de la infraestructura multi-núcleo mediante distintas estrategias concurrentes y de alto rendimiento (HPC) para mitigar cuellos de botella espaciales y temporales en Ciencia de Datos:

1. **Ejercicio 1 (C++/OpenMP):** Normalización masiva de matrices ($50.000.000 \times 16$) con un esquema de privatización absoluta en buffers locales para erradicar el *false sharing*.
2. **Ejercicio 2 (Python):** Pipeline distribuido de logs masivos (2 GB) estructurado en un flujo continuo por bloques (*streaming chunks*) para evadir las restricciones del GIL mediante `ProcessPoolExecutor`.
3. **Ejercicio 3 (C++/OpenMP):** Cálculo exacto *in-place* del Top-10 de vecinos cercanos para 20.000 embeddings de dimensión 128 usando colas de prioridad (`std::priority_queue`) privadas y planificación dinámica triangular.

---

## 💻 Registro de Infraestructura (Hardware de Pruebas)
En cumplimiento con el protocolo de registro obligatorio de la cátedra, los benchmarks automatizados integrados en los scripts se ejecutaron bajo el siguiente entorno controlado:

* **Sistema Operativo:** Windows 11 Home
* **Procesador (CPU):** AMD Ryzen 5 5500 (6 Núcleos Físicos, 12 Hilos Lógicos)
* **Memoria RAM:** 16 GB
* **Versión de Python:** Python 3.12.x
* **Compilador C++:** GCC `g++` v13.2.0 (Soporte nativo para OpenMP)

---

## Instrucciones de Ejecución

### Ejercicios en C++ (Códigos 1 y 3)
El directorio cuenta con un archivo `Makefile` para automatizar la compilación con las banderas de optimización exigidas (`-O3 -fopenmp`).

1. Abre la terminal en la carpeta `codigo_openmp/`.
2. Compila ambos programas ejecutando:
   `make`
3. Ejecuta el benchmark de Normalización Masiva (Ejercicio 1):
   `./normalizacion`
4. Ejecuta el benchmark de Similitud de Embeddings (Ejercicio 3):
   `./ejercicio3`
5. Para limpiar los ejecutables y archivos binarios intermedios:
   `make clean`

*Nota: Si tu entorno de Windows no reconoce el comando `make`, puedes compilar de forma manual directa ejecutando `g++ -O3 -fopenmp normalizacion.cpp -o normalizacion` y `g++ -O3 -fopenmp ejercicio3.cpp -o ejercicio3`.*

### Ejercicio en Python (Código 2)
El flujo por bloques dinámicos opera exclusivamente sobre módulos nativos del lenguaje.
1. Abre la terminal en la carpeta `ejercicio_2_python/`.
2. (Opcional) Si necesitas reconstruir o generar el set de datos de logs comprimidos, ejecuta:
   `python generar_logs.py`
3. Ejecuta el pipeline completo y el generador automático de estadísticas en formato Markdown:
   `python pipeline_logs.py`

---

## Declaración de Integridad Académica y Uso de IA
En estricto cumplimiento con el artículo 10 de las bases de la evaluación ("*Advertencia de integridad académica y uso de herramientas de IA*"), el grupo de trabajo declara lo siguiente:

* Se utilizó la herramienta de Inteligencia Artificial **Gemini** de forma explícita como soporte técnico auxiliar en la **creación de la estructura base del código fuente**, el **apoyo en la redacción formal y estilizada del informe escrito** y la entrega de **recomendaciones técnicas** para abordar las restricciones lógicas de cada ejercicio.
* **Validación Humana:** Toda sugerencia, bloque de código o directiva arquitectónica provista por la IA no fue copiada de manera acrítica. El equipo de trabajo evaluó de forma independiente cada recomendación, quedando bajo nuestro estricto criterio la verificación, el análisis de veracidad, la correcta integración con el hardware local y el desarrollo lógico de los pipelines en C++ y Python. 
* Los análisis conceptuales respecto a la Pared de la Memoria (*Memory Wall*), el comportamiento no asociativo de los flotantes, la evasión del GIL y la justificación de los *trade-offs* empíricos de la Ley de Amdahl responden a un proceso de asimilación teórica y autoría estrictamente humana por parte de los integrantes.
