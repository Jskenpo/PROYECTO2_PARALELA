#include <openssl/des.h>
#include <mpi.h>
#include <cstring>
#include <iostream>
#include <unistd.h>

void decrypt(long key, unsigned char *cipher, int len) {
    DES_key_schedule ks;
    DES_cblock keyBlock;
    // Convert the long key to DES_cblock (64 bits)
    for (int i = 0; i < 8; ++i) {
        keyBlock[i] = (key >> (i * 8)) & 0xFF;
    }
    DES_set_odd_parity(&keyBlock); // Set key parity
    DES_set_key(&keyBlock, &ks);   // Set key schedule

    // Decrypt in ECB mode
    DES_ecb_encrypt((DES_cblock*)cipher, (DES_cblock*)cipher, &ks, DES_DECRYPT);
}

void encrypt(long key, unsigned char *cipher, int len) {
    DES_key_schedule ks;
    DES_cblock keyBlock;
    // Convert the long key to DES_cblock (64 bits)
    for (int i = 0; i < 8; ++i) {
        keyBlock[i] = (key >> (i * 8)) & 0xFF;
    }
    DES_set_odd_parity(&keyBlock); // Set key parity
    DES_set_key(&keyBlock, &ks);   // Set key schedule

    // Encrypt in ECB mode
    DES_ecb_encrypt((DES_cblock*)cipher, (DES_cblock*)cipher, &ks, DES_ENCRYPT);
}

char search[] = " the ";
int tryKey(long key, unsigned char *cipher, int len) {
    unsigned char temp[len + 1];
    memcpy(temp, cipher, len);
    temp[len] = '\0';  // Null-terminate for strstr usage
    decrypt(key, temp, len);
    return strstr((char*)temp, search) != NULL;
}

unsigned char cipher[] = {108, 245, 65, 63, 125, 200, 150, 66, 17, 170, 207, 170, 34, 31, 70, 215, 0};

int main(int argc, char *argv[]) {
    int N, id;
    long upper = (1L << 56);  // Upper bound DES keys 2^56
    long mylower, myupper;
    MPI_Status st;
    int ciphlen = strlen((char*)cipher);
    MPI_Comm comm = MPI_COMM_WORLD;

    MPI_Init(&argc, &argv);
    MPI_Comm_size(comm, &N);
    MPI_Comm_rank(comm, &id);

    long range_per_node = upper / N;
    mylower = range_per_node * id;
    myupper = range_per_node * (id + 1) - 1;

    if (id == N - 1) {
        // Compensate for any leftover
        myupper = upper;
    }

    long found = 0;
    long global_found = 0;

    // Print start message for each process
    std::cout << "Process " << id << " started. Range: [" << mylower << ", " << myupper << "]" << std::endl;

    for (long i = mylower; i < myupper && global_found == 0; ++i) {
        // Reduced the frequency of print statements
        if (i % 10000000 == 0) {
            std::cout << "Process " << id << " checking key: " << i << std::endl;
        }

        if (tryKey(i, cipher, ciphlen)) {
            found = i;
            std::cout << "Process " << id << " found the key: " << found << std::endl;
            // Notify all other processes about the found key
            for (int node = 0; node < N; ++node) {
                MPI_Send(&found, 1, MPI_LONG, node, 0, MPI_COMM_WORLD);
            }
            break;
        }

        // Check if any other process has already found the key
        int flag;
        MPI_Iprobe(MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &flag, &st);
        if (flag) {
            MPI_Recv(&global_found, 1, MPI_LONG, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &st);
            std::cout << "Process " << id << " received key from another process: " << global_found << std::endl;
            break;
        }
    }

    // Make sure all processes synchronize
    MPI_Bcast(&found, 1, MPI_LONG, 0, MPI_COMM_WORLD);

    if (found != 0 && id == 0) {
        decrypt(found, cipher, ciphlen);
        std::cout << "Key: " << found << " Decrypted message: " << cipher << std::endl;
    } else if (id == 0) {
        std::cout << "No key found." << std::endl;
    }

    MPI_Finalize();
    return 0;
}
