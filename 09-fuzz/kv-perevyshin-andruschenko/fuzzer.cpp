#include "kv-perevyshin.hpp"

static size_t num_valid = 0;

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
    if (data == nullptr) {
        return 0;
    }

    bool valid = is_input_valid(data, size);
    num_valid += valid;
    //std::cerr << "Valid inputs processed: " << num_valid << "\n";

    char temp_file_name[64];
    char kv_temp_file_name[64];
    char* tmpnam_res = nullptr;

    while (tmpnam_res == nullptr) {
        tmpnam_res = tmpnam(temp_file_name);
    }

    memcpy(kv_temp_file_name, temp_file_name, 64);

    size_t cur_end = strlen(temp_file_name);
    memcpy(temp_file_name + cur_end, ".bin", 5);
    memcpy(kv_temp_file_name + cur_end, ".kv", 4);

    FILE* bin_file;
    if ((bin_file = fopen(temp_file_name, "w")) == nullptr) {
        __builtin_trap();
    } 
    
    fwrite(data, sizeof(char), size, bin_file);
    fclose(bin_file);

    char* argv[2];
    argv[1] = temp_file_name;
    int errcode = execute_main(2, argv);

    if (errcode == 0 && !valid) {
        //std::cerr << "Bin file invalid, did not fail\n";
        __builtin_trap();
    }

    if (errcode != 0 && valid) {
        //std::cerr << "Bin file valid, did not failed\n";
        __builtin_trap();
    }

    if (!valid) {
        std::remove(temp_file_name);
        std::remove(kv_temp_file_name);
        return 0;
    }

    std::remove(temp_file_name);

    argv[1] = kv_temp_file_name;
    errcode = execute_main(2, argv);

    if (errcode != 0) {
        //std::cerr << "Failed on reverse transform\n";
        __builtin_trap();
    }

    FILE* restored_file = fopen(temp_file_name, "rb");
    if (restored_file == nullptr) {
        //std::cerr << "No restored file\n";
        __builtin_trap();
    }

    uint8_t restored_buffer[size];
    size_t num_restored_read = 0;
    if ((num_restored_read = fread(restored_buffer, sizeof(char), size, restored_file)) < size) {
        //std::cerr << "Restored bin file smaller than original\n";
        fclose(restored_file);
        __builtin_trap();
    }

    uint8_t extra_byte;
    if (fread(&extra_byte, sizeof(char), 1, restored_file)) {
        //std::cerr << "Restored bin file larger than original\n";
        fclose(restored_file);
        __builtin_trap();
    }

    for (size_t i = 0; i < size; ++i) {
        if (data[i] != restored_buffer[i]) {
            //std::cerr << "Round-trip guarantee not satisfied\n";
            fclose(restored_file);
            __builtin_trap();
        }
    }

    fclose(restored_file);
    std::remove(temp_file_name);
    std::remove(kv_temp_file_name);

    return 0;
}
