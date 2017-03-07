/*
 * Copyright (c) 2012-2017 YunShan Networks, Inc.
 *
 * Author Name: Zhang Pan<zhangpan@yunshan.net.cn>
 * Finish Date:
 *
 * Network Packet Craft lib used to craft any kind
 * of network packets in memroy due to (unit)test
 * requirements
 * Feel free to add new APIs which generating
 * malformed packet and carry out bit manuipulation
 * for your own sake
 *
 */

#ifndef __PKT_CRAFT_H__
#define __PKT_CRAFT_H__
#include <arpa/inet.h>
/*
 * Convert a address string to struct in_addr,
 * the string address may be host name or ip,
 * return 1 when success, if string is invalid or host name
 * can't be resolved, return 0
 */
int
addrconvert(char *address, struct in_addr *inaddr);

/*
 * Generate a TCP packet hdr.
 * Desired size of the packet is len, if actualLen is not NULL
 * actual size will be put in, return a pointer to the packet,
 * caller has duty to free it
 */
void *
gentcppackethdr(int srcPort, int destPort, int len, size_t *actualLen);

/*
 * Generate a UDP packet hdr,
 * desired size of the packet is len, if actualLen is not NULL,
 * actual size will be put in,
 * return a pointer to the packet,
 * caller has duty to free it
 */
void *
genudppackethdr(int srcPort, int destPort, int len, size_t *actualLen);

/*
 * Generate a ICMP ECHO (ping) packet hdr,
 * desired size of the packet is len, if actualLen is not NULL,
 * actual size will be put in,
 * return a pointer to the packet,
 * caller has duty to free it
 */
void *
genicmppackethdr(int len, size_t *actualLen);

/*
 * Generate a IP packet hdr,
 * data to put in the packet is content, and data size is contentLen,
 * sIP, dIP and proto will be filled in the IP header,
 * return a pointer to the packet,
 * caller has duty to free it
 */
void *
genippackethdr(void *content, size_t contentLen,
                  struct in_addr sIP, struct in_addr dIP, unsigned char proto);


#endif
