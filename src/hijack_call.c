/*
 * Tencent is pleased to support the open source community by making TKEStack available.
 *
 * Copyright (C) 2012-2019 Tencent. All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not use
 * this file except in compliance with the License. You may obtain a copy of the
 * License at
 *
 * https://opensource.org/licenses/Apache-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OF ANY KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations under the License.
 */

#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <unistd.h>

#include "include/cuda-helper.h"
#include "include/hijack.h"
#include "include/resource_c_wrapper.hpp"

extern entry_t cuda_library_entry[];
extern char pid_path[];

typedef void (*atomic_fn_ptr)(int, void *);


/** hijack entrypoint */
CUresult cuDriverGetVersion(int *driverVersion) {
    printf("%s\n", "hijacked cuDriverGetVersion is called!");
    CUresult ret;

    load_necessary_data();
 
//   pthread_once(&g_init_set, initialization);

    ret = CUDA_ENTRY_CALL(cuda_library_entry, cuDriverGetVersion, driverVersion);

    return ret;
}

CUresult cuInit(unsigned int flag) {
    CUresult ret;

    load_necessary_data();



    printf("%s\n", "hijacked cuInit is called!");
//   pthread_once(&g_init_set, initialization);
    ret = CUDA_ENTRY_CALL(cuda_library_entry, cuInit, flag);

    return ret;
}

CUresult cuMemAllocManaged(CUdeviceptr *dptr, size_t bytesize,
                           unsigned int flags) {
    printf("%s\n", "hijacked cuMemAllocManaged is called!");

    const size_t used = get_gmem_used();
    const size_t request_size = bytesize;
    CUresult ret;

    if ((used + request_size) > get_shared_GPU_mem_limit())
    {
        return CUDA_ERROR_OUT_OF_MEMORY;
    }
    
    ret = CUDA_ENTRY_CALL(cuda_library_entry, cuMemAllocManaged, dptr, bytesize,
                        flags);

    if (ret == CUDA_SUCCESS)
    {
        add_gmem(dptr, bytesize);
    }
    return ret;
}

CUresult cuMemAlloc_v2(CUdeviceptr *dptr, size_t bytesize) {
    printf("%s\n", "hijacked cuMemAlloc_v2 is called!");

    const size_t used = get_gmem_used();
    const size_t request_size = bytesize;
    CUresult ret;

    if ((used + request_size) > get_shared_GPU_mem_limit())
    {
        return CUDA_ERROR_OUT_OF_MEMORY;
    }
    
    ret = CUDA_ENTRY_CALL(cuda_library_entry, cuMemAlloc_v2, dptr, bytesize);

    if (ret == CUDA_SUCCESS)
    {
        add_gmem(dptr, bytesize);
    }
    
    // print_rnodes();
    // print_gmem();
    // printf("now, used GPU memory = %lu\n", get_gmem_used());
    return ret;
}

CUresult cuMemAlloc(CUdeviceptr *dptr, size_t bytesize) {
    printf("%s\n", "hijacked cuMemAlloc is called!");

    const size_t used = get_gmem_used();
    const size_t request_size = bytesize;
    CUresult ret;

    if ((used + request_size) > get_shared_GPU_mem_limit())
    {
        return CUDA_ERROR_OUT_OF_MEMORY;
    }

    ret = CUDA_ENTRY_CALL(cuda_library_entry, cuMemAlloc, dptr, bytesize);

    if (ret == CUDA_SUCCESS)
    {
        add_gmem(dptr, bytesize);
    }
    return ret;
}

CUresult cuMemAllocPitch_v2(CUdeviceptr *dptr, size_t *pPitch,
                            size_t WidthInBytes, size_t Height,
                            unsigned int ElementSizeBytes) {
    printf("%s\n", "hijacked cuMemAllocPitch_v2 is called!");

    const size_t used = get_gmem_used();
    const size_t request_size = ROUND_UP(WidthInBytes * Height, ElementSizeBytes);
    CUresult ret;

    if ((used + request_size) > get_shared_GPU_mem_limit())
    {
        return CUDA_ERROR_OUT_OF_MEMORY;
    }
    
    ret = CUDA_ENTRY_CALL(cuda_library_entry, cuMemAllocPitch_v2, dptr, pPitch,
                        WidthInBytes, Height, ElementSizeBytes);

    if (ret == CUDA_SUCCESS)
    {
        add_gmem(dptr, request_size);
    }

    return ret;
}

CUresult cuMemAllocPitch(CUdeviceptr *dptr, size_t *pPitch, size_t WidthInBytes,
                         size_t Height, unsigned int ElementSizeBytes) {
    printf("%s\n", "hijacked cuMemAllocPitch is called!");
    const size_t used = 0;
    const size_t request_size = ROUND_UP(WidthInBytes * Height, ElementSizeBytes);
    CUresult ret;

    if ((used + request_size) > get_shared_GPU_mem_limit())
    {
        return CUDA_ERROR_OUT_OF_MEMORY;
    }

    ret = CUDA_ENTRY_CALL(cuda_library_entry, cuMemAllocPitch, dptr, pPitch,
                        WidthInBytes, Height, ElementSizeBytes);

    if (ret == CUDA_SUCCESS)
    {
        add_gmem(dptr, request_size);
    }

    return ret;
}

static size_t get_array_base_size(int format) {
    size_t base_size = 0;

    switch (format) {
    case CU_AD_FORMAT_UNSIGNED_INT8:
    case CU_AD_FORMAT_SIGNED_INT8:
        base_size = 8;
        break;
    case CU_AD_FORMAT_UNSIGNED_INT16:
    case CU_AD_FORMAT_SIGNED_INT16:
    case CU_AD_FORMAT_HALF:
        base_size = 16;
        break;
    case CU_AD_FORMAT_UNSIGNED_INT32:
    case CU_AD_FORMAT_SIGNED_INT32:
    case CU_AD_FORMAT_FLOAT:
        base_size = 32;
        break;
    default:
        base_size = 32;
    }

    return base_size;
}

static size_t
cuArrayCreate_helper(const CUDA_ARRAY_DESCRIPTOR *pAllocateArray) {
    size_t base_size = 0;

    base_size = get_array_base_size(pAllocateArray->Format);
    return base_size * pAllocateArray->NumChannels *
                   pAllocateArray->Height * pAllocateArray->Width;
}

CUresult cuArrayCreate_v2(CUarray *pHandle,
                          const CUDA_ARRAY_DESCRIPTOR *pAllocateArray) {
    printf("%s\n", "hijacked cuArrayCreate_v2 is called!");
    CUresult ret;

    const size_t request_size = cuArrayCreate_helper(pAllocateArray);
    const size_t used = get_gmem_used();

    if ((used + request_size) > get_shared_GPU_mem_limit()) {
        return CUDA_ERROR_OUT_OF_MEMORY;
    }
    
    ret = CUDA_ENTRY_CALL(cuda_library_entry, cuArrayCreate_v2, pHandle,
                        pAllocateArray);

    if (ret == CUDA_SUCCESS)
    {
        add_gmem_cuarr(pHandle, request_size);
    }

    return ret;
}

CUresult cuArrayCreate(CUarray *pHandle,
                       const CUDA_ARRAY_DESCRIPTOR *pAllocateArray) {
    printf("%s\n", "hijacked cuArrayCreate is called!");
    CUresult ret;

    const size_t request_size = cuArrayCreate_helper(pAllocateArray);
    const size_t used = get_gmem_used();

    if ((used + request_size) > get_shared_GPU_mem_limit()) {
        return CUDA_ERROR_OUT_OF_MEMORY;
    }
    
    ret = CUDA_ENTRY_CALL(cuda_library_entry, cuArrayCreate, pHandle,
                        pAllocateArray);

    if (ret == CUDA_SUCCESS)
    {
        add_gmem_cuarr(pHandle, request_size);
    }

    return ret;
}

static size_t
cuArray3DCreate_helper(const CUDA_ARRAY3D_DESCRIPTOR *pAllocateArray) {
    size_t base_size = 0;

    base_size = get_array_base_size(pAllocateArray->Format);
    return base_size * pAllocateArray->NumChannels * pAllocateArray->Height *
           pAllocateArray->Width;
}

CUresult cuArray3DCreate_v2(CUarray *pHandle,
                            const CUDA_ARRAY3D_DESCRIPTOR *pAllocateArray) {
    printf("%s\n", "hijacked cuArray3DCreate_v2 is called!");
    CUresult ret;

    const size_t request_size = cuArray3DCreate_helper(pAllocateArray);
    const size_t used = get_gmem_used();

    if ((used + request_size) > get_shared_GPU_mem_limit()) {
        return CUDA_ERROR_OUT_OF_MEMORY;
    }
    
    ret = CUDA_ENTRY_CALL(cuda_library_entry, cuArray3DCreate_v2, pHandle,
                        pAllocateArray);

    if (ret == CUDA_SUCCESS)
    {
        add_gmem_cuarr(pHandle, request_size);
    }

    return ret;
}

CUresult cuArray3DCreate(CUarray *pHandle,
                         const CUDA_ARRAY3D_DESCRIPTOR *pAllocateArray) {
    printf("%s\n", "hijacked cuArray3DCreate is called!");
    CUresult ret;

    const size_t request_size = cuArray3DCreate_helper(pAllocateArray);
    const size_t used = get_gmem_used();

    if ((used + request_size) > get_shared_GPU_mem_limit()) {
        return CUDA_ERROR_OUT_OF_MEMORY;
    }

    ret = CUDA_ENTRY_CALL(cuda_library_entry, cuArray3DCreate, pHandle,
                        pAllocateArray);

    if (ret == CUDA_SUCCESS)
    {
        add_gmem_cuarr(pHandle, request_size);
    }

    return ret;
}

CUresult cuMipmappedArrayCreate(
    CUmipmappedArray *pHandle,
    const CUDA_ARRAY3D_DESCRIPTOR *pMipmappedArrayDesc,
    unsigned int numMipmapLevels) {

    printf("%s\n", "hijacked cuMipmappedArrayCreate is called!");
    CUresult ret;

    const size_t base_size = get_array_base_size(pMipmappedArrayDesc->Format);

    const size_t request_size = base_size * pMipmappedArrayDesc->NumChannels *
                                pMipmappedArrayDesc->Height *
                                pMipmappedArrayDesc->Width *
                                pMipmappedArrayDesc->Depth;

    const size_t used = get_gmem_used();
    if ((used + request_size) > get_shared_GPU_mem_limit())
    {
        return CUDA_ERROR_OUT_OF_MEMORY;
    }

    ret = CUDA_ENTRY_CALL(cuda_library_entry, cuMipmappedArrayCreate, pHandle,
                        pMipmappedArrayDesc, numMipmapLevels);

    if (ret == CUDA_SUCCESS)
    {
        add_gmem_cumarr(pHandle, request_size);
    }    

    return ret;
}

CUresult cuDeviceTotalMem_v2(size_t *bytes, CUdevice dev) {
    printf("%s\n", "hijacked cuDeviceTotalMem_v2 is called!");
    CUresult res = CUDA_ENTRY_CALL(cuda_library_entry, cuDeviceTotalMem_v2, bytes, dev);
    if (res != CUDA_SUCCESS)
    {
        return res;
    }
    size_t limited = get_shared_GPU_mem_limit();
    size_t final_gpu_mem = limited < *bytes ? limited : *bytes;
    printf("final gpu limit %lu \n", final_gpu_mem);

    *bytes = final_gpu_mem;
    return CUDA_SUCCESS;
}

CUresult cuDeviceTotalMem(size_t *bytes, CUdevice dev) {
    printf("%s\n", "hijacked cuDeviceTotalMem is called!");
    CUresult res = CUDA_ENTRY_CALL(cuda_library_entry, cuDeviceTotalMem, bytes, dev);
    if (res != CUDA_SUCCESS)
    {
        return res;
    }
    size_t limited = get_shared_GPU_mem_limit();
    size_t final_gpu_mem = limited < *bytes ? limited : *bytes;
    *bytes = final_gpu_mem;
    return CUDA_SUCCESS;
}

CUresult cuMemGetInfo_v2(size_t *free, size_t *total) {
    printf("%s\n", "hijacked cuMemGetInfo_v2 is called!");
    const size_t used = get_gmem_used();

   // we use a simplified, non-verifying approach to get total memory
    *total = get_shared_GPU_mem_limit();

    *free = used > *total ? 0 : *total - used;

    return CUDA_SUCCESS;
}

CUresult cuMemGetInfo(size_t *free, size_t *total) {
    printf("%s\n", "hijacked cuMemGetInfo is called!");
    const size_t used = get_gmem_used();

   // we use a simplified, non-verifying approach to get total memory
    *total = get_shared_GPU_mem_limit();

    *free = used > *total ? 0 : *total - used;

    return CUDA_SUCCESS;
}

CUresult cuMemFree_v2(CUdeviceptr dptr) {
    printf("%s\n", "hijacked cuMemFree_v2 is called!");
    free_gmem(dptr);
    return CUDA_ENTRY_CALL(cuda_library_entry, cuMemFree_v2, dptr);
}

CUresult cuMemFree(CUdeviceptr dptr) {
    printf("%s\n", "hijacked cuMemFree is called!");
    free_gmem(dptr);
    return CUDA_ENTRY_CALL(cuda_library_entry, cuMemFree, dptr);
}

CUresult cuArrayDestroy(CUarray hArray) {
  return CUDA_ENTRY_CALL(cuda_library_entry, cuArrayDestroy, hArray);
}

CUresult cuMipmappedArrayDestroy(CUmipmappedArray hMipmappedArray) {
  return CUDA_ENTRY_CALL(cuda_library_entry, cuMipmappedArrayDestroy,
                         hMipmappedArray);
}