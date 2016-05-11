#ifndef TAGLINE_DRIVER_INCLUDED
#define TAGLINE_DRIVER_INCLUDED

////////////////////////////////////////////////////////////////////////////////
//
//  File           : tagline_driver.h
//  Description    : This is the header file for the driver interface between
//                   the OS and the low-level driver.
//
//  Author         : Patrick McDaniel
//  Created        : Mon Jun 15 07:51:29 EDT 2015
//

// Includes
#include "raid_bus.h"
#include "raid_cache.h"

// Project Includes
#define MAX_TAGLINE_BLOCK_NUMBER  256
#define TAGLINE_BLOCK_SIZE        RAID_BLOCK_SIZE
#define RAID_DISKS                9
#define RAID_DISKBLOCKS           4096

// Type definitions
typedef uint16_t TagLineNumber;
typedef uint32_t TagLineBlockNumber;

//
// Interface functions


int tagline_driver_init(uint32_t maxlines);
        // Initialize the driver with a number of maximum lines to process

int tagline_read(TagLineNumber tag, TagLineBlockNumber bnum, uint8_t blks, char *buf);
        // Read a number of blocks from the tagline driver

int tagline_write(TagLineNumber tag, TagLineBlockNumber bnum, uint8_t blks, char *buf);
        // Write a number of blocks from the tagline driver

int tagline_close(void);
        // Close the tagline interface

int raid_disk_signal(void);
        // A disk has failed which needs to be recovered

RAIDOpCode create_raid_request(RAID_REQUEST_TYPES requestType, uint8_t blks, RAIDDiskID raidDiskNum, RAIDBlockID raidBlockNum);
        // Create RAID Op Code

int extract_raid_response(RAIDOpCode resp, RAIDOpCode resp2);
        // Check if RAID Response matches

RAIDOpCode client_raid_bus_request(RAIDOpCode op, void *buf);

int init_raid_cache(uint32_t max_blocks);
#endif /* RAID_DRIVER_INCLUDED */
