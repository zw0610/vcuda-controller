cmake_minimum_required(VERSION 3.5)
project(hijack CXX C)

add_definitions(-D_GNU_SOURCE)
add_compile_options(-Wall -Wshadow -Werror)

include_directories(${CMAKE_SOURCE_DIR})

if (${ENABLE_DEBUG})
    add_compile_options(-g -O0)
else ()
    add_compile_options(-g -O2)
endif ()

set(STATIC_C_LIBRARIES -static-libgcc -static-libstdc++)

add_library(cuda-control SHARED
            src/hijack_call.c
            include/hijack.h
            include/resource_c_wrapper.hpp
            include/resource.hpp
            include/rnode.hpp
            include/gmem.hpp
            include/cuda-subset.h
            include/cuda-helper.h
            src/cuda_originals.c
            src/loader.c
            src/resource_c_wrapper.cxx
            src/resource.cxx)

target_link_libraries(cuda-control ${STATIC_C_LIBRARIES})            

target_compile_options(cuda-control PUBLIC $<$<COMPILE_LANGUAGE:CXX>:-std=c++11>)

