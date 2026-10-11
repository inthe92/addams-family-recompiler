#include "recomp/module_abi.h"
/* Synthetic loadable modules: no game entry point, code or resources. */
#ifdef TEST_ABI_VERSION
uint32_t nes_recomp_abi_version(void) { return TEST_ABI_VERSION; }
#else
uint32_t synthetic_module_marker(void) { return 0u; }
#endif
