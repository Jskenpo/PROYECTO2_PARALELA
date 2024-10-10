#include <openssl/des.h>
#include <iostream>
#include <iomanip>
#include <cstring>

void apply_padding(unsigned char* input, size_t length, size_t padded_length) {
    // Aplicar relleno PKCS#7
    unsigned char pad_value = padded_length - length;
    for (size_t i = length; i < padded_length; ++i) {
        input[i] = pad_value;
    }
}

void remove_padding(unsigned char* input, size_t& length) {
    unsigned char pad_value = input[length - 1];
    length -= pad_value;
    input[length] = '\0'; // Asegurar el terminador nulo
}

void encrypt(long key, unsigned char* plain_text, size_t plain_text_len, unsigned char* result) {
    DES_key_schedule schedule;
    DES_cblock key_block;

    // Convertir la clave larga a una clave de 8 bytes para DES
    for (int i = 0; i < 8; ++i) {
        key_block[i] = (key >> (i * 8)) & 0xFF;
    }

    // Establecer la paridad
    DES_set_odd_parity(&key_block);

    // Establecer la clave de cifrado
    DES_set_key_checked(&key_block, &schedule);

    // Cifrar en bloques de 8 bytes (DES es un cifrado de bloque de 64 bits)
    for (size_t i = 0; i < plain_text_len; i += 8) {
        DES_ecb_encrypt((DES_cblock*)&plain_text[i], (DES_cblock*)&result[i], &schedule, DES_ENCRYPT);
    }
}

void decrypt(long key, unsigned char* cipher_text, size_t cipher_text_len, unsigned char* result) {
    DES_key_schedule schedule;
    DES_cblock key_block;

    // Convertir la clave larga a una clave de 8 bytes para DES
    for (int i = 0; i < 8; ++i) {
        key_block[i] = (key >> (i * 8)) & 0xFF;
    }

    // Establecer la paridad
    DES_set_odd_parity(&key_block);

    // Establecer la clave de descifrado
    DES_set_key_checked(&key_block, &schedule);

    // Descifrar en bloques de 8 bytes
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

int main() {
    unsigned char plain_text[] = "Hola a todos"; // Ejemplo de texto a cifrar
    size_t plain_text_len = strlen((char*)plain_text);

    // Longitud con relleno (múltiplo de 8)
    size_t padded_length = (plain_text_len % 8 == 0) ? plain_text_len : ((plain_text_len / 8) + 1) * 8;

    unsigned char padded_plain_text[padded_length];
    unsigned char cipher_text[padded_length];
    unsigned char decrypted_text[padded_length];

    // Copiar texto plano y aplicar relleno
    std::memcpy(padded_plain_text, plain_text, plain_text_len);
    apply_padding(padded_plain_text, plain_text_len, padded_length);

    long key = 123456L;

    // Cifrar el texto
    encrypt(key, padded_plain_text, padded_length, cipher_text);
    std::cout << "Texto cifrado: ";
    print_hex(cipher_text, padded_length);

    // Descifrar el texto
    decrypt(key, cipher_text, padded_length, decrypted_text);

    // Remover relleno
    remove_padding(decrypted_text, padded_length);
    std::cout << "Texto descifrado: " << decrypted_text << std::endl;

    return 0;
}
