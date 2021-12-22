#ifndef CACHEGENERAL_H
#define CACHEGENERAL_H

#define verbose false

#include <stdio.h>
#include <stdlib.h>
#include <cmath>
#include <vector>

typedef struct cache_mem{
    bool valid = false;
    bool dirty = false;
    unsigned int tag = 0;
    unsigned int counter = 0;
    
}cache_cell;

class cache
{
    public:
        cache_cell **Cache_cell;
        unsigned int sets;
        unsigned int num_index;
        unsigned int num_block_offset;
        unsigned int tag;
        unsigned int TAG;
        unsigned int INDEX;
        unsigned int OFFSET;
        unsigned int ASSOSC;
        int LEVEL;

    cache(int num_ways,int cache_size,int assosc,int block_size,int level);

    void find_tag_index_blockoffset(unsigned int address,unsigned int *tag_index_offset);

    void hit_or_miss(unsigned int address,unsigned int *arr);

    
        
};


#endif