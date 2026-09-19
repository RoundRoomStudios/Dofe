#ifndef IRON_FILETYPES_BASIC

#define IRON_FILETYPES_BASIC

static inline bool seekFile(FILE* file, long offset, int origin, const char* errorType, const char* locationName) {
    // Helper fseek with error checking
    int e = fseek(file, offset, origin);
    if (e != 0) {
        printf("%s | Failed to seek to %s.\n", errorType, locationName);
        return false;
    }
    return true;
}

static inline bool tellFile(FILE* file, long* buffer, const char* errorType, const char* locationName) {
    // Helper fseek with error checking
    *buffer = ftell(file);
    if (*buffer == -1L) {
        printf("%s | Failed to tell location of %s.\n", errorType, locationName);
        return false;
    }
    return true;
}

static inline bool readChar(uint8_t* buffer, size_t count, FILE* file, const char* errorType, const char* location) {
    // Helper fread with error checking
    size_t size = fread(buffer, sizeof(char), count, file);
    if (size != count) {
        printf("%s | File cuts off at %s.\n", errorType, location);
        return false;
    }
    return true;
}

static inline bool readU8(uint8_t* buffer, size_t count, FILE* file, const char* errorType, const char* location) {
    // Helper fread with error checking
    size_t size = fread(buffer, sizeof(uint8_t), count, file);
    if (size != count) {
        printf("%s | File cuts off at %s.\n", errorType, location);
        return false;
    }
    return true;
}

static inline bool readU16(uint16_t* buffer, size_t count, FILE* file, const char* errorType, const char* location, bool bswap) {
    // Helper fread with error checking
    size_t size = fread(buffer, sizeof(uint16_t), count, file);
    if (size != count) {
        printf("%s | File cuts off at %s.\n", errorType, location);
        return false;
    }
    if (bswap) {
        for (size_t i = 0; i < count; i++) {
            *(buffer+i) = __builtin_bswap16(*(buffer+i));
        }
    }
    return true;
}

static inline bool readU32(uint32_t* buffer, size_t count, FILE* file, const char* errorType, const char* location, bool bswap) {
    // Helper fread with error checking
    size_t size = fread(buffer, sizeof(uint32_t), count, file);
    if (size != count) {
        printf("%s | File cuts off at %s.\n", errorType, location);
        return false;
    }
    if (bswap) {
        for (size_t i = 0; i < count; i++) {
            *(buffer+i) = __builtin_bswap32(*(buffer+i));
        }
    }
    return true;
}

static inline bool read16(int16_t* buffer, size_t count, FILE* file, const char* errorType, const char* location, bool bswap) {
    // Helper fread with error checking
    size_t size = fread(buffer, sizeof(int16_t), count, file);
    if (size != count) {
        printf("%s | File cuts off at %s.\n", errorType, location);
        return false;
    }
    if (bswap) {
        for (size_t i = 0; i < count; i++) {
            *(buffer+i) = __builtin_bswap16(*(buffer+i));
        }
    }
    return true;
}

static inline bool readList() {return true;} // TODO make this function

#endif
