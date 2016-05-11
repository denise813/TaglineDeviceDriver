///////////////////////////////////////////////////////////////////////////////
//
//  File           : tagline_driver.c
//  Description    : This is the implementation of the driver interface
//                   between the OS and the low-level hardware.
//
//  Author         : Dhruva Seelin
//  Created        : 11/27/15

// Include Files
#include <stdlib.h>
#include <string.h>
#include <cmpsc311_log.h>

// Project Includes
#include "raid_bus.h"
#include "tagline_driver.h"
#include "raid_cache.h"

//temptemptempkdfks

// Global Variables
int diskNum = 0;
int diskBlockNum = 0;
int **diskArray_ptr;
int **diskBlockArray_ptr;
// number of blocks written on specific disk
int numOfBlocksArray[9];
int cacheInsert = 0;
int cacheGet = 0;
int cacheHit = 0;
int cacheMiss = 0;

//
// Functions

////////////////////////////////////////////////////////////////////////////////
//
// Function     : tagline_driver_init
// Description  : Initialize the driver with a number of maximum lines to process
//
// Inputs       : maxlines - the maximum number of tag lines in the system
// Outputs      : 0 if successful, -1 if failure

int tagline_driver_init(uint32_t maxlines) {
	RAIDOpCode raidOpCode;
	RAIDOpCode returnOpCode;
	int i, j;
	uint8_t temp;

	// initialize 2d Array pointers
	diskArray_ptr = (int **) malloc(sizeof(int*)* (int) maxlines);
	for(j = 0; j < maxlines; j++) {
		diskArray_ptr[j] = (int *) malloc(sizeof(int)*MAX_TAGLINE_BLOCK_NUMBER);
	}

	diskBlockArray_ptr = (int **) malloc(sizeof(int*)* (int) maxlines);
	for(j = 0; j < maxlines; j++) {
		diskBlockArray_ptr[j] = (int *) malloc(sizeof(int)*MAX_TAGLINE_BLOCK_NUMBER);
	}
	
	// fills both arrays with -1 as default
	for(i = 0; i < maxlines; i++) {
		for(j = 0; j < MAX_TAGLINE_BLOCK_NUMBER; j++) {
			diskBlockArray_ptr[i][j] = -1;
			diskArray_ptr[i][j] = -1;
		}
	}
	
	
	// initialize driver
	temp = (uint8_t) (RAID_DISKBLOCKS / RAID_TRACK_BLOCKS);
	raidOpCode = create_raid_request(RAID_INIT, temp, RAID_DISKS, (RAIDBlockID) 0);
	returnOpCode = client_raid_bus_request(raidOpCode, NULL);
	
	extract_raid_response(raidOpCode, returnOpCode); 	
	
	// format each disk
	for(i = 0; i < RAID_DISKS; i++) {
		raidOpCode = create_raid_request(RAID_FORMAT, 0, i, 0);
        	returnOpCode = client_raid_bus_request(raidOpCode, NULL);
		
        	//extract raid opcode
        	extract_raid_response(raidOpCode, returnOpCode);
	}
	
	// initliaze cache
	init_raid_cache(TAGLINE_CACHE_SIZE);
	
	logMessage(LOG_INFO_LEVEL, "CACHE: initialized storage (maxsize = %u", TAGLINE_CACHE_SIZE);	

	// Return successfully
	logMessage(LOG_INFO_LEVEL, "TAGLINE: initialized storage (maxline=%u)", maxlines);
	return(0);
}

////////////////////////////////////////////////////////////////////////////////
//
// Function     : tagline_read
// Description  : Read a number of blocks from the tagline driver
//
// Inputs       : tag - the number of the tagline to read from
//                bnum -<F3> the starting block to read from
//                blks - the number of blocks to read
//                bug - memory block to read the blocks into
// Outputs      : 0 if successful, -1 if failure

int tagline_read(TagLineNumber tag, TagLineBlockNumber bnum, uint8_t blks, char *buf) {
	RAIDOpCode raidOpCode;
	RAIDOpCode returnOpCode;
	int i, diskLocation, diskBlockLocation;
	char *cacheBuf;

	// reads one block at a time
	for(i = 0; i < blks; i++) {
		
		// retreive disk and diskBlock location(number) of current block
		diskLocation = diskArray_ptr[(int)tag][i+bnum];
		diskBlockLocation = diskBlockArray_ptr[(int)tag][i+bnum];

		cacheBuf = get_raid_cache((RAIDDiskID) diskLocation, (RAIDBlockID) diskBlockLocation);

		// if block is not in cache
		if(cacheBuf == NULL) {
			raidOpCode = create_raid_request(RAID_READ, 1, (RAIDDiskID) diskLocation, (RAIDBlockID) diskBlockLocation);
        		returnOpCode = client_raid_bus_request(raidOpCode, buf+i*RAID_BLOCK_SIZE);
			extract_raid_response(raidOpCode, returnOpCode);

			logMessage(LOG_INFO_LEVEL, "TAGLINE : read %u blocks from tagline %u, starting block %u.", blks, tag, bnum);
		}
		// if block is in cache
		else {
			memcpy(buf, cacheBuf, RAID_BLOCK_SIZE);
			//logMessage(LOG_INFO_LEVEL, "Cache : read block %d from cache.", i);
		}
	}

	// Return successfully
	return(0);
}
////////////////////////////////////////////////////////////////////////////////
//
// Function     : tagline_write
// Description  : Write a number of blocks from the tagline driver
//
// Inputs       : tag - the number of the tagline to write from
//                bnum - the starting block to write from
//                blks - the number of blocks to write
//                bug - the place to write the blocks into
// Outputs      : 0 if successful, -1 if failure

int tagline_write(TagLineNumber tag, TagLineBlockNumber bnum, uint8_t blks, char *buf) {
	RAIDOpCode raidOpCode;
	RAIDOpCode returnOpCode;
	int i, diskLocation, diskBlockLocation;
		for(i = 0; i < blks; i++) {	
			if(diskArray_ptr[(int)tag][i+bnum] == -1) {
				// write blocks to disk
				raidOpCode = create_raid_request(RAID_WRITE, 1, (RAIDDiskID) diskNum, (RAIDBlockID) diskBlockNum);
				returnOpCode = client_raid_bus_request(raidOpCode, buf+i*RAID_BLOCK_SIZE);
				extract_raid_response(raidOpCode, returnOpCode);

				// write to cache
				put_raid_cache((RAIDDiskID) diskNum, (RAIDBlockID) diskBlockNum, buf);
				cacheInsert++;
		
				// set disk and block value to block for given tagline and block #
				diskArray_ptr[(int)tag][i+bnum] = diskNum;
				diskBlockArray_ptr[(int)tag][i+bnum] = diskBlockNum;

				numOfBlocksArray[diskNum] += 1;

				// write blocks to backup Disk
				raidOpCode = create_raid_request(RAID_WRITE, 1, (RAIDDiskID) diskNum+1, (RAIDBlockID) diskBlockNum);
        	       		returnOpCode = client_raid_bus_request(raidOpCode, buf+i*RAID_BLOCK_SIZE);
       		         	extract_raid_response(raidOpCode, returnOpCode);

				// write to cache
				put_raid_cache((RAIDDiskID) diskNum+1, (RAIDBlockID) diskBlockNum, buf);
				cacheInsert++;
				
				numOfBlocksArray[diskNum+1] += 1;
		
				// if disk is full, put next block at the start of next available disk
				if(diskBlockNum + 1 >= RAID_DISKBLOCKS) {
					diskNum += 2;
					diskBlockNum = 0;
				}		
				else {
					// increment disk block array pointer
                        		diskBlockNum += 1;
				}
			}

			// Overwrite
			else {
				diskLocation = diskArray_ptr[(int)tag][i+bnum];
		                diskBlockLocation = diskBlockArray_ptr[(int)tag][i+bnum];
				
        	        	raidOpCode = create_raid_request(RAID_WRITE, 1, (RAIDDiskID) diskLocation, (RAIDBlockID) diskBlockLocation);
                		returnOpCode = client_raid_bus_request(raidOpCode, buf+i*RAID_BLOCK_SIZE);
        		        extract_raid_response(raidOpCode, returnOpCode);
				
				// write to cache
				put_raid_cache((RAIDDiskID) diskLocation, (RAIDBlockID) diskBlockLocation, buf);
                                cacheInsert++;

				raidOpCode = create_raid_request(RAID_WRITE, 1, (RAIDDiskID) diskLocation+1, (RAIDBlockID) diskBlockLocation);
                                returnOpCode = client_raid_bus_request(raidOpCode, buf+i*RAID_BLOCK_SIZE);
                                extract_raid_response(raidOpCode, returnOpCode);
				
				// write to cache
				put_raid_cache((RAIDDiskID) diskLocation+1, (RAIDBlockID) diskBlockLocation, buf);
                                cacheInsert++;
			}

		}	
	//successfully
	logMessage(LOG_INFO_LEVEL, "TAGLINE : wrote %u blocks to tagline %u, starting block %u.",
			blks, tag, bnum);
	return(0);
}


// FUNCTION: Create_raid_request
// input: request type, number of blocks, Raid disk ID, Raid block id
// output: raid op code
RAIDOpCode create_raid_request(RAID_REQUEST_TYPES requestType, uint8_t blks, RAIDDiskID raidDiskNum, RAIDBlockID raidBlockNum) {
	RAIDOpCode raidOpCode;
	raidOpCode = (uint64_t) raidBlockNum;
	raidOpCode = raidOpCode | ((uint64_t) raidDiskNum << 40);
	raidOpCode = raidOpCode | ((uint64_t) blks << 48);
	raidOpCode = raidOpCode | ((uint64_t) requestType << 56);
	return raidOpCode;
}

// FUNCTION: extract_raid_response
// input: Raid op code 1, raid opcode 2
// ouput: true or false
int extract_raid_response(RAIDOpCode resp, RAIDOpCode resp2) {
	if (resp == resp2) {
		return(0);
	}
	else {
		return(1);
	}
}

// FUNCTION: raid_disk_signal
// Input: void
// Output: 0
// This is called to check the disk's status and check for failed disks
int raid_disk_signal(void) {
	RAIDOpCode raidOpCode;
        RAIDOpCode returnOpCode;
	uint32_t diskStatus;
	int i, j;
	char *bufHolder;

	char *buffer = malloc(sizeof(char*) *TAGLINE_BLOCK_SIZE);	
	
	// check all disks for failure
	for(i = 0; i < (int)RAID_DISKS; i++) {
		raidOpCode = create_raid_request(RAID_STATUS, 0, (RAIDDiskID) i, (RAIDBlockID) 0);
        	returnOpCode = client_raid_bus_request(raidOpCode, NULL);
		extract_raid_response(raidOpCode, returnOpCode);
		
		// find disk with failed status
		diskStatus = returnOpCode & 3;
		if(diskStatus == RAID_DISK_FAILED) {
			raidOpCode = create_raid_request(RAID_FORMAT, 0, (RAIDDiskID) i, 0);
	                returnOpCode = client_raid_bus_request(raidOpCode, NULL);

        	        //extract raid opcode
                	extract_raid_response(raidOpCode, returnOpCode);

			// For all even disks
			if(i % 2 == 0) {
				// replace numOfBlks with RAID_DISKBLOCKS
				for(j = 0; j < numOfBlocksArray[i]; j++) {
					// put in cache
					bufHolder = get_raid_cache((RAIDDiskID) i+1, (RAIDBlockID) j);
					
					// If block is not in cache	
					if(bufHolder == NULL) {
						raidOpCode = create_raid_request(RAID_READ, 1, (RAIDDiskID) i+1, (RAIDBlockID) j);
						returnOpCode = client_raid_bus_request(raidOpCode, buffer);
        	                                extract_raid_response(raidOpCode, returnOpCode);

						raidOpCode = create_raid_request(RAID_WRITE, 1, (RAIDDiskID) i, (RAIDBlockID) j);
                	                        returnOpCode = client_raid_bus_request(raidOpCode, buffer);
		                                extract_raid_response(raidOpCode, returnOpCode);
						// update cache
						put_raid_cache((RAIDDiskID) i, (RAIDBlockID) j, buffer);
					}
					// if block is already in cache
					else {
						memcpy(buffer, bufHolder, RAID_BLOCK_SIZE);
						
					}
				}
			}
			// For all odd disks
			else {
				for(j = 0; j < numOfBlocksArray[i]; j++) {				
					// put in cache
                                        bufHolder = get_raid_cache((RAIDDiskID) i+1, (RAIDBlockID) j);

					// if block is not in cache
					if(bufHolder == NULL) {
						raidOpCode = create_raid_request(RAID_READ, 1, (RAIDDiskID) i-1, (RAIDBlockID) j);
                                        	returnOpCode = client_raid_bus_request(raidOpCode, buffer);
                                        	extract_raid_response(raidOpCode, returnOpCode);
						
						raidOpCode = create_raid_request(RAID_WRITE, 1, (RAIDDiskID) i, (RAIDBlockID) j);
                                	        returnOpCode = client_raid_bus_request(raidOpCode, buffer);
                                	        extract_raid_response(raidOpCode, returnOpCode);
						
						// update cache
                                                put_raid_cache((RAIDDiskID) i, (RAIDBlockID) j, buffer);
					}
					// if block is already in cache
					else {
						memcpy(buffer, bufHolder, RAID_BLOCK_SIZE);
					}
					
				}
			}
		}
	}
	
	return (0);
}


// Function: Close
// input: 
// output:
// Function closes raid cache and free's disk and diskblock arrays
int tagline_close() {
	free(diskArray_ptr);
	free(diskBlockArray_ptr);
	close_raid_cache();
        return(0);
}

