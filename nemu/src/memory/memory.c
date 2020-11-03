#include "common.h"
#include "memory/cache.h"
#include "burst.h"

uint32_t dram_read(hwaddr_t, size_t);
void dram_write(hwaddr_t, size_t, uint32_t);

/* Memory accessing interfaces */

uint32_t hwaddr_read(hwaddr_t addr, size_t len) {
	int first_id = readCache(addr);
	uint32_t offset = addr&(L1_CACHE_BLOCK_SIZE - 1);
	uint8_t temp[2*BURST_LEN];

	if (offset + len > L1_CACHE_BLOCK_SIZE)
	{
		int second_id = readCache(addr + L1_CACHE_BLOCK_SIZE - offset);
		memcpy(temp, l1_cache[first_id].data + offset, L1_CACHE_BLOCK_SIZE - offset);
		memcpy(temp + L1_CACHE_BLOCK_SIZE - offset, l1_cache[second_id].data, len - L1_CACHE_BLOCK_SIZE + offset);
	} else
	{
		memcpy(temp, l1_cache[first_id].data + offset, len);
	}

	int zero = 0;
	uint32_t tmp = unalign_rw(temp+zero, 4) & (~0u >> ((4 - len) << 3));
	
	return tmp;
}

void hwaddr_write(hwaddr_t addr, size_t len, uint32_t data) {
	writeCache(addr, len, data);
}

uint32_t lnaddr_read(lnaddr_t addr, size_t len) {
	return hwaddr_read(addr, len);
}

void lnaddr_write(lnaddr_t addr, size_t len, uint32_t data) {
	hwaddr_write(addr, len, data);
}

uint32_t swaddr_read(swaddr_t addr, size_t len) {
#ifdef DEBUG
	assert(len == 1 || len == 2 || len == 4);
#endif
	return lnaddr_read(addr, len);
}

void swaddr_write(swaddr_t addr, size_t len, uint32_t data) {
#ifdef DEBUG
	assert(len == 1 || len == 2 || len == 4);
#endif
	lnaddr_write(addr, len, data);
}

