# Proyecto: Cifrado y Descifrado con DES

Este proyecto implementa varios enfoques para cifrar y descifrar texto utilizando el algoritmo DES (Data Encryption Standard). El objetivo es comparar diferentes métodos de ejecución (fuerza bruta, secuencial y paralelo) para entender el rendimiento y las mejoras en eficiencia al paralelizar el proceso.

### Rama `Paralelo`: Cifrado Paralelo

En la rama `Paralelo`, se presentan tres enfoques diferentes para paralelizar el cifrado y descifrado:

- **`main.cpp` (Naive)**: Implementa un enfoque paralelo básico.
- **`DLB.cpp` (Dynamic Load Balancing)**: Utiliza balanceo dinámico de carga para distribuir el trabajo entre los procesos.
- **`OpenMP.cpp` (Enfoque Híbrido MPI+OpenMP)**: Mezcla MPI y OpenMP para optimizar el procesamiento paralelo.

#### Compilación y Ejecución

- **Enfoque Naive (`main.cpp`)**

```bash
mpic++ -o main main.cpp -lssl -lcrypto
mpiexec -np 4 ./main texto.txt 123456
```

- **Dynamic Load Balancing (`DLB.cpp`)**

```bash
mpic++ -o DLB DLB.cpp -lssl -lcrypto
mpiexec -np 4 ./DLB texto.txt 123456
```

- **Enfoque Híbrido MPI+OpenMP (`OpenMP.cpp`)**

```bash
mpic++ -o OpenMP OpenMP.cpp -lssl -lcrypto
mpiexec -np 4 ./OpenMP texto.txt 123456
```

## Requisitos

- OpenSSL para las bibliotecas de cifrado y descifrado (`-lssl -lcrypto`).
- MPI (Message Passing Interface) para los enfoques paralelos.
- OpenMP para los enfoques paralelos
