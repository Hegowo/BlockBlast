#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char *argv[]) {
    FILE *in, *out;
    unsigned char *buffer;
    long size;
    size_t i;
    const char *var_name;
    
    if (argc < 4) {
        fprintf(stderr, "Usage: %s <input_file> <output_file> <variable_name>\n", argv[0]);
        return 1;
    }
    
    in = fopen(argv[1], "rb");
    if (!in) {
        fprintf(stderr, "Cannot open input file: %s\n", argv[1]);
        return 1;
    }
    
    fseek(in, 0, SEEK_END);
    size = ftell(in);
    fseek(in, 0, SEEK_SET);
    
    buffer = (unsigned char *)malloc(size);
    if (!buffer) {
        fprintf(stderr, "Memory allocation failed\n");
        fclose(in);
        return 1;
    }
    
    if (fread(buffer, 1, size, in) != (size_t)size) {
        fprintf(stderr, "Failed to read file\n");
        free(buffer);
        fclose(in);
        return 1;
    }
    fclose(in);
    
    var_name = argv[3];
    
    out = fopen(argv[2], "w");
    if (!out) {
        fprintf(stderr, "Cannot open output file: %s\n", argv[2]);
        free(buffer);
        return 1;
    }
    
    fprintf(out, "const unsigned char %s_data[] = {\n    ", var_name);
    
    for (i = 0; i < (size_t)size; i++) {
        fprintf(out, "0x%02X", buffer[i]);
        if (i < (size_t)size - 1) {
            fprintf(out, ",");
        }
        if ((i + 1) % 16 == 0 && i < (size_t)size - 1) {
            fprintf(out, "\n    ");
        }
    }
    
    fprintf(out, "\n};\n");
    fprintf(out, "const size_t %s_size = %ld;\n", var_name, size);
    
    fclose(out);
    free(buffer);
    
    printf("Converted %s -> %s (%ld bytes)\n", argv[1], argv[2], size);
    return 0;
}

