#include <mpi.h>
#include <openssl/des.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <cstring>
#include <iomanip>
#include <algorithm>

void pad_text(std::vector<unsigned char>& text) {
    size_t padding_size = 8 - (text.size() % 8);
    text.insert(text.end(), padding_size, static_cast<unsigned char>(padding_size));
}

void unpad_text(std::vector<unsigned char>& text) {
    if (!text.empty()) {
        unsigned char last_byte = text.back();
        if (last_byte <= 8 && last_byte > 0) {
            text.resize(text.size() - last_byte);
        }
    }
}

void process_des(long key, std::vector<unsigned char>& text, int mode) {
    DES_key_schedule schedule;
    DES_cblock key_block;
    for (int i = 0; i < 8; ++i) {
        key_block[i] = (key >> (i * 8)) & 0xFF;
    }
    DES_set_odd_parity(&key_block);
    DES_set_key_checked(&key_block, &schedule);

    for (size_t i = 0; i < text.size(); i += 8) {
        DES_ecb_encrypt((DES_cblock*)&text[i], (DES_cblock*)&text[i], &schedule, mode);
    }
}

void print_hex(const std::vector<unsigned char>& text) {
    for (unsigned char c : text) {
        std::cout << std::hex << std::setfill('0') << std::setw(2) << static_cast<int>(c);
    }
    std::cout << std::endl;
}

int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);
    int world_size, world_rank;
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);
    MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);

    if (argc < 3) {
        if (world_rank == 0) {
            std::cerr << "Uso: " << argv[0] << " archivo.txt clave" << std::endl;
        }
        MPI_Finalize();
        return -1;
    }

    std::vector<unsigned char> text;
    long key = 0;

    if (world_rank == 0) {
        std::ifstream file(argv[1], std::ios::binary);
        if (!file) {
            std::cerr << "No se puede abrir el archivo." << std::endl;
            MPI_Abort(MPI_COMM_WORLD, 1);
            return -1;
        }
        text.assign((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
        file.close();
        key = std::stol(argv[2]);
        pad_text(text);
    }

    // Broadcast the key and text size
    MPI_Bcast(&key, 1, MPI_LONG, 0, MPI_COMM_WORLD);
    int text_size = text.size();
    MPI_Bcast(&text_size, 1, MPI_INT, 0, MPI_COMM_WORLD);

    // Resize text vector on non-root processes
    if (world_rank != 0) {
        text.resize(text_size);
    }

    // Broadcast the text content
    MPI_Bcast(text.data(), text_size, MPI_UNSIGNED_CHAR, 0, MPI_COMM_WORLD);

    // Divide work among processes
    size_t chunk_size = text_size / world_size;
    size_t start = world_rank * chunk_size;
    size_t end = (world_rank == world_size - 1) ? text_size : start + chunk_size;

    // Encrypt
    double start_time = MPI_Wtime(); // Start time for encryption
    process_des(key, text, DES_ENCRYPT);
    double end_time = MPI_Wtime();   // End time for encryption
    double encryption_time = end_time - start_time;

    // Gather encrypted chunks
    std::vector<unsigned char> encrypted_text;
    if (world_rank == 0) {
        encrypted_text.resize(text_size);
    }

    MPI_Gather(text.data() + start, chunk_size, MPI_UNSIGNED_CHAR,
               encrypted_text.data(), chunk_size, MPI_UNSIGNED_CHAR, 0, MPI_COMM_WORLD);

    if (world_rank == 0) {
        std::cout << "Texto cifrado: ";
        print_hex(encrypted_text);
        std::cout << "Tiempo de cifrado: " << encryption_time << " segundos" << std::endl;

        // Decrypt
        start_time = MPI_Wtime(); // Start time for decryption
        process_des(key, encrypted_text, DES_DECRYPT);
        unpad_text(encrypted_text);
        end_time = MPI_Wtime();   // End time for decryption
        double decryption_time = end_time - start_time;

        std::cout << "Texto descifrado: ";
        std::cout.write(reinterpret_cast<const char*>(encrypted_text.data()), encrypted_text.size()) << std::endl;
        std::cout << "Tiempo de descifrado: " << decryption_time << " segundos" << std::endl;
    }

    MPI_Finalize();
    return 0;
}
