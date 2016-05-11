////////////////////////////////////////////////////////////////////////////////
//
//  File          : raid_client.c
//  Description   : This is the client side of the RAID communication protocol.
//
//  Author        : Dhruva Seelin
//  Last Modified : 12/11/15
//

// Include Files
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>
#include <stdint.h>

// Project Include Files
#include <raid_network.h>
#include <cmpsc311_log.h>
#include <cmpsc311_util.h>

// Global data
unsigned char *raid_network_address = NULL; // Address of CRUD server
unsigned short raid_network_port = 0; // Port of CRUD server
char *ip = RAID_DEFAULT_IP;
int socket_fd;
uint64_t value;
uint64_t opCode;
uint64_t length;
struct sockaddr_in caddr;

//
// Functions

////////////////////////////////////////////////////////////////////////////////
//
// Function     : client_raid_bus_request
// Description  : This the client operation that sends a request to the RAID
//                server.   It will:
//
//                1) if INIT make a connection to the server
//                2) send any request to the server, returning results
//                3) if CLOSE, will close the connection
//
// Inputs       : op - the request opcode for the command
//                buf - the block to be read/written from (READ/WRITE)
// Outputs      : the response structure encoded as needed

RAIDOpCode client_raid_bus_request(RAIDOpCode op, void *buf) {
	uint64_t reverseOpCode;
	uint64_t reverseLength;
	
		
	// Hanshake
	if ((op >> 56) == RAID_INIT) {
		caddr.sin_family = AF_INET;
		caddr.sin_port = htons(RAID_DEFAULT_PORT);
		if ( inet_aton(ip, &caddr.sin_addr) == 0 ) {
			return( -1 );
		}
		socket_fd = socket(PF_INET, SOCK_STREAM, 0);
		if (socket_fd == -1) {
			printf( "Error on socket creation [%s]\n", strerror(errno) );
			return( -1 );
		}
		if ( connect(socket_fd, (const struct sockaddr *)&caddr, sizeof(caddr)) == -1 ) {
			printf( "Error on socket connect [%s]\n", strerror(errno) );
			return( -1 );
		} 
	}
		
	// WRITE
	reverseOpCode = htonll64(op);
 	if ( write( socket_fd, &reverseOpCode, sizeof(reverseOpCode)) != sizeof(reverseOpCode) ) {
		printf( "Error writing network data [%s]\n", strerror(errno) );
 		return( -1 );
 	}
	logMessage(LOG_INFO_LEVEL, "Sent a Op Code of [%d]\n", reverseOpCode); 
	
	if ((op >> 56) == RAID_WRITE) {
		reverseLength = htonll64(RAID_BLOCK_SIZE);
	}
	else {
		reverseLength = 0;
	}

	if ( write( socket_fd, &reverseLength, sizeof(reverseLength)) != sizeof(reverseLength) ) {
		printf( "Error writing network data [%s]\n", strerror(errno) );
		return( -1 );
	}
	logMessage(LOG_INFO_LEVEL, "Sent a length of [%d]\n", reverseLength); 
		
	if ((op >> 56) == RAID_WRITE) {
		if ( (write( socket_fd, buf, RAID_BLOCK_SIZE )) != RAID_BLOCK_SIZE) {
			printf( "Error writing network data [%s]\n", strerror(errno) );
			return( -1 );
               	}

	} 

	// READ
	if ( read( socket_fd, &opCode, sizeof(opCode)) != sizeof(opCode) ) {
		printf( "Error reading network data [%s]\n", strerror(errno) );
		return( -1 );
	}
	reverseOpCode = ntohll64(opCode);
	logMessage(LOG_INFO_LEVEL, "Received Op Code of [%d]\n", reverseOpCode); 

	if ( read( socket_fd, &length, sizeof(length)) != sizeof(length) ) {
		printf( "Error reading network data [%s]\n", strerror(errno) );
		return( -1 );
	}
	reverseLength = ntohll64(length);
 	logMessage(LOG_INFO_LEVEL, "Received a length of [%d]\n", reverseLength); 	
	
	if ( length != 0) {
		if ( read( socket_fd, buf, reverseLength) != reverseLength) {
			printf( "Error writing network data [%s]\n", strerror(errno) );
			return( -1 );
		}
	}

	// close socket
	if( (op >> 56) == RAID_CLOSE) {
		close(socket_fd);
		
	}

	
    return(reverseOpCode);
}
