////////////////////////////////////////////////////////////////////////////////
//
//  File           : raid_cache.c
//  Description    : This is the implementation of the cache for the TAGLINE
//                   driver.
//
//  Author         : Dhruva Seelin
//  Last Modified  : 11/30/15
//

// Includes
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>

// Project includes
#include <cmpsc311_log.h>
#include <cmpsc311_util.h>
#include <raid_cache.h>

// Gobal Variables
int *cacheDisk;
int *cahceDiskBlock;
int *cacheData;
int *cacheTime;
int sysTime = 0;
uint32_t cacheSize;

int cacheHit;
int cacheMiss;
int cacheInsert;
int cacheGet;

// Cache struct
struct CACHE {
	RAIDDiskID disk;
	RAIDBlockID diskBlock;
	void *data;
	int time;

}*cache;

//
// TAGLINE Cache interface

////////////////////////////////////////////////////////////////////////////////
//
// Function     : init_raid_cache
// Description  : Initialize the cache and note maximum blocks
//
// Inputs       : max_items - the maximum number of items your cache can hold
// Outputs      : 0 if successful, -1 if failure

int init_raid_cache(uint32_t max_items) {
	int i;
	cacheSize = max_items;
	
	cache = (struct CACHE*)malloc(sizeof(struct CACHE)*max_items);	

	for(i = 0; i < cacheSize; i++) {
		cache[i].data = malloc(RAID_BLOCK_SIZE);
		cache[i].time = -1;
	}	

	// Return successifully
	return(0);
}

////////////////////////////////////////////////////////////////////////////////
//
// Function     : close_raid_cache
// Description  : Clear all of the contents of the cache, cleanup
//
// Inputs       : none
// Outputs      : o if successful, -1 if failure

int close_raid_cache(void) {
	free(cache);
	double cacheEfficiency;

	cacheEfficiency = ((double) cacheHit / cacheGet) * 100;
	
	logMessage(LOG_INFO_LEVEL, "*** Cache Statistics***");
	logMessage(LOG_INFO_LEVEL, "Total cache inserts:\t%d", cacheInsert);
	logMessage(LOG_INFO_LEVEL, "Total cache gets: \t%d", cacheGet);
	logMessage(LOG_INFO_LEVEL, "Total cache hits: \t%d", cacheHit);
	logMessage(LOG_INFO_LEVEL, "Total cache misses: \t%d", cacheMiss);
	logMessage(LOG_INFO_LEVEL, "Cache Efficiency: \t%f", cacheEfficiency);

	// Return successfully
	return(0);
}

////////////////////////////////////////////////////////////////////////////////
//
// Function     : put_raid_cache
// Description  : Put an object into the block cache
//
// Inputs       : dsk - this is the disk number of the block to cache
//                blk - this is the block number of the block to cache
//                buf - the buffer to insert into the cache
// Outputs      : 0 if successful, -1 if failure

int put_raid_cache(RAIDDiskID dsk, RAIDBlockID blk, void *buf)  {
	int i, j;
	int inCache = 0;
	int minTime = cache[0].time;
	int cacheReplaceIndex;
	
	// already in cache
	for(i = 0; i < cacheSize; i++) {
		if(cache[i].disk == dsk && cache[i].diskBlock == blk) {
			memcpy(cache[i].data, buf, RAID_BLOCK_SIZE);
			cache[i].time = sysTime++;
			inCache = 1;
			cacheInsert++;
			logMessage(LOG_INFO_LEVEL,"Cache block %d updated at time %d", i, cache[i].time);
		}
	}
	// if not already in cache
	if(inCache == 0) {
		// search cache for lowest time
		for(j = 0; j < cacheSize; j++) {
			if(cache[j].time <= minTime) {
				minTime = cache[j].time;
				cacheReplaceIndex = j;
			}
		}
		// inject
		cache[cacheReplaceIndex].disk =  dsk;
		cache[cacheReplaceIndex].diskBlock = blk;
		memcpy(cache[cacheReplaceIndex].data, buf, RAID_BLOCK_SIZE);
		cache[cacheReplaceIndex].time = sysTime++;
		cacheInsert++;
		logMessage(LOG_INFO_LEVEL, "Cache block %d updated %d", cacheReplaceIndex, cache[cacheReplaceIndex].time);
	}

	// Return successfully
	return(0);
}

////////////////////////////////////////////////////////////////////////////////
//
// Function     : get_raid_cache
// Description  : Get an object from the cache (and return it)
//
// Inputs       : dsk - this is the disk number of the block to find
//                blk - this is the block number of the block to find
// Outputs      : pointer to cached object or NULL if not found

void * get_raid_cache(RAIDDiskID dsk, RAIDBlockID blk) {
	int i;
	void *cacheBlock;
	int temp = 0;
	
	for(i = 0; i < cacheSize; i++) {
		// if found block in cache
		if(cache[i].disk == dsk && cache[i].diskBlock == blk) {
			cacheBlock = cache[i].data;
			cache[i].time = sysTime++;
			logMessage(LOG_INFO_LEVEL, "CACHE: read cache block %d", i);
			temp = 1;
			cacheGet++;
			cacheHit++;
		}
		else {
			cacheBlock = NULL;
		}
	}

	if(temp == 0) {
		cacheMiss++;
		cacheGet++;
	}

	return(cacheBlock);
}

