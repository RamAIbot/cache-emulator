#include "cache_general.h"


cache::cache(int num_ways,int cache_size,int assosc,int block_size,int level)
{
    sets = (cache_size)/(assosc*block_size);
    num_index = log2(sets);
    num_block_offset = log2(block_size);
    tag = 32 - num_index - num_block_offset;
    TAG = 0;
    INDEX = 0;
    OFFSET = 0;
    ASSOSC = assosc;
    LEVEL = level;

    Cache_cell = new cache_cell*[sets];
    for(unsigned int i=0;i<sets;i++)
    {
        Cache_cell[i] = new cache_cell[num_ways];
    }

    //ADDDED FOR TEST
    for(int i=0;i<sets;i++)
    {
        for(int j=0;j<num_ways;j++)
        {
            Cache_cell[i][j].counter = j;
        }
    }
    //END OF ADDED FOR TEST


}


void cache::find_tag_index_blockoffset(unsigned int address,unsigned int *tag_index_offset)
{
    
    unsigned int power = pow(2,num_block_offset) - 1;
    OFFSET = (address >> 0) & power;
	if(verbose)
		printf("%d\n",OFFSET);

    unsigned int power_index = pow(2,num_index) - 1;
    INDEX = (address >> num_block_offset) & power_index;
	if(verbose)
		printf("%d\n",INDEX);

    unsigned int tag_index = pow(2,tag) - 1;
    int new_size = num_block_offset + num_index;
    TAG = (address >> new_size) & tag_index;
	if(verbose)
		printf("%x\n",TAG);

    tag_index_offset[0] = TAG;
    tag_index_offset[1] = INDEX;
    tag_index_offset[2] = OFFSET;
    
}


void cache::hit_or_miss(unsigned int address,unsigned int *arr)
{
    bool hit = false;
    int index = -1;
    for(unsigned int i=0;i<ASSOSC;i++)
    {
        if((Cache_cell[INDEX][i].valid == true) && (Cache_cell[INDEX][i].tag == TAG))
        {
            hit = true;
            index = i;
            arr[0] = (int)hit;
            arr[1] = index;
            return;
        }
    }

    arr[0] = (int)hit;
    arr[1] = index;
    return;
}

