#include <stdlib.h>
#include <include/cuda-subset.h>

#ifndef resource_c_wrapper_hpp
#define resource_c_wrapper_hpp

#ifdef __cplusplus                     
extern "C" {
#endif

    void RNM_init(const int, const int, const int);
    void print_rnodes(void);
    void print_gmem(void);
    void set_process(const int);

    void add_gmem(CUdeviceptr*, const size_t);
    void add_gmem_cuarr(CUarray*, const size_t);
    void add_gmem_cumarr(CUmipmappedArray*, const size_t);

    void free_gmem(CUdeviceptr);
    void free_gmem_cuarr(CUarray);
    void free_gmem_cumarr(CUmipmappedArray);

    size_t get_shared_GPU_mem_limit(void); 
    size_t get_gmem_used(void);

#ifdef __cplusplus
}
#endif

#endif /* resource_c_wrapper_hpp */