#include <openssl/des.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <unistd.h>

void decrypt(long key, char *ciph, int len) {
    DES_key_schedule ks;
    DES_cblock keyBlock;
    DES_string_to_key((const char *)&key, &keyBlock); // Convertir clave
    DES_set_key_unchecked(&keyBlock, &ks); // Establecer la clave
    DES_ecb_encrypt((DES_cblock *)ciph, (DES_cblock *)ciph, &ks, DES_DECRYPT);
}

void encrypt(long key, char *ciph, int len) {
    DES_key_schedule ks;
    DES_cblock keyBlock;
    DES_string_to_key((const char *)&key, &keyBlock); // Convertir clave
    DES_set_key_unchecked(&keyBlock, &ks); // Establecer la clave
    DES_ecb_encrypt((DES_cblock *)ciph, (DES_cblock *)ciph, &ks, DES_ENCRYPT);
}

// Nuevo mensaje en texto claro
unsigned char plaintext[] = "Secreto muy importante!";
// Texto cifrado (ajustar el tamaño del buffer si el mensaje es más largo)
unsigned char cipher[64];

int tryKey(long key, char *ciph, int len) {
    char temp[len + 1];
    memcpy(temp, ciph, len);
    temp[len] = 0;
    decrypt(key, temp, len);
    // Cambiar el texto de validación para que coincida con el nuevo mensaje
    return strcmp(temp, "Secreto muy importante!") == 0;
}

int main(int argc, char *argv[]) {
    int N, id;
    long upper = (1L << 16); // Limitar las claves posibles a 16 bits (2^16 = 65536 claves)
    long mylower, myupper;
    MPI_Status st;
    MPI_Request req;
    int flag;
    int ciphlen = strlen((char *)plaintext);  // longitud del nuevo texto en claro
    MPI_Comm comm = MPI_COMM_WORLD;
    MPI_Init(NULL, NULL);
    MPI_Comm_size(comm, &N);
    MPI_Comm_rank(comm, &id);

    // Medir el tiempo inicial
    double start_time = MPI_Wtime();

    // Usar una nueva clave para cifrar el mensaje
    long known_key = 6789;
    memcpy(cipher, plaintext, ciphlen); // copiar el nuevo mensaje al buffer de texto cifrado
    encrypt(known_key, (char *)cipher, ciphlen); // cifrar el nuevo mensaje usando la clave 6789

    if (id == 0) {
        printf("Texto original: %s\n", plaintext);
        printf("Texto cifrado: ");
        for (int i = 0; i < ciphlen; i++) {
            printf("%02x ", cipher[i]);
        }
        printf("\nBuscando la clave...\n");
    }

    int range_per_node = upper / N;
    mylower = range_per_node * id;
    myupper = range_per_node * (id + 1) - 1;
    if (id == N - 1) {
        // Compensar residuo
        myupper = upper;
    }

    // Mostrar el rango de claves que está buscando cada proceso
    printf("Proceso %d buscando claves en el rango [%ld, %ld]\n", id, mylower, myupper);

    long found = 0;
    MPI_Irecv(&found, 1, MPI_LONG, MPI_ANY_SOURCE, MPI_ANY_TAG, comm, &req);

    for (long i = mylower; i < myupper && (found == 0); ++i) {
        if (tryKey(i, (char *)cipher, ciphlen)) {
            found = i;
            for (int node = 0; node < N; node++) {
                MPI_Send(&found, 1, MPI_LONG, node, 0, MPI_COMM_WORLD);
            }
            break;
        }
    }

    if (id == 0) {
        MPI_Wait(&req, &st);
        decrypt(found, (char *)cipher, ciphlen);
        printf("Clave encontrada: %li\nTexto descifrado: %s\n", found, cipher);

        // Medir el tiempo final y mostrar el tiempo que tomó encontrar la clave
        double end_time = MPI_Wtime();
        printf("Tiempo para encontrar la clave: %f segundos\n", end_time - start_time);
    }

    MPI_Finalize();
}
