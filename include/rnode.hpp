//
//  rnode.h
//  shmht
//
//  Created by Wang Zhang on 3/22/20.
//  Copyright © 2020 Wang Zhang. All rights reserved.
//

#include <iostream>
#include <string>
#include <tuple>

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

#include <signal.h>

#ifndef rnode_h
#define rnode_h

typedef std::tuple<int32_t, uint64_t> pid_st;

enum RNodeStatus { NONEXIST = 0, DELETED, EXIST };

struct RNode {
    int pid;
    unsigned long t;
    int res_entry_idx;
    RNodeStatus stat;

    RNode() { initialize(); }

    // Copy Constructor
    RNode(const RNode &rn) {
        this->pid = rn.pid;
        this->t = rn.t;
        this->res_entry_idx = rn.res_entry_idx;
        this->stat = rn.stat;
    }

    RNode(const int npid, const unsigned long nt, const int nres_entry_idx) {
        this->pid = npid;
        this->t = nt;
        this->res_entry_idx = nres_entry_idx;
        this->stat = RNodeStatus::EXIST;
    }

    // copy operator
    RNode &operator=(const RNode &rn) {
        this->pid = rn.pid;
        this->t = rn.t;
        this->res_entry_idx = rn.res_entry_idx;
        this->stat = rn.stat;
        return *this;
    }

    void initialize(void) {
        pid = -1;
        t = 0;
        res_entry_idx = -1;
        stat = RNodeStatus::NONEXIST;
    }

    void set_key(const pid_st &key) {
        pid = std::get<0>(key);
        t = std::get<1>(key);
    }

    void set_rec_entry(const int idx) { res_entry_idx = idx; }

    void print(void) {
        std::cout << "PID: " << pid << " time: " << t
                  << " res_entry_idx: " << res_entry_idx << std::endl;
    }

    RNodeStatus get_stat(void) { return stat; }

    bool same_key(const pid_st &key) const {
        return pid == std::get<0>(key) && t == std::get<1>(key);
    }

    void shallow_delete(void) { stat = RNodeStatus::DELETED; }

    const int get_entry(void) { return res_entry_idx; }

    bool process_exist(void) {
        // check pid exist
        if (kill((pid_t)pid, 0) == -1) {
            return false;
        }
        // check stime
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

            return strtoul(stime_placeholder.c_str(), NULL, 10) == t;
        }
        // cannot open /proc/{pid}/stat
        return false;
    }

    friend std::ostream &operator<<(std::ostream &os, const RNode &rn);
};

std::ostream &operator<<(std::ostream &os, const RNode &rn);

#endif /* rnode_h */
