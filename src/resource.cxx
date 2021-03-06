#include "include/resource.hpp"

#include <assert.h>
#include <errno.h>

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

#include "include/cuda-subset.h"

std::ostream &operator<<(std::ostream &os, const RNode &rn) {
    os << "PID: " << rn.pid << " time: " << rn.t
       << " res_entry_idx: " << rn.res_entry_idx;
    return os;
}

std::ostream &operator<<(std::ostream &os, const GMem &gm) {
    os << "dptr (" << gm.ptr << ") with size of " << gm.bytes
       << " bytes and pointing to " << gm.next;
    return os;
}

/*----------------------------------------------------------------------*/

RNM::RNM() {
    set_process(-1);
    len_rnode = 101;
    len_gmem = 1024;
    init();
}

RNM::RNM(const RNM &rnm) {
    set_process(rnm.pid);

    // RNode related copy constructor
    key_rnode = rnm.key_rnode;
    shm_id_rnode = rnm.shm_id_rnode;
    ptr_rnode = rnm.ptr_rnode;
    len_rnode = rnm.len_rnode;
    hash_d = rnm.hash_d;

    // GMem related copy constructor
    key_gmem = rnm.key_gmem;
    shm_id_gmem = rnm.shm_id_gmem;
    ptr_gmem = rnm.ptr_gmem;
    len_gmem = rnm.len_gmem;
}

RNM::RNM(const int npid, const int clen_rnode, const int clen_gmem) {
    set_process(npid);
    this->len_rnode = clen_rnode;
    this->len_gmem = clen_gmem;
    init();
}

RNM::~RNM() {
    shmdt(ptr_rnode);
    shmdt(ptr_gmem);
}

void RNM::init(void) {
    // init RNode array in shared memory
    key_rnode = 0x7C00;
    hash_d = len_rnode;
    std::size_t bytes_rnode = len_rnode * sizeof(RNode);

    bool if_rnode_created = init_rnode(bytes_rnode);
    ptr_rnode = (RNode *)shmat(shm_id_rnode, NULL, 0);
    if (!if_rnode_created) {
        for (int i = 0; i < len_rnode; i++) {
            ptr_rnode[i].initialize();
        }
    }

    // init Resource array in shared memory
    key_gmem = 0x7E00;
    std::size_t bytes_gmem = len_gmem * sizeof(GMem);

    bool if_gmem_created = init_gmem(bytes_gmem);
    ptr_gmem = (GMem *)shmat(shm_id_gmem, NULL, 0);
    if (!if_gmem_created) {
        for (int i = 0; i < len_gmem; i++) {
            ptr_gmem[i].initialize();
        }
    }
}

bool RNM::init_rnode(const std::size_t bytes) {
    shm_id_rnode = shmget(key_rnode, bytes, 0666 | IPC_CREAT | IPC_EXCL);
    if (shm_id_rnode == -1) {
        // created, return true
        shm_id_rnode = shmget(key_rnode, bytes, 0666 | IPC_CREAT);
        return true;
    }
    return false;
}

bool RNM::init_gmem(const std::size_t bytes) {
    shm_id_gmem = shmget(key_gmem, bytes, 0666 | IPC_CREAT | IPC_EXCL);
    if (shm_id_gmem == -1) {
        shm_id_gmem = shmget(key_gmem, bytes, 0666 | IPC_CREAT);
        return true;
    }
    return false;
}

void RNM::set_process(const int npid) {
    pid = npid;
    // read stime from /proc/{pid}/stat, the 22nd element
    if (pid <= 0) {
        stime = 0;
        return;
    }

    const std::string filename = "stat";
    const std::string proc_dir = "/proc/" + std::to_string(pid) + "/";

    std::ifstream stat_file{proc_dir + filename};
    std::string stime_placeholder;

    if (stat_file.is_open()) {
        std::stringstream str_stream;
        str_stream << stat_file.rdbuf();

        for (int i = 0; i < 22; i++) {
            str_stream >> stime_placeholder;
        }
        stime = atol(stime_placeholder.c_str());
    } else {
        std::cout << "cannot open " << proc_dir << filename << std::endl;
    }
}

const std::size_t RNM::pre_hash(const pid_st &key) const {
    // temporary implemantation
    std::size_t seed = std::get<0>(key);
    seed = seed << 6;
    seed += std::size_t(std::get<1>(key));
    return seed;
}

const std::size_t RNM::hash(const pid_st &key, const int i) const {
    const std::size_t seed = pre_hash(key);
    return (seed + i) % hash_d;
}

RNode *RNM::insert_rnode(const pid_st &key, const RNode &val) {
    for (int i = 0; i < len_rnode; i++) {
        std::size_t pos = hash(key, i);
        if (ptr_rnode[pos].get_stat() == RNodeStatus::NONEXIST ||
            ptr_rnode[pos].get_stat() == RNodeStatus::DELETED) {
            ptr_rnode[pos] = val;
            return &ptr_rnode[pos];
        }
        // rna[pos].get_stat() == RNodeStatus::EXIST
        if (ptr_rnode[pos].same_key(key)) {
            ptr_rnode[pos] = val;
            return &ptr_rnode[pos];
        }
    }
    // if nullptr is returned, it means the rnode hash table is full
    return nullptr;
}

RNode *RNM::find_rnode(const pid_st &key) const {
    for (int i = 0; i < len_rnode; i++) {
        std::size_t pos = hash(key, i);
        if (ptr_rnode[pos].get_stat() == RNodeStatus::NONEXIST) {
            // not found
            return nullptr;
        } else if (ptr_rnode[pos].get_stat() == RNodeStatus::EXIST) {
            if (ptr_rnode[pos].same_key(key)) {
                return ptr_rnode + pos;
            }
            // if the rnode exist, but not match, check the next
        }
        // if the stat is DELETED, check the next
    }
    return nullptr;
}

void RNM::delete_rnode(const pid_st &key) {
    RNode *del_ptr = find_rnode(key);
    if (del_ptr != nullptr) {
        del_ptr->shallow_delete();
    }
}

bool RNM::contain(const pid_st &key) const {
    return find_rnode(key) != nullptr;
}

void RNM::print_rnodes(void) const {
    std::cout << "Listing items from Resource Node Map from Process " << pid
              << " Starting since " << stime << " :" << std::endl;
    std::size_t count = 0;
    for (int i = 0; i < len_rnode; i++) {
        RNode &rna_ref = ptr_rnode[i];
        if (rna_ref.get_stat() == RNodeStatus::EXIST) {
            rna_ref.print();
            count++;
        }
    }
    if (count == 0) {
        std::cout << "None" << std::endl;
    }
}

void RNM::print_gmem(void) const {
    std::cout << "Listing items from GPU Memory from Process " << pid
              << " Starting since " << stime << " :" << std::endl;
    std::size_t count = 0;
    for (int i = 0; i < len_gmem; i++) {
        GMem &gm_ref = ptr_gmem[i];
        if (!gm_ref.empty()) {
            std::cout << gm_ref << std::endl;
            count++;
        }
    }
    if (count == 0) {
        std::cout << "None" << std::endl;
    }
}

const int RNM::push_gmem(const GMem &gm) {
    for (int i = 0; i < len_gmem; i++) {
        if (ptr_gmem[i].empty()) {
            ptr_gmem[i].set_res(gm.ptr, gm.bytes);
            ptr_gmem[i].set_next(-1);
            return i;
        }
    }
    // if the shared memory is full:
    return -1;
}

const int RNM::last_rec(const int idx) const {
    if (ptr_gmem[idx].is_tail()) {
        return idx;
    }
    return last_rec(ptr_gmem[idx].next);
}

void RNM::link_rnode_gmem(RNode *rn, const int gmem_idx) {
    if (rn->get_entry() == -1) {
        rn->set_rec_entry(gmem_idx);
    } else {
        const int tail_idx = last_rec(rn->get_entry());
        if (tail_idx != gmem_idx) {
            ptr_gmem[tail_idx].set_next(gmem_idx);
        }
    }
}

template <typename T>
const int RNM::find_gmem(const RNode *rn, const T dptr) const {
    int g_idx = rn->res_entry_idx;
    while (!ptr_gmem[g_idx].empty()) {
        if (*static_cast<T*>(ptr_gmem[g_idx].ptr) == dptr) {
            return g_idx;
        }
        g_idx = ptr_gmem[g_idx].next;
    }
    return -1;
}

template<typename T>
void RNM::add_gmem(void* dptr, const std::size_t bytes) {
    // prepare data
    const pid_st key = std::make_tuple(pid, stime);
    const GMem gm = GMem(dptr, bytes);

    // check if the rnode exist
    RNode *target_rn = find_rnode(key);

    if (target_rn != nullptr) {
        // push the record to shared memory
        int g_idx = find_gmem(target_rn, *static_cast<T*>(dptr));
        if (g_idx == -1) {
            g_idx = push_gmem(gm);
        } else {
            // mark: do we need to change the bytes registered?
            // which should not accur. 
            ptr_gmem[g_idx].set_res(dptr, bytes);
        }
        // if the process already exists
        link_rnode_gmem(target_rn, g_idx);
    } else {
        // if the node does not exist
        // create node first
        RNode val_rn = RNode(pid, stime, -1);
        RNode *rn = insert_rnode(key, val_rn);

        // push the record to shared memory
        int g_idx = find_gmem(rn, *static_cast<T*>(dptr));
        if (g_idx == -1) {
            g_idx = push_gmem(gm);
        } else {
            // mark: do we need to change the bytes registered?
            // which should not accur. 
            ptr_gmem[g_idx].set_res(dptr, bytes);
        }

        // then insert resource
        link_rnode_gmem(rn, g_idx);
    }
}

template<typename T>
void RNM::remove_gmem_by_dptr(const int prev_idx, const int g_idx,
                              T dptr) {
    if (ptr_gmem[g_idx].empty()) {
        return;
    }
    void* tmp_ptr = ptr_gmem[g_idx].ptr;
    if (*static_cast<T*>(tmp_ptr) == dptr) {
        if (prev_idx == -1) {
            ptr_gmem[g_idx].shallow_delete();
        } else {
            ptr_gmem[prev_idx].set_next(ptr_gmem[g_idx].next);
            ptr_gmem[g_idx].shallow_delete();
        }
    }
    remove_gmem_by_dptr(g_idx, ptr_gmem[g_idx].next, dptr);
}

template<typename T>
void RNM::remove_gmem(T dptr) {
    print_rnodes();
    print_gmem();

    const pid_st key = std::make_tuple(pid, stime);

    RNode *target_rn = find_rnode(key);
    if (target_rn == nullptr)
    {
        return;
    }
    
    remove_gmem_by_dptr(-1, target_rn->get_entry(), dptr);
}

std::size_t RNM::gmem_used(void) {
    std::size_t used = 0;

    for (int i = 0; i < len_rnode; i++) {
        RNode &rna_ref = ptr_rnode[i];
        if (rna_ref.get_stat() == RNodeStatus::EXIST) {
            if (rna_ref.process_exist()) {
                for (int gi = rna_ref.get_entry(); gi > -1;
                     gi = ptr_gmem[i].next) {
                    used += ptr_gmem[gi].bytes;
                }
            }
        }
    }

    return used;
}

template void RNM::add_gmem<CUdeviceptr>(void*, const std::size_t);
template void RNM::add_gmem<CUarray>(void*, const std::size_t);
template void RNM::add_gmem<CUmipmappedArray>(void*, const std::size_t);

template const int RNM::find_gmem<CUdeviceptr>(const RNode*, const CUdeviceptr) const;
template const int RNM::find_gmem<CUarray>(const RNode*, const CUarray) const;
template const int RNM::find_gmem<CUmipmappedArray>(const RNode*, const CUmipmappedArray) const;

template void RNM::remove_gmem_by_dptr<CUdeviceptr>(const int, const int, CUdeviceptr);
template void RNM::remove_gmem_by_dptr<CUarray>(const int, const int, CUarray);
template void RNM::remove_gmem_by_dptr<CUmipmappedArray>(const int, const int, CUmipmappedArray);

template void RNM::remove_gmem<CUdeviceptr>(CUdeviceptr);
template void RNM::remove_gmem<CUarray>(CUarray);
template void RNM::remove_gmem<CUmipmappedArray>(CUmipmappedArray);