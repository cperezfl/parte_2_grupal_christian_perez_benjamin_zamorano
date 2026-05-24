import gzip
import time
import csv
from datetime import datetime
from collections import defaultdict
from concurrent.futures import ProcessPoolExecutor

# --- ETAPA 2: PARSEO Y LIMPIEZA (CPU-bound) ---
def procesar_chunk_worker(lineas_crudas):
    agregacion_local = defaultdict(lambda: {"total_latencia": 0.0, "conteo": 0})
    
    for linea in lineas_crudas:
        linea = linea.strip()
        if not linea:
            continue
        
        try:
            user_id, timestamp_str, endpoint, latencia_str, codigo_str = linea.split(",")
            
            # Extracción y redondeo de la ventana temporal a bloques de 15 minutos 
            fecha_dt = datetime.strptime(timestamp_str, "%Y-%m-%d %H:%M:%S")
            minuto_ventana = (fecha_dt.minute // 15) * 15
            ventana_id = fecha_dt.replace(minute=minuto_ventana, second=0).strftime("%Y-%m-%d %H:%M")
            
            latencia = float(latencia_str)
            llave = (ventana_id, user_id, endpoint)
            
            agregacion_local[llave]["total_latencia"] += latencia
            agregacion_local[llave]["conteo"] += 1
            
        except (ValueError, IndexError):
            continue
            
    return dict(agregacion_local)


# --- ETAPA 1 Y 3: LECTURA (I/O-bound) Y REDUCCIÓN ---
def ejecutar_pipeline(ruta_archivo, max_workers, chunk_size=100000):
    diccionario_global = defaultdict(lambda: {"total_latencia": 0.0, "conteo": 0})
    tiempo_inicio = time.time()
    
    with ProcessPoolExecutor(max_workers=max_workers) as executor:
        futuros = []
        
        try:
            with gzip.open(ruta_archivo, "rt", encoding="utf-8") as f:
                chunk_actual = []
                for linea in f:
                    chunk_actual.append(linea)
                    if len(chunk_actual) >= chunk_size:
                        futuros.append(executor.submit(procesar_chunk_worker, chunk_actual))
                        chunk_actual = []
                
                if chunk_actual:
                    futuros.append(executor.submit(procesar_chunk_worker, chunk_actual))
                    
        except EOFError:
            # Captura el error de fin de flujo de Gzip y procesa lo que se alcanzó a rescatar
            print(f"\n[Aviso Técnico] Se detectó un fin de archivo abrupto (EOF) en {max_workers} worker(s).")
            print("Procesando los bloques recuperados de forma segura para el benchmark...")
            if chunk_actual:
                futuros.append(executor.submit(procesar_chunk_worker, chunk_actual))
        
        # --- ETAPA 3: AGREGACIÓN GLOBAL ---
        for futuro in futuros:
            resumen_worker = futuro.result()
            for llave, metricas in resumen_worker.items():
                diccionario_global[llave]["total_latencia"] += metricas["total_latencia"]
                diccionario_global[llave]["conteo"] += metricas["conteo"]

    tiempo_fin = time.time()
    duracion = tiempo_fin - tiempo_inicio
    return duracion, diccionario_global


# --- EXPORTACIÓN DE RESULTADOS ---
def guardar_resultados_csv(diccionario_datos, ruta_salida="metricas_ventanas_15min.csv"):
    print(f">> Exportando resultados funcionales a {ruta_salida}... ", end="", flush=True)
    
    with open(ruta_salida, mode="w", newline="", encoding="utf-8") as f:
        escritor = csv.writer(f)
        # Cabeceras claras según los requerimientos del ejercicio 
        escritor.writerow(["ventana_temporal", "user_id", "endpoint", "latencia_promedio_ms", "total_peticiones"])
        
        for (ventana, user_id, endpoint), datos in diccionario_datos.items():
            promedio_latencia = datos["total_latencia"] / datos["conteo"]
            escritor.writerow([ventana, user_id, endpoint, round(promedio_latencia, 4), datos["conteo"]])
            
    print("Completado con éxito.")


# --- BLOQUE DE BENCHMARKING Y EJECUCIÓN INTEGRADA ---
if __name__ == "__main__":
    ruta_logs = "logs_accesos.log.gz"
    configuraciones_workers = [1, 2, 4, 8]
    tiempos_registrados = {}
    ultimo_diccionario = None
    
    print("====================================================================")
    print("INICIANDO EVALUACIÓN EXPERIMENTAL Y GENERACIÓN DE RESULTADOS")
    print("====================================================================")
    
    for workers in configuraciones_workers:
        print(f">> Procesando pipeline con {workers} worker(s)... ", end="", flush=True)
        
        tiempo_total, datos_consolidados = ejecutar_pipeline(ruta_logs, max_workers=workers)
        tiempos_registrados[workers] = tiempo_total
        ultimo_diccionario = datos_consolidados  # Conservar los datos de la última corrida
        
        print(f"Completado en {tiempo_total:.4f} segundos.")
    
    # Una vez terminado el benchmark, guardamos el producto de datos final en disco
    guardar_resultados_csv(ultimo_diccionario)
    
    # Generación automática de la tabla en formato Markdown
    print("\n====================================================================")
    print("TABLA REQUERIDA PARA EL INFORME GRUPAL (FORMATO MARKDOWN)")
    print("====================================================================")
    print("| Configuración de Workers ($p$) | Tiempo de Ejecución ($T_p$) | Speedup ($S_p$) | Eficiencia ($E_p$) |")
    print("| :---: | :---: | :---: | :---: |")
    
    t1 = tiempos_registrados[1]
    for workers in configuraciones_workers:
        tp = tiempos_registrados[workers]
        speedup = t1 / tp
        eficiencia = speedup / workers
        print(f"| **{workers} Worker(s)** | {tp:.5f} s | {speedup:.5f}x | {eficiencia * 100.0:.2f}% |")
        
    print("====================================================================")