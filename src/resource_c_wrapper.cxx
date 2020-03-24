//
//  resource_c_wrapper.c
//  shmht
//
//  Created by Wang Zhang on 3/23/20.
//  Copyright © 2020 Wang Zhang. All rights reserved.
//

#include "include/resource_c_wrapper.hpp"
#include "include/resource.hpp"

static RNM rnm;

extern "C" void RNM_init(const int pid, const int len_rnode, const int len_gmem) {
    rnm = RNM(pid, len_rnode, len_gmem);
}

extern "C" void print_rnodes(void) {
    rnm.print_rnodes();
}

extern "C" void print_gmem(void) {
    rnm.print_gmem();
}

extern "C" void add_gmem(unsigned long long dptr, const unsigned int bytes) {
    rnm.add_gmem(dptr, bytes);
}

extern "C" void set_process(const int pid) {
    rnm.set_process(pid);
}