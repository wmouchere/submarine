#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>

uint32_t fileSize(FILE* file) {
    fseek(file, 0L, SEEK_END);
    uint32_t sz = ftell(file);
    rewind(file);
    return sz;
}

uint8_t compare_byte(uint8_t data1, uint8_t data2) {
    uint8_t result = 0;
    for(int i = 7; i >= 0; --i) {
        result += ((data1 >> i) ^ (data2 >> i)) & 0xFE;
    }
    return result;
}

uint32_t compare(uint8_t* data1, uint8_t* data2, uint32_t len) {
    uint32_t result = 0;
    for(int i = 0; i < len; ++i) {
        result += compare_byte(data1[i], data2[i]);
    }
    return result;
}

int main(){

    FILE* file = NULL; 
    file = fopen("img/submarine_small.png","r");
    if(file == (FILE*) NULL) {
        fprintf(stderr, "Impossible d'ouvrir le fichier\n");
        exit(2);
    }
    
    uint32_t len = fileSize(file);
    uint8_t* data = (uint8_t*) malloc(sizeof(uint8_t)*len);
    fread(data, sizeof(uint8_t)*len, 1, file);

    FILE* output = NULL; 
    output = fopen("out/submarine_small_output.png","r");
    if(output == (FILE*) NULL) {
        fprintf(stderr, "Impossible d'ouvrir le fichier\n");
        exit(2);
    }
    uint32_t output_len = fileSize(output);
    uint8_t* output_data = (uint8_t*) malloc(sizeof(uint8_t)*output_len);
    fread(output_data, sizeof(uint8_t)*output_len, 1, output);

    printf("Difference between the two files : %d bits\n", compare(data, output_data, len));

    free(data);
    free(output_data);
    fclose(output);
    fclose(file);
    return 0;
}