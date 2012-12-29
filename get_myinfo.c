/* include get_ifi_info1 */
#include	"get_myinfo.h"

struct myinfo *
get_myinfo()
{
	struct myinfo		*ifi, *ifihead, **ifipnext;
	int					sockfd, len, lastlen, flags;
	char				*ptr, *buf, lastname[IFNAMSIZ], *cptr;
	struct ifconf		ifc;
	struct ifreq		*ifr, ifrcopy;
	struct sockaddr_in	*sinptr;  //, servaddr;
        char                     *maddr, *iaddr, *sbaddr;
        struct in_addr          tmpaddr, tmpaddr1, tmpaddr2;
       
	sockfd = Socket(AF_INET, SOCK_DGRAM, 0);
        
	lastlen = 0;
	len = 100 * sizeof(struct ifreq);	/* initial buffer size guess */
	for ( ; ; ) {
		buf = Malloc(len);
		ifc.ifc_len = len;
		ifc.ifc_buf = buf;
		if (ioctl(sockfd, SIOCGIFCONF, &ifc) < 0) {
			if (errno != EINVAL || lastlen != 0)
				err_sys("ioctl error");
		} else {
			if (ifc.ifc_len == lastlen)
				break;		/* success, len has not changed */
			lastlen = ifc.ifc_len;
		}
		len += 10 * sizeof(struct ifreq);	/* increment */
		free(buf);
	}
	ifihead = NULL;
	ifipnext = &ifihead;
	lastname[0] = 0;
        
	
/* include get_ifi_info2 */
	for (ptr = buf; ptr < buf + ifc.ifc_len; ) {
		ifr = (struct ifreq *) ptr;

#ifdef	HAVE_SOCKADDR_SA_LEN
		len = max(sizeof(struct sockaddr), ifr->ifr_addr.sa_len);
#else
		switch (ifr->ifr_addr.sa_family) {
#ifdef	IPV6
		case AF_INET6:	
			len = sizeof(struct sockaddr_in6);
			break;
#endif
		case AF_INET:	
		default:	
			len = sizeof(struct sockaddr);
			break;
		}
#endif	/* HAVE_SOCKADDR_SA_LEN */
		ptr += sizeof(ifr->ifr_name) + len;	/* for next one in buffer */



		if (ifr->ifr_addr.sa_family != AF_INET)
			continue;	/* ignore if not desired address family */
                if ( (cptr = strchr(ifr->ifr_name, ':')) != NULL)
			*cptr = 0;		/* replace colon with null */
		
		memcpy(lastname, ifr->ifr_name, IFNAMSIZ);

		ifrcopy = *ifr;
		Ioctl(sockfd, SIOCGIFFLAGS, &ifrcopy);
		flags = ifrcopy.ifr_flags;
		if ((flags & IFF_UP) == 0)
			continue;	/* ignore if interface not up */
/* end get_ifi_info2 */

/* include get_ifi_info3 */
		ifi = Calloc(1, sizeof(struct myinfo));
		*ifipnext = ifi;			/* prev points to this new one */
		ifipnext = &ifi->next;	/* pointer to next one goes here */
                sinptr = (struct sockaddr_in *) &ifr->ifr_addr;
		ifi->ipaddr = Calloc(1, sizeof(struct sockaddr_in));
		memcpy(ifi->ipaddr, sinptr, sizeof(struct sockaddr_in));

              ////////////////////bind socket
        /*  bzero(&servaddr,sizeof(servaddr));
          saddr= inet_ntoa(((struct sockaddr_in *) &ifr->ifr_addr)->sin_addr);
          ifi->udpfd = Socket(AF_INET, SOCK_DGRAM, 0);
          servaddr.sin_family= AF_INET;
          Inet_pton(AF_INET, saddr, &servaddr.sin_addr);
          servaddr.sin_port= htons(1234);
          Bind(ifi->udpfd, (SA *) &servaddr, sizeof(servaddr));*/
                   
////////////////////////////////////////////////////////

#ifdef	SIOCGIFNETMASK
	Ioctl(sockfd, SIOCGIFNETMASK, &ifrcopy);
	sinptr = (struct sockaddr_in *) &ifrcopy.ifr_addr;
	ifi->maskaddr = Calloc(1, sizeof(struct sockaddr_in));
	memcpy(ifi->maskaddr, sinptr, sizeof(struct sockaddr_in));
			
#endif

    //get subnet address
       tmpaddr.s_addr= NULL;
       iaddr= Sock_ntop_host(ifi->ipaddr, sizeof(*ifi->ipaddr));
       maddr=inet_ntoa(((struct sockaddr_in *) ifi->maskaddr)->sin_addr);
     
       inet_aton(iaddr, &tmpaddr1);   //string to in_addr
       inet_aton(maddr, &tmpaddr2);
       tmpaddr.s_addr=  tmpaddr1.s_addr & tmpaddr2.s_addr; 
       sbaddr= inet_ntoa(tmpaddr);
       strcpy(ifi->subnetaddr, sbaddr);
  
	}
	free(buf);
	return(ifihead);	/* pointer to first structure in linked list */
}
/* end get_ifi_info4 */

/* include free_myinfo */
void
free_myinfo(struct myinfo *ifihead)
{
	struct myinfo	*ifi, *ifinext;

	for (ifi = ifihead; ifi != NULL; ifi = ifinext) {
		if (ifi->ipaddr != NULL)
			free(ifi->ipaddr);
		if (ifi->maskaddr != NULL)
			free(ifi->maskaddr);
		if (ifi->subnetaddr != NULL)
			 *ifi->subnetaddr=NULL;
		ifinext = ifi->next;	/* can't fetch ifi_next after free() */
		free(ifi);					/* the ifi_info{} itself */
	}
}
/* end free_ifi_info */

struct myinfo *
Get_myinfo()
{
	struct myinfo	*ifi;

	if ( (ifi = get_myinfo()) == NULL)
		err_quit("get_myinfo error");
	return(ifi);
}
