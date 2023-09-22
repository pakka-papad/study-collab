#ifndef GLOBAL_UTILS
#define GLOBAL_UTILS

#include <iostream>
#include <string>
#include <openssl/bio.h>
#include <openssl/buffer.h>
#include <openssl/evp.h>

std::string ByteArrayToBase64(char* input, int length) {
    BIO *bio, *b64;
    BUF_MEM *bufferPtr;

    b64 = BIO_new(BIO_f_base64());
    bio = BIO_new(BIO_s_mem());
    bio = BIO_push(b64, bio);

    // Prevent line breaks in the output
    BIO_set_flags(bio, BIO_FLAGS_BASE64_NO_NL);

    BIO_write(bio, input, length);
    BIO_flush(bio);
    BIO_get_mem_ptr(bio, &bufferPtr);

    std::string base64Encoded(bufferPtr->data, bufferPtr->length);

    BIO_free_all(bio);

    return base64Encoded;
}

std::string Base64ToByteArray(const std::string &base64Encoded) {
    BIO *bio, *b64;
    int len = base64Encoded.size();
    char buffer[len*3/4+10]; // Adjust the buffer size as needed
    int length;
    
    b64 = BIO_new(BIO_f_base64());
    bio = BIO_new_mem_buf(base64Encoded.c_str(), -1);
    bio = BIO_push(b64, bio);

    // Prevent line breaks in the input
    BIO_set_flags(bio, BIO_FLAGS_BASE64_NO_NL);

    // Read the Base64 data and write to the buffer
    length = BIO_read(bio, buffer, sizeof(buffer));

    std::string byteArray(buffer, length);

    BIO_free_all(bio);

    return byteArray;
}

#endif