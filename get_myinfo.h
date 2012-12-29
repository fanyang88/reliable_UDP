#ifndef	__header_h
#define	__header_h

#include	"unp.h"
#include	<net/if.h>


struct myinfo {
 // int               udpfd;
  struct sockaddr  *maskaddr;	  
  char             subnetaddr[40];	
  struct sockaddr  *ipaddr;		/* primary address */
  struct myinfo    *next;		/* next of these structures */
};

struct   myinfo	*get_myinfo();
struct   myinfo	*Get_myinfo();
void   free_myinfo(struct myinfo *);

#endif
