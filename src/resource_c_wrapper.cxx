//
//  resource_c_wrapper.c
//  shmht
//
//  Created by Wang Zhang on 3/23/20.
//  Copyright © 2020 Wang Zhang. All rights reserved.
//

#include "include/resource_c_wrapper.hpp"
#include "include/resource.hpp"

#include <iostream>
#include <stdlib.h>

static RNM rnm;

extern "C" void RNM_init(const int pid, const int len_rnode,
                         const int len_gmem) {
    rnm = RNM(pid, len_rnode, len_gmem);
}

extern "C" void print_rnodes(void) { rnm.print_rnodes(); }

extern "C" void print_gmem(void) { rnm.print_gmem(); }

extern "C" void add_gmem(CUdeviceptr* dptr, const size_t bytes) {
    void* temp_ptr = static_cast<void*>(dptr);
    rnm.add_gmem<CUdeviceptr>(temp_ptr, bytes);
}

extern "C" void free_gmem(unsigned long long dptr) {
    rnm.remove_gmem(dptr);
}

extern "C" void set_process(const int pid) { rnm.set_process(pid); }

extern "C" size_t get_shared_GPU_mem_limit(void) {
    const char *GML_SPTR = secure_getenv("GPU_MEM_LIMIT");
    size_t gml = 0;
    if (GML_SPTR != NULL) {
        gml = strtoul(GML_SPTR, NULL, 10);
    }
    return gml;
}

extern "C" size_t get_gmem_used(void) { return rnm.gmem_used(); }
