import gzip
import random
import time
from datetime import datetime, timedelta

def generar_dataset_logs(nombre_archivo="logs_accesos.log.gz", num_lineas=5000000):
    print("Generando datos sintéticos de logs comprimidos...")
    endpoints = ["/api/v1/user", "/api/v1/login", "/api/v1/metrics", "/api/v1/payment", "/products"]
    codigos = [200, 200, 200, 404, 500, 201]
    
    fecha_base = datetime(2026, 5, 18, 0, 0, 0)
    
    with gzip.open(nombre_archivo, "wt", encoding="utf-8") as f:
        for i in range(num_lineas):
            user_id = f"user_{random.randint(1000, 9999)}"
            # Incrementar el tiempo secuencialmente para simular flujo real
            timestamp = (fecha_base + timedelta(seconds=i * 0.1)).strftime("%Y-%m-%d %H:%M:%S")
            endpoint = random.choice(endpoints)
            latencia = round(random.uniform(5.0, 500.0), 2)
            codigo = random.choice(codigos)
            
            # Formato CSV estándar: user_id,timestamp,endpoint,latencia,codigo
            f.write(f"{user_id},{timestamp},{endpoint},{latencia},{codigo}\n")
            
            if i % 1000000 == 0 and i > 0:
                print(f" En progreso: {i} líneas escritas...")

    print(f"Dataset generado exitosamente bajo el nombre: {nombre_archivo}\n")

if __name__ == "__main__":
    generar_dataset_logs()