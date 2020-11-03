#include "common.h"
#include <stdlib.h>
#include "burst.h"
#include "memory/cache.h"

void ddr3_read_public(hwaddr_t addr, void *data);
void ddr3_write_public(hwaddr_t addr, void *data, uint8_t *mask);
void dram_write(hwaddr_t addr, size_t len, uint32_t data);

void addMemoryTime(uint32_t t) {
    MEMORY_TIME += t;
}

void resetCache() {
    int i = 0;
    MEMORY_TIME = 0;
    for ( i = 0; i < L1_CACHE_SET_SIZE * L1_CACHE_WAY_SIZE; i++)
    {
        l1_cache[i].valid = false;
    }

    for ( i = 0; i < L2_CACHE_SET_SIZE * L2_CACHE_WAY_SIZE ; i++)
    {
        l2_cache[i].valid = false;
        l2_cache[i].dirty = false;
    }
    
    
}

int readCache(hwaddr_t addr) {
    uint32_t tag = addr >> (L1_CACHE_BLOCK_SIZE_BIT + L1_CACHE_SET_BIT);
    uint32_t set = (addr >> L1_CACHE_BLOCK_SIZE_BIT) & (L1_CACHE_SET_SIZE - 1);
    int i, j;
    for ( i = L1_CACHE_WAY_SIZE*set; i < L1_CACHE_WAY_SIZE*(set+1); i++)
    {
        if (l1_cache[i].tag == tag && l1_cache[i].valid) return i;
    }

    j = readCache2(addr);
    srand(i);
    i = L1_CACHE_WAY_SIZE * set + rand()%L1_CACHE_WAY_SIZE;
    memcpy(l1_cache[i].data, l2_cache[j].data, L1_CACHE_BLOCK_SIZE);
    l1_cache[i].valid = true;
    l1_cache[i].tag = tag;
    return i;
}

void writeCache(hwaddr_t addr, size_t len, uint32_t data) {
    uint32_t tag = addr>>(L1_CACHE_BLOCK_SIZE_BIT + L1_CACHE_SET_BIT);
    uint32_t set = (addr>>L1_CACHE_BLOCK_SIZE_BIT) & (L1_CACHE_SET_SIZE - 1);
    uint32_t offset = addr & (L1_CACHE_BLOCK_SIZE - 1);

    int i;
    for( i = L1_CACHE_WAY_SIZE*set; i < L1_CACHE_WAY_SIZE*(set+1); i++)
    {
        if (l1_cache[i].tag == tag && l1_cache[i].valid)
        {
            if (offset + len > L1_CACHE_BLOCK_SIZE)
            {
                dram_write(addr, L1_CACHE_BLOCK_SIZE - offset, data);
                memcpy(l1_cache[i].data+offset, &data, L1_CACHE_BLOCK_SIZE - offset);
                writeCache2(addr, L1_CACHE_BLOCK_SIZE - offset, data);
                writeCache(addr+L1_CACHE_BLOCK_SIZE - offset, len - L1_CACHE_BLOCK_SIZE + offset, data>>(L1_CACHE_BLOCK_SIZE - offset));
            }
            else
            {
                dram_write(addr, len, data);
                memcpy(l1_cache[i].data + offset, &data, len);
                writeCache2(addr, len, data);
            }
            
            return;
            
        }
        
    }
    writeCache2(addr, len, data);
    
}

int readCache2(hwaddr_t addr) {
    uint32_t tag = addr >> (L2_CACHE_BLOCK_SIZE_BIT + L2_CACHE_SET_BIT);
    uint32_t set = (addr>>L2_CACHE_BLOCK_SIZE_BIT) & (L2_CACHE_SET_SIZE - 1);
    uint32_t block = (addr>>L2_CACHE_BLOCK_SIZE_BIT) << L2_CACHE_BLOCK_SIZE_BIT;
    int i, j;
    for ( i = L2_CACHE_WAY_SIZE*set; i < L2_CACHE_WAY_SIZE*(set+1); i++)
    {
        if (l2_cache[i].tag == tag && l2_cache[i].valid) return i;
    }
    srand(i);
    i = L2_CACHE_WAY_SIZE * set + rand()%L2_CACHE_WAY_SIZE;
    if (l2_cache[i].dirty && l2_cache[i].valid)
    {
        uint8_t mask[BURST_LEN * 2];
        uint32_t block2 = (l2_cache[i].tag << (L2_CACHE_BLOCK_SIZE_BIT + L2_CACHE_SET_BIT)) | (set << L2_CACHE_BLOCK_SIZE_BIT);
        memset(mask, 1, BURST_LEN*2);
        for ( j = 0; j < L2_CACHE_BLOCK_SIZE; j++)
        {
            ddr3_write_public(block2 + j*BURST_LEN, l2_cache[i].data + j*BURST_LEN, mask);
        } 
    }

    for ( j = 0; j < L2_CACHE_BLOCK_SIZE/BURST_LEN; j++)    
    {
        ddr3_read_public(block + j*BURST_LEN, l2_cache[i].data + j * BURST_LEN);
    }

    l2_cache[i].valid = true;
    l2_cache[i].tag = tag;
    l2_cache[i].dirty = false;
    return i;
}

void writeCache2(hwaddr_t addr, size_t len, uint32_t data) {
    uint32_t tag = addr>>(L2_CACHE_BLOCK_SIZE_BIT + L2_CACHE_SET_BIT);
    uint32_t set = (addr>>L2_CACHE_BLOCK_SIZE_BIT) & (L2_CACHE_SET_SIZE - 1);
    uint32_t offset = addr & (L2_CACHE_BLOCK_SIZE - 1);
    int i;
    for ( i = L2_CACHE_WAY_SIZE*set; i < L2_CACHE_WAY_SIZE*(set+1); i++) {
        if (l2_cache[i].tag == tag && l2_cache[i].valid)
        {
            l2_cache[i].dirty = true;
            if (offset + len > L2_CACHE_WAY_SIZE)
            {
                memcpy(l2_cache[i].data+offset, &data, L2_CACHE_BLOCK_SIZE - offset);
                writeCache2(addr+L2_CACHE_BLOCK_SIZE - offset, len - L2_CACHE_BLOCK_SIZE + offset, data>>(L2_CACHE_BLOCK_SIZE - offset));
            } else
            {
                memcpy(l2_cache[i].data+offset, &data, len);
            }
            
            return;
        }
        
    }
    i = readCache2(addr);
    l2_cache[i].dirty = true;
    memcpy(l2_cache[i].data + offset, &data, len);
    
}