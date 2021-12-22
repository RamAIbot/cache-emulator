#ifndef SIM_CACHE_H
#define SIM_CACHE_H

#include <bits/stdc++.h>
#include "cache_general.h"

typedef struct cache_params{
    unsigned long int block_size;
    unsigned long int l1_size;
    unsigned long int l1_assoc;
    unsigned long int vc_num_blocks;
    unsigned long int l2_size;
    unsigned long int l2_assoc;
}cache_params;

typedef struct cache_performace_params
{
    unsigned long int num_L1_reads = 0;
    unsigned long int num_L1_read_misses = 0;
    unsigned long int num_L1_writes = 0;
    unsigned long int num_L1_write_misses = 0;
    unsigned long int num_swap_requests = 0;
    float swap_request_rate = 0.0;
    unsigned long int num_swaps = 0;
    float combined_L1_VC_miss_rate = 0.0;
    unsigned long int num_writebacks_from_L1_or_VC = 0;
    unsigned long int num_L2_reads = 0;
    unsigned long int num_L2_read_misses = 0;
    unsigned long int num_L2_writes = 0;
    unsigned long int num_L2_write_misses = 0;
    float num_L2_miss_rate = 0.0;
    unsigned long int num_writebacks_from_L2 = 0;
    unsigned long int total_mem_traffic = 0;
}performance_params;

// Put additional data structures here as per your requirement

void cache_read_hit(unsigned int address,unsigned int element_loc,cache cache_memory);

int cache_read_miss(unsigned int address,std::vector<cache> caches,cache cache_memory,int level);

int cache_read(std::vector<cache> cache_mem_hier,unsigned int addr);

void cache_write_hit(unsigned int address,unsigned int element_loc,cache cache_memory);

void cache_write_miss(unsigned int address,cache cache_memory,std::vector<cache> caches,int level);

void cache_write(unsigned int addr,cache cache_memory,std::vector<cache> caches);

/* --------------------NEEDED ONES--------------------------*/
void print_set_contents(cache cache_memory,unsigned int setno);

void print_contents(cache cache_memory);

void cache_read_hit_lru(unsigned int address,unsigned int element_loc,cache cache_memory);

void write_to_next_level(unsigned int address,cache cache_memory,std::vector<cache> cache_hier);

int Update_cache(unsigned int address,unsigned int cache_index,cache cache_memory,std::vector<cache> cache_hier);

int cache_read(std::vector<cache> cache_mem_hier,unsigned int addr);

void cache_write_hit_lru(unsigned int address,unsigned int element_loc,cache cache_memory);

void cache_write_hit(unsigned int address,unsigned int element_loc,cache cache_memory);

void cache_write(std::vector<cache> cache_mem_hier,unsigned int addr);

#endif
