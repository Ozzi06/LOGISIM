#include "random_id.h"
#include <random>
#include <limits>

my_uid_t generate_id() {
    // std::random_device requests entropy from the OS kernel (e.g., /dev/urandom)
    // This ensures different seeds on every restart.
    static std::random_device rd; 

    // Mersenne Twister 64-bit is extremely high quality and fast.
    static std::mt19937_64 gen(rd()); 

    // Use the full 64-bit range.
    static std::uniform_int_distribution<my_uid_t> dis(1, std::numeric_limits<my_uid_t>::max());

    return dis(gen);
}
