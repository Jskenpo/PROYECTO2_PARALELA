#include <openssl/des.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <cstring>
#include <iomanip>
#include <chrono>
#include <cmath>

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

void process_des(unsigned long long key, std::vector<unsigned char>& text, int mode) {
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

// Función para realizar cálculos intensivos
double intensive_calculation(int iterations) {
    double result = 0.0;
    for (int i = 0; i < iterations; ++i) {
        result += std::sin(i) * std::cos(i);
    }
    return result;
}

bool decrypt_and_search(unsigned long long key, const std::vector<unsigned char>& ciphertext, const std::string& keyword) {
    std::vector<unsigned char> decrypted = ciphertext;
    process_des(key, decrypted, DES_DECRYPT);
    unpad_text(decrypted);
    std::string decrypted_str(decrypted.begin(), decrypted.end());

    intensive_calculation(100000);

    if (decrypted_str.find(keyword) != std::string::npos) {
        std::cout << "Clave correcta encontrada: " << std::hex << key << std::dec << std::endl;
        return true;
    }
    return false;
}

int main(int argc, char** argv) {
    if (argc < 3) {
        std::cerr << "Uso: " << argv[0] << " archivo.txt clave_inicial" << std::endl;
        return -1;
    }

    std::vector<unsigned char> text;
    unsigned long long initial_key = std::stoull(argv[2]);
    std::string keyword = "es una prueba de";

    // Leer el archivo
    std::ifstream file(argv[1], std::ios::binary);
    if (!file) {
        std::cerr << "No se puede abrir el archivo." << std::endl;
        return -1;
    }
    text.assign((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    file.close();
    pad_text(text);

    // Medir tiempo de cifrado
    auto start_encryption = std::chrono::high_resolution_clock::now();
    process_des(initial_key, text, DES_ENCRYPT);
    auto end_encryption = std::chrono::high_resolution_clock::now();
    double encryption_time = std::chrono::duration<double>(end_encryption - start_encryption).count();

    std::cout << "Texto cifrado: ";
    print_hex(text);
    std::cout << "Tiempo de cifrado: " << encryption_time << " segundos" << std::endl;

    // Búsqueda de la clave correcta
    unsigned long long key_range = 10000000000ULL;
    auto start_time_total = std::chrono::high_resolution_clock::now();

    unsigned long long correct_key = 0;
    for (unsigned long long key = 0; key < key_range; ++key) {
        if (decrypt_and_search(key, text, keyword)) {
            correct_key = key;
            break;
        }

        // Imprimir progreso cada 100,000 claves probadas
        if (key % 100000 == 0) {
            std::cout << "Probando clave: " << key << std::endl;
        }
    }

    auto end_time_total = std::chrono::high_resolution_clock::now();
    double total_time = std::chrono::duration<double>(end_time_total - start_time_total).count();

    if (correct_key != 0) {
        std::cout << "Clave correcta encontrada: " << std::hex << correct_key << std::dec << std::endl;
        std::vector<unsigned char> decrypted = text;
        auto start_decryption = std::chrono::high_resolution_clock::now();
        process_des(correct_key, decrypted, DES_DECRYPT);
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

    return 0;
}
