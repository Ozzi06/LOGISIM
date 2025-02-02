#include "random_id.h"

#ifdef _WIN32
    // Windows-specific includes and library linking.
    #pragma comment(lib, "rpcrt4.lib")  // UuidCreate - Minimum supported OS Win 2000
    #include <Windows.h>

    my_uid_t generate_id() {
        UUID uuid;
        UuidCreate(&uuid);
        return uuid.Data1;
    }

#else
    // Linux (or other Unix-like OS) implementation using libuuid.
    #include <uuid/uuid.h>
    #include <string.h>  // for memcpy

    my_uid_t generate_id() {
        uuid_t uuid;
        uuid_generate(uuid);

        // Extract the first 4 bytes as a my_uid_t (assuming my_uid_t is 32 bits).
        my_uid_t id;
        memcpy(&id, uuid, sizeof(id));
        return id;
    }
#endif
