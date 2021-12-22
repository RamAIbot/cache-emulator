//#include <stdio.h>
//#include <stdlib.h>
#include "sim_cache.h"
#include "cache_general.h"


/*  argc holds the number of command line arguments
    argv[] holds the commands themselves

    Example:-
    sim_cache 32 8192 4 7 262144 8 gcc_trace.txt
    argc = 8
    argv[0] = "sim_cache"
    argv[1] = "32"
    argv[2] = "8192"
    ... and so on
*/
/* ------------------------------------------TESTING------------------------------------*/
performance_params per_params;
bool is_l2 = false;
bool is_vc_enabled = false;
cache *vc;

//bool verbose=false;

void print_set_contents(cache cache_memory,unsigned int setno)
{
    for(int i=0;i<cache_memory.ASSOSC;i++)
    {
        
        printf("%x ",cache_memory.Cache_cell[setno][i].tag);
        
    }
    printf("\n");
}

bool counter_sorting(const cache_cell &a, const cache_cell &b)
{
    return a.counter < b.counter;
}

void print_contents(cache cache_memory)
{
    for(int i=0;i<cache_memory.sets;i++)
    {
        printf("\tset %d:\t",i);
        std::vector<cache_cell> total_array;
        for(int j=0;j<cache_memory.ASSOSC;j++)
        {   
            total_array.push_back(cache_memory.Cache_cell[i][j]);
        }   

        std::sort(total_array.begin(),total_array.end(),counter_sorting);
        
        for(int j=0;j<cache_memory.ASSOSC;j++)
        {
            // printf("%d ",cache_memory.Cache_cell[i][j].valid);
            // printf("%d ",cache_memory.Cache_cell[i][j].dirty);
            // printf("%x ",cache_memory.Cache_cell[i][j].tag);
            // printf("%d ",cache_memory.Cache_cell[i][j].counter);
            // printf(" | ");

            //printf("%d ",total_array[j].valid);
            printf("%x ",total_array[j].tag);
            if(total_array[j].dirty == 1)
            {
                printf(" D\t");
            }
            else
            {
                printf("\t\t");
            }
            //printf("%d ",total_array[j].counter);
            printf("\t");
         }
        printf("\n");
    }
}


void print_contents_vc(cache* cache_memory)
{
    for(int i=0;i<cache_memory->sets;i++)
    {
        printf("set %d: ",i);
        std::vector<cache_cell> total_array;
        for(int j=0;j<cache_memory->ASSOSC;j++)
        {   
            total_array.push_back(cache_memory->Cache_cell[i][j]);
        }   

        std::sort(total_array.begin(),total_array.end(),counter_sorting);
        
        for(int j=0;j<cache_memory->ASSOSC;j++)
        {
            // printf("%d ",cache_memory.Cache_cell[i][j].valid);
            // printf("%d ",cache_memory.Cache_cell[i][j].dirty);
            // printf("%x ",cache_memory.Cache_cell[i][j].tag);
            // printf("%d ",cache_memory.Cache_cell[i][j].counter);
            // printf(" | ");

            //printf("%d ",total_array[j].valid);
            printf("%x ",total_array[j].tag);
            if(total_array[j].dirty == 1)
            {
                printf(" D\t");
            }
            else
            {
                printf("\t");
            }
            //printf("%d ",total_array[j].counter);
            printf("\t");
        }
       printf("\n");
    }
}

void cache_read_hit_lru(unsigned int address,unsigned int element_loc,cache cache_memory)
{
	if(verbose)
		printf("L%d hit\n",cache_memory.LEVEL);
	
    int previous_count = -1;
    for(unsigned int i=0;i<cache_memory.ASSOSC;i++)
    {
        if(i == element_loc)
        {
            previous_count = cache_memory.Cache_cell[cache_memory.INDEX][i].counter;
            cache_memory.Cache_cell[cache_memory.INDEX][i].counter = 0;
        }
    }
    for(unsigned int i=0;i<cache_memory.ASSOSC;i++)
    {        
        if(i!=element_loc)
        {
            if(cache_memory.Cache_cell[cache_memory.INDEX][i].counter < previous_count)
                cache_memory.Cache_cell[cache_memory.INDEX][i].counter = ((cache_memory.Cache_cell[cache_memory.INDEX][i].counter) + 1) % cache_memory.ASSOSC;        
        }
        
    }
}

void write_to_next_level(unsigned int address,cache cache_memory,std::vector<cache> cache_hier)
{
    unsigned int tag_index_offset_l1[3];
    unsigned int arr_l1[2];

    unsigned int tag_index_offset_l2[3];
    unsigned int arr_l2[2];

    unsigned int cache_level = cache_memory.LEVEL - 1;

    int setno = -1;
    if(is_l2 || is_vc_enabled)
        per_params.num_writebacks_from_L1_or_VC += 1;

    if(is_l2)
    {
        per_params.num_L2_writes += 1;
        cache_hier[cache_level + 1].find_tag_index_blockoffset(address,tag_index_offset_l2);
        cache_hier[cache_level + 1].hit_or_miss(tag_index_offset_l2[0],arr_l2); 

        if(arr_l2[0])
        {
            //L2 hit
			if(verbose)
				printf("\nL2 hit\n");
			
            cache_write_hit(tag_index_offset_l2[0],arr_l2[1],cache_hier[cache_level + 1]);
			
			if(verbose)
				printf("\nWriting from L1 to next level L2\n");
			
            cache_write_hit_lru(tag_index_offset_l2[0],arr_l2[1],cache_hier[cache_level + 1]);
			
			if(verbose)
				printf("\n L2 LRU update\n");
           
        }

        else
        {
            //L2 miss
			if(verbose)
				printf("\nL2 cache miss\n");
			
            per_params.num_L2_write_misses += 1;
            //Update L2 from memory
			if(verbose)
				printf("\nUpdate L2 cache from memory\n");
            //ADDED Sep 19 to test total mem access
                per_params.total_mem_traffic += 1;
            //ENDS
            int setno = Update_cache(tag_index_offset_l2[0],tag_index_offset_l2[1],cache_hier[cache_level + 1],cache_hier);
            //Write from L1 to L2
			if(verbose)
				printf("\nWrite operation in L2 from L1\n");
            cache_write_hit(tag_index_offset_l2[0],setno,cache_hier[cache_level + 1]);

            cache_write_hit_lru(tag_index_offset_l2[0],setno,cache_hier[cache_level + 1]);
			
			if(verbose)
				printf("\nL2 LRU update\n");

        }
    }

    else
    {
		if(verbose)
			printf("\n Writing from L1 to mem\n");
		
        per_params.total_mem_traffic += 1;
    }
    
    

}

void write_from_vc_to_next_level(unsigned int address,cache* cache_memory,std::vector<cache> cache_hier)
{
    unsigned int tag_index_offset_l1[3];
    unsigned int arr_l1[2];

    unsigned int tag_index_offset_l2[3];
    unsigned int arr_l2[2];

    unsigned int cache_level = 0;

    int setno = -1;
    per_params.num_writebacks_from_L1_or_VC += 1;
    if(is_l2)
    {
        per_params.num_L2_writes += 1;
       
        cache_hier[cache_level + 1].find_tag_index_blockoffset(address,tag_index_offset_l2);
        cache_hier[cache_level + 1].hit_or_miss(tag_index_offset_l2[0],arr_l2); 

        if(arr_l2[0])
        {
            //L2 hit
			if(verbose)
				printf("\nL2 cache hit\n");
			
            cache_write_hit(tag_index_offset_l2[0],arr_l2[1],cache_hier[cache_level + 1]);
			
			if(verbose)
				printf("\nWriting from L1 to next level L2\n");
			
            cache_write_hit_lru(tag_index_offset_l2[0],arr_l2[1],cache_hier[cache_level + 1]);
			
			if(verbose)
				printf("\n L2 LRU update\n");
           
        }

        else
        {
            //L2 miss
			if(verbose)
				printf("\nL2 cache miss\n");
            //Update L2 from memory
            per_params.num_L2_write_misses += 1;
            //ADDED Sep 19 to test total mem access
                per_params.total_mem_traffic += 1;
            //ENDS
			if(verbose)
				printf("\nUpdate L2 cache from memory\n");
            int setno = Update_cache(tag_index_offset_l2[0],tag_index_offset_l2[1],cache_hier[cache_level + 1],cache_hier);
            //Write from L1 to L2
			
			if(verbose)
				printf("\nWrite operation in L2 from L1\n");
			
            cache_write_hit(tag_index_offset_l2[0],setno,cache_hier[cache_level + 1]);

            cache_write_hit_lru(tag_index_offset_l2[0],setno,cache_hier[cache_level + 1]);
			
			if(verbose)
				printf("\nL2 LRU update\n");

        }
    }

    else
    {
		if(verbose)
			printf("\n Writing from L1 to mem\n");
		
        per_params.total_mem_traffic += 1;

    }
    

}

void place_in_vc(unsigned int address,bool valid,bool dirty,std::vector<cache> cache_hier)
{
    int index_placed = -1;
    int prev_count = -1;


    unsigned int tag_index_offset_vc[3];

    vc->find_tag_index_blockoffset(address,tag_index_offset_vc);

    for(int i=0;i<vc->ASSOSC;i++)
    {
        if(vc->Cache_cell[0][i].counter == ((vc->ASSOSC)-1))
        {

            if(vc->Cache_cell[0][i].dirty == false)
            {
				if(verbose)
					printf("\n VC replace and VC LRU update\n");
				
                vc->Cache_cell[0][i].tag = tag_index_offset_vc[0];
                vc->Cache_cell[0][i].valid = valid;
                vc->Cache_cell[0][i].dirty = dirty;
                index_placed = i;
                prev_count = vc->Cache_cell[0][i].counter;
                vc->Cache_cell[0][i].counter = 0;
                break;
            }

            else
            {
				if(verbose)
					printf("\n VC dirty bit set\n");
				
				if(verbose)
					printf("\n Writing from L1 to L2 or memory\n");

                int new_address = (vc->Cache_cell[0][i].tag << (vc->num_index + vc->num_block_offset)) | (0 << vc->num_block_offset) | 0;
                write_from_vc_to_next_level(new_address,vc,cache_hier);
                //remove dirty bit
                vc->Cache_cell[0][i].dirty = false;

				if(verbose)
					printf("\n VC replace and VC LRU update\n");
				
                vc->Cache_cell[0][i].tag = tag_index_offset_vc[0];
                vc->Cache_cell[0][i].valid = valid;
                vc->Cache_cell[0][i].dirty = dirty;
                index_placed = i;
                prev_count = vc->Cache_cell[0][i].counter;
                vc->Cache_cell[0][i].counter = 0;
            }
            
        }
    }

    for(int i=0;i<vc->ASSOSC;i++)
    {
        if(i!=index_placed)
        {
            if(vc->Cache_cell[0][i].counter < prev_count)
            {
                vc->Cache_cell[0][i].counter = ((vc->Cache_cell[0][i].counter) + 1) % vc->ASSOSC;        
            }
        }
    }
}

int Update_cache(unsigned int address,unsigned int cache_index,cache cache_memory,std::vector<cache> cache_hier)
{
    int index_placed = -1;
    int previous_count = -1;

    for(unsigned int i=0;i<cache_memory.ASSOSC;i++)
    {
        if(cache_memory.Cache_cell[cache_memory.INDEX][i].valid == false)
        {
            cache_memory.Cache_cell[cache_memory.INDEX][i].tag = cache_memory.TAG;
            cache_memory.Cache_cell[cache_memory.INDEX][i].valid = true;
            previous_count = cache_memory.Cache_cell[cache_memory.INDEX][i].counter;
            cache_memory.Cache_cell[cache_memory.INDEX][i].counter = 0;
            index_placed = i;   //CHANGE INDEX PLACED 
            break;
        }
    }

    /*ADDED FOR TEST*/
    // if(cache_memory.ASSOSC == 1)
    // {
       

    //     if(cache_memory.Cache_cell[cache_memory.INDEX][0].valid == false)
    //     {
    //         cache_memory.Cache_cell[cache_memory.INDEX][0].tag = cache_memory.TAG;
    //         cache_memory.Cache_cell[cache_memory.INDEX][0].valid = true;
    //         cache_memory.Cache_cell[cache_memory.INDEX][0].counter = 0;
    //         index_placed = 0;   //CHANGE INDEX PLACED 
    //         return index_placed;
    //     }


    //     if(cache_memory.Cache_cell[cache_memory.INDEX][0].dirty == false)
    //         {
    //             cache_memory.Cache_cell[cache_memory.INDEX][0].tag = cache_memory.TAG;
    //         }
    //         else
    //         {
    //             printf("Clean the dirty bit of %d level,Write to next hierarchy",cache_memory.LEVEL);
                
    //             int new_address = (cache_memory.Cache_cell[cache_memory.INDEX][0].tag << (cache_memory.num_index + cache_memory.num_block_offset)) | (cache_index << cache_memory.num_block_offset) | 0;
    //             printf("\n TAG: %d\n",cache_memory.Cache_cell[cache_memory.INDEX][0].tag);
    //             printf("\n INDEX: %d\n",cache_index);
    //             printf("\nNEW ADDRESS: %d\n",new_address);

    //             if(cache_memory.LEVEL != 2)
    //                 write_to_next_level(new_address,cache_memory,cache_hier); //ERROR PASS THE TAG FROM BLOCK NOT THE ORIGINAL ONES
    //             else    
    //                 printf("\nWrite update from L2 to main memeory\n");

    //             cache_memory.Cache_cell[cache_memory.INDEX][0].dirty = false;
    //             cache_memory.Cache_cell[cache_memory.INDEX][0].tag = cache_memory.TAG;
                
    //         }
    //         cache_memory.Cache_cell[cache_memory.INDEX][0].valid = true;
    //         cache_memory.Cache_cell[cache_memory.INDEX][0].counter = 0;
    //         index_placed = 0;
    //         return index_placed;
    // }
    /*END OF ADDED FOR TEST*/
    
    if(index_placed == -1)
    {
    for(unsigned int i=0;i<cache_memory.ASSOSC;i++)
    {
		if(verbose)
			printf("\n COUNTER: %d\n",cache_memory.Cache_cell[cache_memory.INDEX][i].counter);
        previous_count = cache_memory.Cache_cell[cache_memory.INDEX][i].counter;
        if(cache_memory.Cache_cell[cache_memory.INDEX][i].counter == cache_memory.ASSOSC-1)
        {
            if(cache_memory.Cache_cell[cache_memory.INDEX][i].dirty == false)
            {
                if(is_vc_enabled)
                {
                    //int new_address = (cache_memory.Cache_cell[cache_memory.INDEX][i].tag << (cache_memory.num_index + cache_memory.num_block_offset)) | (cache_memory.INDEX << cache_memory.num_block_offset) | 0; //MODIFIED THIS CHANGE IN ORDER CAUSES SERIOUS FLAW
                    cache_memory.Cache_cell[cache_memory.INDEX][i].tag = cache_memory.TAG;
                    //bool valid = cache_memory.Cache_cell[cache_memory.INDEX][i].valid;
                    //bool dirty = cache_memory.Cache_cell[cache_memory.INDEX][i].dirty;
                    //place_in_vc(new_address,valid,dirty,cache_hier);
                }

                else
                {
                    cache_memory.Cache_cell[cache_memory.INDEX][i].tag = cache_memory.TAG;
                }
                
            }
            else
            {
				if(verbose)
					printf("Clean the dirty bit of %d level,Write to next hierarchy",cache_memory.LEVEL); //NO NEED JUST PUT IN VC
                
                int new_address = (cache_memory.Cache_cell[cache_memory.INDEX][i].tag << (cache_memory.num_index + cache_memory.num_block_offset)) | (cache_index << cache_memory.num_block_offset) | 0;
                // printf("\n TAG: %d\n",cache_memory.Cache_cell[cache_memory.INDEX][i].tag);
                // printf("\n INDEX: %d\n",cache_index);
                // printf("\n NEW ADDRESS: %d\n",new_address);

                if(cache_memory.LEVEL != 2)
                {
                    per_params.num_writebacks_from_L1_or_VC += 1;
                    
                    if(is_vc_enabled)
                    {
                        bool valid = cache_memory.Cache_cell[cache_memory.INDEX][i].valid;
                        bool dirty = cache_memory.Cache_cell[cache_memory.INDEX][i].dirty;
                        place_in_vc(new_address,valid,dirty,cache_hier);
                    }
                    else
                    {
                        write_to_next_level(new_address,cache_memory,cache_hier); //ERROR PASS THE TAG FROM BLOCK NOT THE ORIGINAL ONES
                    }
                    
                }
                    
                else  

                {
                    per_params.num_writebacks_from_L2 += 1;
                    per_params.total_mem_traffic += 1;
					if(verbose)
						printf("\nWrite update from L2 to main memory\n");
                } 
                    
                cache_memory.Cache_cell[cache_memory.INDEX][i].dirty = false; //CHANGED so as to capture in VC
                cache_memory.Cache_cell[cache_memory.INDEX][i].tag = cache_memory.TAG;
                
            }
            cache_memory.Cache_cell[cache_memory.INDEX][i].valid = true;
            cache_memory.Cache_cell[cache_memory.INDEX][i].counter = 0;
            index_placed = i; 
            break;
        }
    }
    }

    for(unsigned int i=0;i<cache_memory.ASSOSC;i++)
    {
        
        if((index_placed != i) && (cache_memory.Cache_cell[cache_memory.INDEX][i].valid == true))
        {
			if(verbose)
				printf("\nPREC: %d\n",previous_count);
			
            if(cache_memory.Cache_cell[cache_memory.INDEX][i].counter < previous_count)
            {
                //printf("\nHERE\n");
                cache_memory.Cache_cell[cache_memory.INDEX][i].counter = ((cache_memory.Cache_cell[cache_memory.INDEX][i].counter) + 1) % cache_memory.ASSOSC;
            }
                
        }
    }
    return index_placed;


}


bool should_call_victim_cache(cache cache_memory,int index,int assosc)
{   
    //Check if cache is full
    for(int i=0;i<assosc;i++)
    {
        if(cache_memory.Cache_cell[index][i].valid == false)
        {
            return false;
        }
    }
    return true;
}

void update_lru_vc(int element_loc)
{
    int previous_count = -1;
    for(unsigned int i=0;i<vc->ASSOSC;i++)
    {
        if(i == element_loc)
        {
            previous_count = vc->Cache_cell[0][i].counter;
            vc->Cache_cell[0][i].counter = 0;
        }
    }
    for(unsigned int i=0;i<vc->ASSOSC;i++)
    {        
        if(i!=element_loc)
        {
            if(vc->Cache_cell[0][i].counter < previous_count)
                vc->Cache_cell[0][i].counter = ((vc->Cache_cell[0][i].counter) + 1) % vc->ASSOSC;        
        }
        
    }
}

int swap_l1_and_vc(unsigned int l1_cache_index,cache cache_memory,int element_loc_vc)
{
       
    //int element_loc_vc = -1;
    unsigned int element_loc_l1 = -1;
    // for(int i=0;i<vc->ASSOSC;i++)
    // {
    //     if(vc->Cache_cell[0][i].counter == (vc->ASSOSC - 1))
    //     {
    //         element_loc_vc = i;
    //         break;
    //     }
    // }

    for(int i=0;i<cache_memory.ASSOSC;i++)
    {
        if(cache_memory.Cache_cell[l1_cache_index][i].counter == (cache_memory.ASSOSC - 1))
        {
            element_loc_l1 = i;
            break;
        }
    }

    unsigned int full_address_from_l1_tag = (cache_memory.Cache_cell[cache_memory.INDEX][element_loc_l1].tag << (cache_memory.num_index + cache_memory.num_block_offset)) | (l1_cache_index << cache_memory.num_block_offset) | 0;
    int l1_index = cache_memory.INDEX;
    bool l1_valid = cache_memory.Cache_cell[l1_index][element_loc_l1].valid;
    bool l1_dirty = cache_memory.Cache_cell[l1_index][element_loc_l1].dirty;

    unsigned int vc_full_address_from_tag = (vc->Cache_cell[0][element_loc_vc].tag << (vc->num_index + vc->num_block_offset)) | (0 << vc->num_block_offset) | 0;
    
    unsigned int tag_index_offset_l1[3];
    unsigned int tag_index_offset_vc[3];

    bool vc_valid = vc->Cache_cell[0][element_loc_vc].valid;
    bool vc_dirty = vc->Cache_cell[0][element_loc_vc].dirty;

    cache_memory.find_tag_index_blockoffset(vc_full_address_from_tag,tag_index_offset_l1);
    vc->find_tag_index_blockoffset(full_address_from_l1_tag,tag_index_offset_vc);

    cache_memory.Cache_cell[l1_index][element_loc_l1].tag = tag_index_offset_l1[0];
    cache_memory.Cache_cell[l1_index][element_loc_l1].dirty = vc_dirty;
    cache_memory.Cache_cell[l1_index][element_loc_l1].valid = vc_valid;


    vc->Cache_cell[0][element_loc_vc].tag = tag_index_offset_vc[0];
    vc->Cache_cell[0][element_loc_vc].dirty = l1_dirty;
    vc->Cache_cell[0][element_loc_vc].valid = l1_valid;

    //Update LRU of VC;
    update_lru_vc(element_loc_vc);
	if(verbose)
		printf("\n VC update LRU\n");

    return element_loc_l1;
}

int cache_read(std::vector<cache> cache_mem_hier,unsigned int addr)
{
    unsigned int tag_index_offset_l1[3];
    unsigned int arr_l1[2];

    unsigned int tag_index_offset_l2[3];
    unsigned int arr_l2[2];

    int setno = -1;
    int cache_level = 0;

    per_params.num_L1_reads += 1;
    cache_mem_hier[cache_level].find_tag_index_blockoffset(addr,tag_index_offset_l1);
    cache_mem_hier[cache_level].hit_or_miss(tag_index_offset_l1[0],arr_l1);    
    if(arr_l1[0])
    {
        //L1 cache hit;
		if(verbose)
			printf("\nL1 hit\n");
		
        cache_read_hit_lru(tag_index_offset_l1[0],arr_l1[1],cache_mem_hier[cache_level]);
        setno = arr_l1[1];
    }
    else
    {
        //L1 cache miss;
		if(verbose)
			printf("\nL1 miss\n");
        //Check whether L1 cache's set is full or not
        per_params.num_L1_read_misses += 1;
        bool check_vc = should_call_victim_cache(cache_mem_hier[cache_level],tag_index_offset_l1[1],cache_mem_hier[cache_level].ASSOSC);
        if(check_vc && is_vc_enabled)
        {
            //Check in VC
			if(verbose)
				printf("\n Checking in VC\n");
			
            per_params.num_swap_requests += 1;
            unsigned int tag_index_offset_vc[3];
            unsigned int arr_vc[2];
            vc->find_tag_index_blockoffset(addr,tag_index_offset_vc);
			if(verbose)
				printf("%d %d %d %d",addr,tag_index_offset_vc[0],tag_index_offset_vc[1],tag_index_offset_vc[2]);
            vc->hit_or_miss(tag_index_offset_vc[0],arr_vc);

            if(arr_vc[0])
            {
                //VC hit
				if(verbose)
					printf("\n VC hit\n");
				
                per_params.num_swaps += 1;
                //swap_l1_and_vc(tag_l1,tag_vc,l1_index,cache_memory)
                setno = swap_l1_and_vc(tag_index_offset_l1[1],cache_mem_hier[cache_level],arr_vc[1]);
                cache_read_hit_lru(tag_index_offset_l1[0],setno,cache_mem_hier[cache_level]);
                return setno;

            }

            else
            {
                //VC miss
				if(verbose)
					printf("\n VC miss.. Checking in L2 cache\n");
				
                //MOVE the block which is going to be evicted to victim
                if(is_vc_enabled)
                {
                    int element_location = -1;
                    for(unsigned int i=0;i<cache_mem_hier[cache_level].ASSOSC;i++)
                    {
                        if(cache_mem_hier[cache_level].Cache_cell[tag_index_offset_l1[1]][i].counter == ((cache_mem_hier[cache_level].ASSOSC) - 1))
                        {
                            element_location = i;
                            break;
                        }

                    }
                    int new_address = (cache_mem_hier[cache_level].Cache_cell[cache_mem_hier[cache_level].INDEX][element_location].tag << (cache_mem_hier[cache_level].num_index + cache_mem_hier[cache_level].num_block_offset)) | (cache_mem_hier[cache_level].INDEX << cache_mem_hier[cache_level].num_block_offset) | 0; //MODIFIED THIS CHANGE IN ORDER CAUSES SERIOUS FLAW
                    
                    bool valid = cache_mem_hier[cache_level].Cache_cell[cache_mem_hier[cache_level].INDEX][element_location].valid;
                    bool dirty = cache_mem_hier[cache_level].Cache_cell[cache_mem_hier[cache_level].INDEX][element_location].dirty;
                    
                    place_in_vc(new_address,valid,dirty,cache_mem_hier);
                    cache_mem_hier[cache_level].Cache_cell[cache_mem_hier[cache_level].INDEX][element_location].dirty = false;

                    //cache_memory.Cache_cell[cache_memory.INDEX][i].tag = cache_memory.TAG;
                }
            }
        }

        
        if(is_l2)
        {
            
            per_params.num_L2_reads += 1;
            //SERIOUS BUG FIX
            int element_location = -1;
            for(unsigned int i=0;i<cache_mem_hier[cache_level].ASSOSC;i++)
            {
                if(cache_mem_hier[cache_level].Cache_cell[tag_index_offset_l1[1]][i].counter == ((cache_mem_hier[cache_level].ASSOSC) - 1))
                {
                    element_location = i;
                    break;
                }

            }

            if(cache_mem_hier[cache_level].Cache_cell[tag_index_offset_l1[1]][element_location].dirty == true)
            {
				if(verbose)
					printf("\n L1 dirty bit\n");
                int new_address = (cache_mem_hier[cache_level].Cache_cell[cache_mem_hier[cache_level].INDEX][element_location].tag << (cache_mem_hier[cache_level].num_index + cache_mem_hier[cache_level].num_block_offset)) | (cache_mem_hier[cache_level].INDEX << cache_mem_hier[cache_level].num_block_offset) | 0;
                write_to_next_level(new_address,cache_mem_hier[cache_level],cache_mem_hier);
				
				if(verbose)
					printf("\n Remove dirty bit in L1\n");
                cache_mem_hier[cache_level].Cache_cell[cache_mem_hier[cache_level].INDEX][element_location].dirty = false;

                cache_level+=1;
                cache_mem_hier[cache_level].find_tag_index_blockoffset(addr,tag_index_offset_l2);
                cache_mem_hier[cache_level].hit_or_miss(tag_index_offset_l2[0],arr_l2);
                if(arr_l2[0])
                {
            //L2 cache hit;
					if(verbose)
						printf("\nL2 cache hit\n");
                    cache_read_hit_lru(tag_index_offset_l2[0],arr_l2[1],cache_mem_hier[cache_level]);
            
            //Update back in L1
					if(verbose)
						printf("\nUpdate from L2 back in L1 cache\n");
                    setno = Update_cache(tag_index_offset_l1[0],tag_index_offset_l1[1],cache_mem_hier[cache_level-1],cache_mem_hier);
            
                }

                else
                {
            //L2 cache miss;
            //Update L2 from memory
					if(verbose)
						printf("\nL2 cache miss\n");
					
                    per_params.num_L2_read_misses += 1;
					
					if(verbose)
						printf("\nUpdate L2 cache from memory\n");
					
                    per_params.total_mem_traffic += 1;
                    int dummy = Update_cache(tag_index_offset_l2[0],tag_index_offset_l2[1],cache_mem_hier[cache_level],cache_mem_hier);
            //Update back in L1
					if(verbose)
						printf("\nUpdate from L2 back in L1 cache\n");
                    setno = Update_cache(tag_index_offset_l1[0],tag_index_offset_l1[1],cache_mem_hier[cache_level-1],cache_mem_hier);
                }
            }

            else
            {
                cache_level+=1;
                cache_mem_hier[cache_level].find_tag_index_blockoffset(addr,tag_index_offset_l2);
                cache_mem_hier[cache_level].hit_or_miss(tag_index_offset_l2[0],arr_l2);
                if(arr_l2[0])
                {
            //L2 cache hit;
					if(verbose)
						printf("\nL2 cache hit\n");
                    cache_read_hit_lru(tag_index_offset_l2[0],arr_l2[1],cache_mem_hier[cache_level]);
            
            //Update back in L1
					if(verbose)
						printf("\nUpdate from L2 back in L1 cache\n");
                    setno = Update_cache(tag_index_offset_l1[0],tag_index_offset_l1[1],cache_mem_hier[cache_level-1],cache_mem_hier);
            
                }

                else
                {
            //L2 cache miss;
            //Update L2 from memory
					if(verbose)
						printf("\nL2 cache miss\n");
					
                    per_params.num_L2_read_misses += 1;
					if(verbose)
						printf("\nUpdate L2 cache from memory\n");
                    per_params.total_mem_traffic += 1;
                    int dummy = Update_cache(tag_index_offset_l2[0],tag_index_offset_l2[1],cache_mem_hier[cache_level],cache_mem_hier);
            //Update back in L1
			
					if(verbose)
						printf("\nUpdate from L2 back in L1 cache\n");
                    setno = Update_cache(tag_index_offset_l1[0],tag_index_offset_l1[1],cache_mem_hier[cache_level-1],cache_mem_hier);
                }
            }

            //ENDS
           
        }
        
        else
        {
			if(verbose)
				printf("\nUpdate L1 cache from memory\n");
            per_params.total_mem_traffic +=  1;
            setno = Update_cache(tag_index_offset_l1[0],tag_index_offset_l1[1],cache_mem_hier[cache_level],cache_mem_hier);
        }
        
    }
    return setno;
    
}

// WRITING ====================================

void cache_write_hit_lru(unsigned int address,unsigned int element_loc,cache cache_memory)
{
    //printf("L%d hit\n",cache_memory.LEVEL);
    // for(unsigned int i=0;i<cache_memory.ASSOSC;i++)
    // {
    //     if(i == element_loc)
    //     {
    //         cache_memory.Cache_cell[cache_memory.INDEX][i].counter = 0;
    //     }
    //     else{
    //         cache_memory.Cache_cell[cache_memory.INDEX][i].counter = (cache_memory.Cache_cell[cache_memory.INDEX][i].counter + 1) % cache_memory.ASSOSC;
    //     }
    // }


    int previous_count = -1;
    for(unsigned int i=0;i<cache_memory.ASSOSC;i++)
    {
        if(i == element_loc)
        {
            previous_count = cache_memory.Cache_cell[cache_memory.INDEX][i].counter;
            cache_memory.Cache_cell[cache_memory.INDEX][i].counter = 0;
        }
    }
    for(unsigned int i=0;i<cache_memory.ASSOSC;i++)
    {        
        if(i!=element_loc)
        {
            if(cache_memory.Cache_cell[cache_memory.INDEX][i].counter < previous_count)
                cache_memory.Cache_cell[cache_memory.INDEX][i].counter = ((cache_memory.Cache_cell[cache_memory.INDEX][i].counter) + 1) % cache_memory.ASSOSC;        
        }
        
    }
}


void cache_write_hit(unsigned int address,unsigned int element_loc,cache cache_memory)
{
	if(verbose)
		printf("\nLOC: %d\n",element_loc);
    cache_memory.Cache_cell[cache_memory.INDEX][element_loc].dirty = true;
	
	if(verbose)
		printf("\nL%d set dirty\n",cache_memory.LEVEL);
}

void cache_write(std::vector<cache> cache_mem_hier,unsigned int addr)
{
    unsigned int tag_index_offset_l1[3];
    unsigned int arr_l1[2];

    unsigned int tag_index_offset_l2[3];
    unsigned int arr_l2[2];

    int setno = -1;
    int cache_level = 0;

    per_params.num_L1_writes += 1;
    cache_mem_hier[cache_level].find_tag_index_blockoffset(addr,tag_index_offset_l1);
    cache_mem_hier[cache_level].hit_or_miss(tag_index_offset_l1[0],arr_l1); 

    if(arr_l1[0])
        {
            //L1 hit
			if(verbose)
				printf("\nL1 cache hit\n");
			
            cache_write_hit(tag_index_offset_l1[0],arr_l1[1],cache_mem_hier[cache_level]);
			if(verbose)
				printf("\nWrite opeartion to L1\n");
            cache_write_hit_lru(tag_index_offset_l1[0],arr_l1[1],cache_mem_hier[cache_level]);
			if(verbose)
				printf("\nL1 LRU update\n");
            return;
        }
    else
        {
            //L1 miss
            per_params.num_L1_write_misses += 1;
			if(verbose)
				printf("\nL1 cache miss\n");

            bool check_vc = should_call_victim_cache(cache_mem_hier[cache_level],tag_index_offset_l1[1],cache_mem_hier[cache_level].ASSOSC);
            if(check_vc && is_vc_enabled)
            {
            //Check in VC
				if(verbose)
					printf("\n Checking in VC\n");
                per_params.num_swap_requests += 1;
                unsigned int tag_index_offset_vc[3];
                unsigned int arr_vc[2];
                vc->find_tag_index_blockoffset(addr,tag_index_offset_vc);
                vc->hit_or_miss(tag_index_offset_vc[0],arr_vc);

                if(arr_vc[0])
                {
                //VC hit
					if(verbose)
						printf("\n VC hit\n");
                    per_params.num_swaps += 1;
                //swap_l1_and_vc(tag_l1,tag_vc,l1_index,cache_memory)
					if(verbose)
						printf("\n SWAP operation between L1 and VC\n");
                    setno = swap_l1_and_vc(tag_index_offset_l1[1],cache_mem_hier[cache_level],arr_vc[1]);//ERROR SWAP THE VALUE WHICH IS GOING TO BE REMOVED FROM L1 not new address
					
					if(verbose)
						printf("\nWrite operation in L1\n");
                    cache_write_hit(tag_index_offset_l1[0],setno,cache_mem_hier[cache_level]);
                    cache_write_hit_lru(tag_index_offset_l1[0],setno,cache_mem_hier[cache_level]);
					
					if(verbose)
						printf("\nL1 LRU update\n");
                    return;
                }

                else
                {
                //VC miss
				
					if(verbose)
						printf("\n VC miss.. Checking in L2 cache\n");
					
                    if(is_vc_enabled)
                    {
                        int element_location = -1;
                        for(unsigned int i=0;i<cache_mem_hier[cache_level].ASSOSC;i++)
                        {
                            if(cache_mem_hier[cache_level].Cache_cell[tag_index_offset_l1[1]][i].counter == ((cache_mem_hier[cache_level].ASSOSC) - 1))
                            {
                                element_location = i;
                                break;
                            }

                    }
                    int new_address = (cache_mem_hier[cache_level].Cache_cell[cache_mem_hier[cache_level].INDEX][element_location].tag << (cache_mem_hier[cache_level].num_index + cache_mem_hier[cache_level].num_block_offset)) | (cache_mem_hier[cache_level].INDEX << cache_mem_hier[cache_level].num_block_offset) | 0; //MODIFIED THIS CHANGE IN ORDER CAUSES SERIOUS FLAW
                    
                    bool valid = cache_mem_hier[cache_level].Cache_cell[cache_mem_hier[cache_level].INDEX][element_location].valid;
                    bool dirty = cache_mem_hier[cache_level].Cache_cell[cache_mem_hier[cache_level].INDEX][element_location].dirty;
                    
                    place_in_vc(new_address,valid,dirty,cache_mem_hier);
                    cache_mem_hier[cache_level].Cache_cell[cache_mem_hier[cache_level].INDEX][element_location].dirty = false;

                    //cache_memory.Cache_cell[cache_memory.INDEX][i].tag = cache_memory.TAG;
                    }
                }
            }

        if(is_l2)
            {
                per_params.num_L2_reads += 1;
                //SERIOUS BUG FIX (CHANGE ORDER TO WRITE DIRTY BIT AND THEN READ L2)
                int element_location = -1;
                for(unsigned int i=0;i<cache_mem_hier[cache_level].ASSOSC;i++)
                {
                    if(cache_mem_hier[cache_level].Cache_cell[tag_index_offset_l1[1]][i].counter == ((cache_mem_hier[cache_level].ASSOSC) - 1))
                    {
                        element_location = i;
                        break;
                    }

                }


                if(cache_mem_hier[cache_level].Cache_cell[tag_index_offset_l1[1]][element_location].dirty == true)
                {
                    //write to l2;
					if(verbose)
						printf("\n L1 dirty bit\n");
					
                    int new_address = (cache_mem_hier[cache_level].Cache_cell[cache_mem_hier[cache_level].INDEX][element_location].tag << (cache_mem_hier[cache_level].num_index + cache_mem_hier[cache_level].num_block_offset)) | (cache_mem_hier[cache_level].INDEX << cache_mem_hier[cache_level].num_block_offset) | 0;
                    write_to_next_level(new_address,cache_mem_hier[cache_level],cache_mem_hier);
                    cache_mem_hier[cache_level].Cache_cell[cache_mem_hier[cache_level].INDEX][element_location].dirty = false;

                    cache_level+=1;
                    cache_mem_hier[cache_level].find_tag_index_blockoffset(addr,tag_index_offset_l2);
                    cache_mem_hier[cache_level].hit_or_miss(tag_index_offset_l2[0],arr_l2);

                    if(arr_l2[0])
                    {
            //L2 cache hit;
						if(verbose)
							printf("\nL2 cache hit\n");
                        cache_write_hit_lru(tag_index_offset_l2[0],arr_l2[1],cache_mem_hier[cache_level]);

            //Update back in L1
						if(verbose)
							printf("\nUpdate from L2 back in L1 cache\n");
                        setno = Update_cache(tag_index_offset_l1[0],tag_index_offset_l1[1],cache_mem_hier[cache_level-1],cache_mem_hier);

            //Write operation in L1
						if(verbose)
							printf("\nWrite operation in L1\n");
                        cache_write_hit(tag_index_offset_l1[0],setno,cache_mem_hier[cache_level-1]);
                        cache_write_hit_lru(tag_index_offset_l1[0],setno,cache_mem_hier[cache_level-1]);
						
						if(verbose)
							printf("\nL1 LRU update\n");
                    }

                    else
                    {
                    //L2 cache miss
                    //Update L2 from memory
                        per_params.num_L2_read_misses += 1;
						if(verbose)
							printf("\nL2 cache miss\n");
                        per_params.total_mem_traffic += 1;
						if(verbose)
							printf("\nUpdate L2 cache from memory\n");
                        int dummy = Update_cache(tag_index_offset_l2[0],tag_index_offset_l2[1],cache_mem_hier[cache_level],cache_mem_hier);

                        if(verbose)
							printf("\nUpdate from L2 back in L1 cache\n");
                        setno = Update_cache(tag_index_offset_l1[0],tag_index_offset_l1[1],cache_mem_hier[cache_level-1],cache_mem_hier);

                    //Write operation in L1
						if(verbose)
							printf("\nWrite operation in L1\n");
                        cache_write_hit(tag_index_offset_l1[0],setno,cache_mem_hier[cache_level-1]);

                        cache_write_hit_lru(tag_index_offset_l1[0],setno,cache_mem_hier[cache_level-1]);
						if(verbose)
							printf("\nL1 LRU update\n");
                    }



                }

                else
                {
                    //perform update from l2 to l1;
                    cache_level+=1;
                    cache_mem_hier[cache_level].find_tag_index_blockoffset(addr,tag_index_offset_l2);
                    cache_mem_hier[cache_level].hit_or_miss(tag_index_offset_l2[0],arr_l2);


                    if(arr_l2[0])
                    {
            //L2 cache hit;
						if(verbose)
							printf("\nL2 cache hit\n");
                        cache_write_hit_lru(tag_index_offset_l2[0],arr_l2[1],cache_mem_hier[cache_level]);

            //Update back in L1
						if(verbose)
							printf("\nUpdate from L2 back in L1 cache\n");
                        setno = Update_cache(tag_index_offset_l1[0],tag_index_offset_l1[1],cache_mem_hier[cache_level-1],cache_mem_hier);

            //Write operation in L1
						if(verbose)
							printf("\nWrite operation in L1\n");
                        cache_write_hit(tag_index_offset_l1[0],setno,cache_mem_hier[cache_level-1]);
                        cache_write_hit_lru(tag_index_offset_l1[0],setno,cache_mem_hier[cache_level-1]);
						
						if(verbose)
							printf("\nL1 LRU update\n");
                    }

                    else
                    {
                    //L2 cache miss
                    //Update L2 from memory
                        per_params.num_L2_read_misses += 1;
						
						if(verbose)
							printf("\nL2 cache miss\n");
                        per_params.total_mem_traffic += 1;
						
						if(verbose)
							printf("\nUpdate L2 cache from memory\n");
                        int dummy = Update_cache(tag_index_offset_l2[0],tag_index_offset_l2[1],cache_mem_hier[cache_level],cache_mem_hier);
						
						if(verbose)
							printf("\nUpdate from L2 back in L1 cache\n");
                        setno = Update_cache(tag_index_offset_l1[0],tag_index_offset_l1[1],cache_mem_hier[cache_level-1],cache_mem_hier);

                    //Write operation in L1
						if(verbose)
							printf("\nWrite operation in L1\n");
                        cache_write_hit(tag_index_offset_l1[0],setno,cache_mem_hier[cache_level-1]);

                        cache_write_hit_lru(tag_index_offset_l1[0],setno,cache_mem_hier[cache_level-1]);
						
						if(verbose)
							printf("\nL1 LRU update\n");
                    }


                }

                //ENDS

                
            
            }

        else
            {
			if(verbose)
				printf("\nUpdate L1 cache from memory\n");
            per_params.total_mem_traffic += 1;
            setno = Update_cache(tag_index_offset_l1[0],tag_index_offset_l1[1],cache_mem_hier[cache_level],cache_mem_hier);
			
			if(verbose)
				printf("\nWrite operation in L1\n");
            cache_write_hit(tag_index_offset_l1[0],setno,cache_mem_hier[cache_level]);

            //ADDED sep 16
            cache_write_hit_lru(tag_index_offset_l1[0],setno,cache_mem_hier[cache_level]);
			if(verbose)
				printf("\nL1 LRU update\n");
            //ends

            }
        }
    
}



/* ------------------------------------------TESTIGNG ENDS------------------------------------*/


int main (int argc, char* argv[])
{
    FILE *FP;               // File handler
    char *trace_file;       // Variable that holds trace file name;
    cache_params params;    // look at sim_cache.h header file for the the definition of struct cache_params
    char rw;                // variable holds read/write type read from input file. The array size is 2 because it holds 'r' or 'w' and '\0'. Make sure to adapt in future projects
    unsigned long int addr; // Variable holds the address read from input file

    if(argc != 8)           // Checks if correct number of inputs have been given. Throw error and exit if wrong
    {
        printf("Error: Expected inputs:7 Given inputs:%d\n", argc-1);
        exit(EXIT_FAILURE);
    }
    
    // strtoul() converts char* to unsigned long. It is included in <stdlib.h>
    params.block_size       = strtoul(argv[1], NULL, 10);
    params.l1_size          = strtoul(argv[2], NULL, 10);
    params.l1_assoc         = strtoul(argv[3], NULL, 10);
    params.vc_num_blocks    = strtoul(argv[4], NULL, 10);
    params.l2_size          = strtoul(argv[5], NULL, 10);
    params.l2_assoc         = strtoul(argv[6], NULL, 10);
    trace_file              = argv[7];

    

    // Open trace_file in read mode
    FP = fopen(trace_file, "r");

    if(FP == NULL)
    {
        // Throw error and exit if fopen() failed
        printf("Error: Unable to open file %s\n", trace_file);
        exit(EXIT_FAILURE);
    }
    
    // Print params
    printf("  ===== Simulator configuration =====\n"
            "  BLOCKSIZE:                        %lu\n"
            "  L1_SIZE:                          %lu\n"
            "  L1_ASSOC:                         %lu\n"
            "  VC_NUM_BLOCKS:                    %lu\n"
            "  L2_SIZE:                          %lu\n"
            "  L2_ASSOC:                         %lu\n"
            "  trace_file:                       %s\n"
            , params.block_size, params.l1_size, params.l1_assoc, params.vc_num_blocks, params.l2_size, params.l2_assoc, trace_file);

    char str[2];
    int num_index = 0;
    std::vector<cache> cache_hier;
    cache l1_cache(params.l1_assoc,params.l1_size,params.l1_assoc,params.block_size,1);
    cache_hier.push_back(l1_cache);

    
    
    if(params.vc_num_blocks!=0)
    {
        //Enable VC
        int cache_size = (params.vc_num_blocks) * (params.block_size);

        vc = new cache(params.vc_num_blocks,cache_size,params.vc_num_blocks,params.block_size,-1);
        is_vc_enabled = true;
    }

    if(params.l2_size != 0)
    {
        is_l2 = true;
        cache l2_cache(params.l2_assoc,params.l2_size,params.l2_assoc,params.block_size,2);
        cache_hier.push_back(l2_cache);
    }
    
    while(fscanf(FP, "%s %lx", str, &addr) != EOF)
    {
        num_index ++;
        rw = str[0];
        
        if (rw == 'r')
        {
			if(verbose)
				printf("# %d: %s %lx\n",num_index, "read", addr);           // Print and test if file is read correctly
            int setno = cache_read(cache_hier,addr);
			if(verbose)
				printf("\nSet no: %d\n",setno);
        }
            
        else if (rw == 'w')
        {
			if(verbose)
				printf("# %d: %s %lx\n",num_index, "write", addr);          // Print and test if file is read correctly
            cache_write(cache_hier,addr);
            
        }
		
		if(verbose)
			printf("\n");
    // print_set_contents(cache_hier[0],9);
    // printf("\n");
    // printf("\n_________________________\n");
    // print_set_contents(cache_hier[1],169);
    // printf("\n");
    // print_set_contents(cache_hier[1],185);
    // printf("\n");
    // print_set_contents(cache_hier[1],201);
    // printf("\n");
	
	if(verbose)
	{
		
    printf("\n");
    print_contents(cache_hier[0]);
    printf("\n");
	if(params.vc_num_blocks !=0)
        print_contents_vc(vc);
    printf("\n");
    if(params.l2_size != 0)
		print_contents(cache_hier[1]);
    printf("\n");
    
    printf("----------------------------------------\n");
    }
	
    }

    printf("\n");
	printf(" ===== L1 contents =====\n");
    print_contents(cache_hier[0]);
    printf("\n");
	
	
	if(params.vc_num_blocks !=0)
	{
		
        printf(" ===== VC contents =====\n");
		print_contents_vc(vc);
		printf("\n");
	}
	
	//printf("\n");
	
    if(params.l2_size != 0)
	{
		printf(" ===== L2 contents =====\n");
		print_contents(cache_hier[1]);
		printf("\n");
	}
	
    //printf("\n");

   

    
    if(params.vc_num_blocks!=0)
    {
        per_params.swap_request_rate = (float)(per_params.num_swap_requests)/(float)(per_params.num_L1_reads + per_params.num_L1_writes);
    }

    per_params.combined_L1_VC_miss_rate = (float)(per_params.num_L1_read_misses + per_params.num_L1_write_misses - per_params.num_swaps) / (float)(per_params.num_L1_reads + per_params.num_L1_writes);
    
    if(params.l2_size!=0)
    {
        per_params.num_L2_miss_rate = (float)(per_params.num_L2_read_misses)/(float)(per_params.num_L2_reads);
    }

    //printf("\n");
    printf("===== Simulation results =====");
    printf("\n a. number of L1 reads: \t%d",per_params.num_L1_reads);
    printf("\n b. number of L1 read misses: \t%d",per_params.num_L1_read_misses);
    printf("\n c. number of L1 writes: \t%d",per_params.num_L1_writes);
    printf("\n d. number of L1 write misses: \t%d",per_params.num_L1_write_misses);
    printf("\n e. number of swap requests: \t%d",per_params.num_swap_requests);
    printf("\n f. swap request rate: \t%.4f",per_params.swap_request_rate);
    printf("\n g. number of swaps:  \t%d",per_params.num_swaps);
    printf("\n h. combined L1+VC miss rate: \t%.4f",per_params.combined_L1_VC_miss_rate);
    printf("\n i. number writebacks from L1/VC: \t%d",per_params.num_writebacks_from_L1_or_VC);
    printf("\n j. number of L2 reads: \t%d",per_params.num_L2_reads);
    printf("\n k. number of L2 read misses: \t%d",per_params.num_L2_read_misses);
    printf("\n l. number of L2 writes: \t%d",per_params.num_L2_writes);
    printf("\n m. number of L2 write misses: \t%d",per_params.num_L2_write_misses);
    printf("\n n. L2 miss rate: \t%.4f",per_params.num_L2_miss_rate);
    printf("\n o. number of writebacks from L2: \t%d",per_params.num_writebacks_from_L2);
    printf("\n p. total memory traffic: \t%d",per_params.total_mem_traffic);
	//printf("\n");
    return 0;
}
