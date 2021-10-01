#include <stdint.h>
#include <stdlib.h>
#include <sol/types.h>

void sol_log_(const char *message, uint64_t len) {}
void sol_log_64(uint64_t arg1, uint64_t arg2, uint64_t arg3, uint64_t arg4, uint64_t arg5) {}
void sol_log_compute_units_() {}

void sol_panic_(const char *file, uint64_t len, uint64_t line, uint64_t column) {
    abort();
}

void sol_log_pubkey(const SolPubkey *pubkey) {
    
}
