#include <mpi.h>
#include <openssl/des.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <cstring>
#include <iomanip>
#include <algorithm>
#include <chrono>

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

bool decrypt_and_search(unsigned long long key, const std::vector<unsigned char>& ciphertext, const std::string& keyword) {
    std::vector<unsigned char> decrypted = ciphertext;
    process_des(key, decrypted, DES_DECRYPT);
    unpad_text(decrypted);
    std::string decrypted_str(decrypted.begin(), decrypted.end());
    if (decrypted_str.find(keyword) != std::string::npos) {
        std::cout << "Clave correcta encontrada: " << std::hex << key << std::dec << std::endl;
        return true;
    }
    return false;
}

int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);
    int world_size, world_rank;
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);
    MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);

    double start_time_total = MPI_Wtime();

    if (argc < 3) {
        if (world_rank == 0) {
            std::cerr << "Uso: " << argv[0] << " archivo.txt clave_inicial" << std::endl;
        }
        MPI_Finalize();
        return -1;
    }

    std::vector<unsigned char> text;
    unsigned long long initial_key = std::stoull(argv[2]);
    std::string keyword = "es una prueba de";

    if (world_rank == 0) {
        std::ifstream file(argv[1], std::ios::binary);
        if (!file) {
            std::cerr << "No se puede abrir el archivo." << std::endl;
            MPI_Abort(MPI_COMM_WORLD, 1);
            return -1;
        }
        text.assign((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
        file.close();
        pad_text(text);
    }

    int text_size = text.size();
    MPI_Bcast(&text_size, 1, MPI_INT, 0, MPI_COMM_WORLD);
    if (world_rank != 0) {
        text.resize(text_size);
    }
    MPI_Bcast(text.data(), text_size, MPI_UNSIGNED_CHAR, 0, MPI_COMM_WORLD);

    // Measure encryption time
    auto start_encryption = std::chrono::high_resolution_clock::now();
    process_des(initial_key, text, DES_ENCRYPT);
    auto end_encryption = std::chrono::high_resolution_clock::now();
    double encryption_time = std::chrono::duration<double>(end_encryption - start_encryption).count();

    // Print the encrypted text
    if (world_rank == 0) {
        std::cout << "Texto cifrado: ";
        print_hex(text);
        std::cout << "Tiempo de cifrado: " << encryption_time << " segundos" << std::endl;
    }

    // Search for the correct key
    unsigned long long key_range = 100000000ULL; // Adjust this range as needed
    unsigned long long keys_per_process = key_range / world_size;
    unsigned long long start_key = (world_rank * keys_per_process);
    unsigned long long end_key = ((world_rank + 1) * keys_per_process);

    // Asegurarse de que el último proceso cubra hasta la clave inicial más el rango
    if (world_rank == world_size - 1) {
        end_key = std::max(end_key, initial_key + key_range);
    }

    std::cout << "Proceso " << world_rank << " buscando desde " << std::hex << start_key << " hasta " << end_key << std::dec << std::endl;

    unsigned long long correct_key = 0;
    for (unsigned long long key = start_key; key < end_key; ++key) {
        if (decrypt_and_search(key, text, keyword)) {
            correct_key = key;
            break;
        }
    }

    unsigned long long global_correct_key;
    MPI_Allreduce(&correct_key, &global_correct_key, 1, MPI_UNSIGNED_LONG_LONG, MPI_MAX, MPI_COMM_WORLD);

    double end_time_total = MPI_Wtime();
    double total_time = end_time_total - start_time_total;

    if (world_rank == 0) {
        if (global_correct_key != 0) {
            std::cout << "Clave correcta encontrada: " << std::hex << global_correct_key << std::dec << std::endl;
            std::vector<unsigned char> decrypted = text;
            auto start_decryption = std::chrono::high_resolution_clock::now();
            process_des(global_correct_key, decrypted, DES_DECRYPT);
            auto end_decryption = std::chrono::high_resolution_clock::now();
            double decryption_time = std::chrono::duration<double>(end_decryption - start_decryption).count();
            unpad_text(decrypted);
            std::cout << "Texto descifrado: ";
            std::cout.write(reinterpret_cast<const char*>(decrypted.data()), decrypted.size()) << std::endl;
            std::cout << "Tiempo de descifrado: " << decryption_time << " segundos" << std::endl;
        } else {
            std::cout << "No se encontró la clave correcta en el rango especificado." << std::endl;
        }
        std::cout << "Tiempo total de ejecución: " << total_time << " segundos" << std::endl;
    }

    MPI_Finalize();
    return 0;
}
