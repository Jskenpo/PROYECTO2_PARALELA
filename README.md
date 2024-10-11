# Proyecto: Cifrado y Descifrado con DES

Este proyecto implementa varios enfoques para cifrar y descifrar texto utilizando el algoritmo DES (Data Encryption Standard). El objetivo es comparar diferentes métodos de ejecución (fuerza bruta, secuencial y paralelo) para entender el rendimiento y las mejoras en eficiencia al paralelizar el proceso.

### Rama `Secuencial`: Cifrado Secuencial

En la rama `Secuencial`, el archivo `main.cpp` contiene el código para cifrar y descifrar texto de manera secuencial usando una única clave. Este enfoque es sencillo y no optimiza el tiempo de ejecución.

#### Compilación y Ejecución

```bash
g++ -o secuencial main.cpp -lssl -lcrypto
./secuencial texto.txt 123456
```

## Requisitos

- OpenSSL para las bibliotecas de cifrado y descifrado (`-lssl -lcrypto`).
