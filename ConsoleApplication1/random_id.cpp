#include "random_id.h"
#pragma comment(lib, "rpcrt4.lib")  // UuidCreate - Minimum supported OS Win 2000
#include <Windows.h>


uid_t generate_id() {
    UUID uuid;
    UuidCreate(&uuid);

    return uuid.Data1;
}