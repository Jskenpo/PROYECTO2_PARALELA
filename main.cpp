#include <mpi.h>
#include <openssl/des.h>
#include <iostream>
#include <fstream>
#include <cstring>
#include <iomanip>
#include <chrono>

// Funciones de cifrado y descifrado (las mismas de antes)

void encrypt(long key, unsigned char* plain_text, size_t plain_text_len, unsigned char* result) {
    DES_key_schedule schedule;
    DES_cblock key_block;

    for (int i = 0; i < 8; ++i) {
        key_block[i] = (key >> (i * 8)) & 0xFF;
    }

    DES_set_odd_parity(&key_block);
    DES_set_key_checked(&key_block, &schedule);

    for (size_t i = 0; i < plain_text_len; i += 8) {
        DES_ecb_encrypt((DES_cblock*)&plain_text[i], (DES_cblock*)&result[i], &schedule, DES_ENCRYPT);
    }
}

void decrypt(long key, unsigned char* cipher_text, size_t cipher_text_len, unsigned char* result) {
    DES_key_schedule schedule;
    DES_cblock key_block;

    for (int i = 0; i < 8; ++i) {
        key_block[i] = (key >> (i * 8)) & 0xFF;
    }

    DES_set_odd_parity(&key_block);
    DES_set_key_checked(&key_block, &schedule);

    for (size_t i = 0; i < cipher_text_len; i += 8) {
        DES_ecb_encrypt((DES_cblock*)&cipher_text[i], (DES_cblock*)&result[i], &schedule, DES_DECRYPT);
    }
}

void print_hex(const unsigned char* text, int len) {
    for (int i = 0; i < len; ++i) {
        std::cout << std::hex << std::setfill('0') << std::setw(2) << (int)text[i];
    }
    std::cout << std::endl;
}

int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);

    int world_size;
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);

    int world_rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);

    // Cargar texto desde archivo
    if (argc < 3) {
        if (world_rank == 0) {
            std::cerr << "Uso: " << argv[0] << " archivo.txt clave" << std::endl;
        }
        MPI_Finalize();
        return -1;
    }

    std::ifstream file(argv[1]);
    if (!file.is_open()) {
        if (world_rank == 0) {
            std::cerr << "No se puede abrir el archivo." << std::endl;
        }
        MPI_Finalize();
        return -1;
    }

    std::string text((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    file.close();

    long key = std::stol(argv[2]);
    size_t text_len = text.length();
    size_t padded_length = (text_len % 8 == 0) ? text_len : ((text_len / 8) + 1) * 8;

    unsigned char padded_text[padded_length];
    std::memcpy(padded_text, text.c_str(), text_len);

    unsigned char encrypted_text[padded_length];
    unsigned char decrypted_text[padded_length];

    // División del trabajo entre procesos MPI
    size_t chunk_size = padded_length / world_size;
    size_t start = world_rank * chunk_size;
    size_t end = (world_rank == world_size - 1) ? padded_length : (world_rank + 1) * chunk_size;

    // Cada proceso cifra su parte del texto
    encrypt(key, padded_text + start, end - start, encrypted_text + start);

    // Crear buffer de recepción en el proceso 0
    unsigned char* gathered_text = nullptr;
    if (world_rank == 0) {
        gathered_text = new unsigned char[padded_length];
    }

    // Recoger y juntar los resultados en el proceso 0
    MPI_Gather(encrypted_text + start, chunk_size, MPI_UNSIGNED_CHAR,
               gathered_text, chunk_size, MPI_UNSIGNED_CHAR, 0, MPI_COMM_WORLD);

    if (world_rank == 0) {
        std::cout << "Texto cifrado: ";
        print_hex(gathered_text, padded_length);

        // Luego descifrar y mostrar el texto original
        decrypt(key, gathered_text, padded_length, decrypted_text);
        std::cout << "Texto descifrado: " << decrypted_text << std::endl;

        delete[] gathered_text; // Liberar memoria del buffer
    }

    MPI_Finalize();
    return 0;
}
