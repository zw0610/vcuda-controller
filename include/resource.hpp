#pragma once

#include <sys/ipc.h>
#include <sys/shm.h>
#include <stdlib.h>
#include <iostream>
#include <tuple>
#include <assert.h>

#include "include/gmem.hpp"
#include "include/rnode.hpp"

class RNM
{
    int pid;
    unsigned long stime;

    key_t key_rnode;
    int shm_id_rnode;
    RNode* ptr_rnode;
    int len_rnode;
    std::size_t hash_d;

    key_t key_gmem;
    int shm_id_gmem;
    GMem* ptr_gmem;
    int len_gmem;

private:
    bool init_rnode(const std::size_t);

    bool init_gmem(const std::size_t);

    void init(void);

    const std::size_t pre_hash(const pid_st &) const;

    const std::size_t hash(const pid_st &, const int ) const;
    
    const int find_gmem(const RNode*, const CUdeviceptr)const ;

    const int push_gmem(const GMem&);
    
    const int last_rec(const int) const;
    
    void link_rnode_gmem(RNode*, const int);
        
    void remove_gmem_by_dptr(const int, const int, CUdeviceptr);
    
public:
    RNM();

    RNM(const int, const int, const int);

    RNM(const RNM &);

    ~RNM();

    void set_process(const int);
    
    RNode *insert_rnode(const pid_st &, const RNode &);
    
    RNode *find_rnode(const pid_st &) const;
    
    void delete_rnode(const pid_st &);

    bool contain(const pid_st &) const;

    void print_rnodes(void) const;
    
    void print_gmem(void) const;

    void add_gmem(CUdeviceptr, const std::size_t);
    
    void remove_gmem(CUdeviceptr);

    std::size_t gmem_used(void); 
};
