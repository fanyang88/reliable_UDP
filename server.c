#include	"get_myinfo.h"
#include        <stdio.h>
#include	"para.h"
#include	"SockTb.h"
#include	<stdlib.h>
#include	"unprtt.h"
#include	<setjmp.h>
#include	<ctype.h>

#define TIMEOUTVAL	4
static sigjmp_buf jmpbuf;

static void 
sig_alrm(int signo)
{
	siglongjmp(jmpbuf, 1);
}

char *
trim(char *s)
{
 int i=0, j=strlen(s)-1, k=0;
 while(isspace(s[i]) && s[i]!='\0')
	i++;
while(isspace(s[j]) && s[j]!='\0')
	j--;
while(i<=j)
s[k++]=s[i++];
s[k]='\0';
return s;

}

// 1:	set state= GET_PACKET 
// 0:	set state= CLOSE_WAIT
//-1:	wrong opr in fetch data in Cricular buffer
int sendData(int sockfd, struct SockTb *socktb) {
	int			len,bread = 0, dread = 0, end, rv;
	int			sndsize =1;
        int			curwnd;
	struct sockaddr_in	sockaddrc;
	struct Packet		*pkt_p, *tp;  
	char     		*strpkt,run_buf[MSS], finstr[PSTRLEN];
	printf("[PROCESS] server keep sending data!\n");
	curwnd = getwnd(socktb);
	if ((socktb->snd_una*SMSS+curwnd) >= socktb->snd_nxt*SMSS) 
		sndsize= socktb->snd_una*SMSS + curwnd- socktb->snd_nxt*SMSS;
	
	printf("[PROCESS] circ buff has %d elems!\n", socktb->cb_send->m_uiTotEl);
	 
		bzero(&sockaddrc,sizeof(sockaddrc));
	  	sockaddrc.sin_family= AF_INET;
          	Inet_pton(AF_INET, socktb->sip, &sockaddrc.sin_addr);
	  	sockaddrc.sin_port= htons(socktb->sport);
	
	if(0 ==socktb->cb_send->m_uiTotEl && length(socktb->lostbuffer)==1)  
 //no more data left, send FIN to client
	{
		 	tp= initAck(tp);
			tp->seq = 0;
			tp->fin = 1;
			tp->ack_seq = 0;
			strncpy(finstr, PackettoString(tp), 13);
   			printf("[PROCESS] server send a FIN and wait to close!\n");
          		if( -1 >= (rv = sendto(sockfd, finstr, strlen(finstr), 0, (struct sockaddr *)&sockaddrc, sizeof(sockaddrc))))
			 perror("[PROCESS] server sending FIN error! ");

        		printf("[PROCESS] server side wait for close \n");
			socktb->sockstate= CLOSE_WAIT;
	  		socktb->runstate= PROCESS_GET_PKT;
		return 1;
	}

	   //calculate number of xmit for congestion avoidance 
	   int xmit_cnt = sndsize/SMSS;
	   if (xmit_cnt% 2 == 1) 
	   {
		if (sndsize% SMSS != 0)  xmit_cnt++;
		else               xmit_cnt--;
	   }

	while (socktb->cb_send->m_uiTotEl>0) 
            {
		
		if ((socktb->snd_una*SMSS+getwnd(socktb)) >= socktb->snd_nxt*SMSS)
		   sndsize=socktb->snd_una*SMSS + getwnd(socktb)- socktb->snd_nxt*SMSS;
		if (socktb->cc_state == CC_CA) //congestion avoidance
		{ if (xmit_cnt-- <= 0)	break;}

		else { if (SMSS > sndsize)   break;}

		memset(run_buf, '\0', MSS);
		len = getAt(socktb->cb_send, run_buf, MSS, end);
		if(len>0)
		   {
		      dread = removeBytes(socktb->cb_send, len);
		      if (len != dread)
			{ printf("[PROCESS] server remove data in cicular buffer error!\n");  return -1;}
		   }
		pkt_p= InitPacket(pkt_p, run_buf);	        
	        pkt_p->seq= socktb->snd_nxt;
		pkt_p->ack_seq= socktb->rcv_nxt;		
		strpkt= PackettoString(pkt_p);

		// sending
		printf("[PROCESS] server send a datagram with seq#: %d, ack_seq#: %d.\n", socktb->snd_nxt,socktb->rcv_nxt);
           if( -1 >= (rv = sendto(sockfd,strpkt, PSTRLEN, 0, (struct sockaddr *)&sockaddrc, sizeof(sockaddrc))))
	            perror("[PROCESS] server sending data to client error! ");
		socktb->snd_nxt += 1;// //snd_nxt move forward
		socktb->lostbuffer= add(socktb->lostbuffer, pkt_p);
		
		free(pkt_p);		 
	}//End of while
	return 1;
}


int run(int sockfd, struct SockTb *socktb) {
	int			rv, state;
	char   			recv_cnt[PSTRLEN];
	struct Packet      	*tp;
	struct sockaddr_in 	sockaddrs, sockaddrc;
	socklen_t		len;

	bzero(&sockaddrs, sizeof(sockaddrs));
        sockaddrs.sin_family = AF_INET;
	sockaddrs.sin_port = htons(socktb->dport); //dip=server
	inet_pton(AF_INET, socktb->dip, &sockaddrs.sin_addr); 
	
	bzero(&sockaddrs, sizeof(sockaddrc));
        sockaddrc.sin_family = AF_INET;
	sockaddrc.sin_port = htons(socktb->sport); //sip=client
	inet_pton(AF_INET, socktb->sip, &sockaddrc.sin_addr);
	
	state = socktb->runstate;
	while(state != PROCESS_END){ 
		switch(state){
			 // Get a new packet. client receive packet from server 
		  case PROCESS_GET_PKT:
		      rv = -1;
	              memset(recv_cnt, '\0', sizeof(recv_cnt));
                      len= sizeof(sockaddrs);
	    rv = recvfrom(sockfd, recv_cnt, PSTRLEN, 0, (struct sockaddr*)&sockaddrs,&len); 
                   if (rv <= 0) {
		     if (rv < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) 
		        printf("[PROCESS] PROCESS END: no data available\n");
			 return (rv = -1);
	               }
			
			  tp= StrtoPacket(tp, recv_cnt);
	                  if(tp->fin == FLAGON)   {state= PROCESS_FIN; break;}
      		        if(tp->ack == FLAGON && socktb->sockstate == ESTABLISHED && socktb->snd_nxt >= tp->ack_seq) 
		          {
				state= PROCESS_ACK_WITHOUT_DATA;
			   	break;
	                  }
	   		 else
			 {printf( "[PROCESS] state error : PROCESS_END \n");
	            	  state= PROCESS_END;
			  break;
		  	 }
			 		
			
		 case PROCESS_FIN: 
	      		socktb->sockstate = CLOSE;
	     		socktb->runstate = PROCESS_END;
	      		break;

	  //server Receive an ACK from client.
		  case PROCESS_ACK_WITHOUT_DATA:
		        
	          printf("[PROCESS] Get an ACK #:%d!\n", tp->ack_seq);
		  if (tp->ack_seq > socktb->snd_una)   //send # unacked
	             {
		             socktb= addwnd(socktb); 
			     printf("[PROCESS] new window size= %d\n", socktb->snd_cwnd/SMSS);
		             socktb->snd_una = tp->ack_seq; 
			    if(socktb->cc_state==1)
 		               printf("[PROCESS] congestion state:SLOW START\n");	
			if(socktb->cc_state==2)
 		               printf("[PROCESS] congestion state:CONGESTION AVOIDANCE\n");	
			if(socktb->cc_state==3)
 		               printf("[PROCESS] congestion state:FAST RECOVERY\n");	
			
//printf("[PROCESS] socktb->snd_una= %d\n", socktb->snd_una);
				int maxseq =tp->ack_seq-1; 
			while(maxseq>=0)
			{		 
			  if(find(socktb->lostbuffer, maxseq)>0)
		              socktb->lostbuffer= DeleteNode(socktb->lostbuffer, maxseq);
			 maxseq--;
			}

		     }
	 	 else if(tp->ack_seq == socktb->snd_una) 
                 { 
	   		printf("[PROCESS] DUP_ACK: %d Duplicate ACKs count: %d", tp->ack_seq, socktb->dupackcount);
		         /* duplicate ack (already receive one) */
			socktb= setwnd(socktb, tp->ack_seq);
		        printf("[PROCESS] new window size= %d\n", socktb->snd_cwnd/SMSS);             
			if(socktb->dupackcount==3)
			{
		 	//fast retransmit
		 	struct Packet		*ptt;
		 	ptt=search(socktb->lostbuffer, tp->ack_seq);  
  
		 	if(ptt->buf!=NULL)
	         	 {
				ptt->ack = FLAGON;
		       		char		strpkt[PSTRLEN];
				strncpy(strpkt, PackettoString(ptt), PSTRLEN);
		  //server send lost packet to client again	          
                 		if( -1 >= (rv = sendto(sockfd,strpkt, PSTRLEN, 0, (struct sockaddr *)&sockaddrc, sizeof(sockaddrc))))
			           perror("[PROCESS] server send packet that lost last time! ");
				   free(strpkt);
				   freePacket(ptt);
	         	 }
	        	}
			 break;
	          }

	       else 
		  {
		           socktb= addwnd(socktb); 
                           printf("[PROCESS] new window size= %d\n", socktb->snd_cwnd);
		           break;//return ACK_SMLTHAN_UNA;
	          }	  
			
              printf("[PROCESS] Try to send data left in circ buff with window size=%d\n", getwnd(socktb)/SMSS);
		
	     rv= sendData(sockfd, socktb);
	      state = PROCESS_END;    
	      break;

		  default:
	             printf("[PROCESS] Error in PROCESS SWITCH STATE\n" );
	             state = PROCESS_END;
		   break;

		}/* END OF SWITCH */
	}/* END OF WHILE(STATE) LOOP */
        printf("[PROCESS] ****** PROCESS END ******\n");
	freePacket(tp);
	return rv;
}/* END of process */

int
main(int argc, char **argv)
{
        struct myinfo	        *ifi, *ifihead;
	struct sockaddr	        *sa;
	int                  i=0, size=0, max=0, bitnumber=0, flag=0, s,udpfds[10], connfd, max_fd, n, rv, new_socket;
        char                    maskaddr[10][20]={"\0"},subaddr[10][20]={"\0"}, ipclient[20]={"\0"}, ipc[20]={"\0"}, buf[PSTRLEN];

        struct in_addr          tmpaddr, ipserver, subserver;
        unsigned long           temp;
        struct sockaddr_in      servaddr[10], cliaddr;
        fd_set                  udpsets;
        pid_t			cpid;
	char              serverinfo[2][MAXLINE]={"\0"}, clientinfo[6][MAXLINE]={"\0"};
        
	
         FILE *file= fopen("server.in", "r");
        if(file!= NULL)
           {
               char line[MAXLINE];
               while(fgets(line, sizeof(line), file)!=NULL)    
                 {               
                   strcpy(serverinfo[i],line);
                   i++;
                 }
                fclose(file);
           }
         else
            {perror("server.in not exist!\n");   return 0; }

	i=0;
	file= fopen("client.in", "r");
        if(file!= NULL)
           {
               char line[MAXLINE];
               while(fgets(line, sizeof(line), file)!=NULL)    
                 {               
                   strcpy(clientinfo[i],line);
                   i++;
                 }
                fclose(file);
           }
         else
            {perror("client.in not exist!\n");   return 0; }
       
		
        for(i=0;i<10;i++)
        udpfds[i]=0;             

        i=0;
	for (ifihead = ifi = Get_myinfo();
		 ifi != NULL; ifi = ifi->next) {
		printf("   %dth address list:\n", i);

                   printf("subnet address: %s\n", ifi->subnetaddr);
                   strcpy(subaddr[i], ifi->subnetaddr);
		
		if ( (sa = ifi->ipaddr) != NULL)
                {
                   strcpy(ipc,inet_ntoa(((struct sockaddr_in *) ifi->ipaddr)->sin_addr));
                        printf("IP address: %s\n",ipc);
          //bind
          bzero(&servaddr[i],sizeof(servaddr[i]));
          udpfds[i] = Socket(AF_INET, SOCK_DGRAM, 0);
          servaddr[i].sin_family= AF_INET;
          Inet_pton(AF_INET, ipc, &servaddr[i].sin_addr);
          servaddr[i].sin_port= htons(atoi(serverinfo[0]));
          if(bind(udpfds[i], (SA *) &servaddr[i], sizeof(servaddr[i]))<0)
            {perror("bind failed!");   exit(EXIT_FAILURE);}
          printf("udp socket binded to: %d\n",udpfds[i]);          
          
              }

		if ( (sa = ifi->maskaddr) != NULL)
                 {
                   tmpaddr= ((struct sockaddr_in *) ifi->maskaddr)->sin_addr;
                   strcpy(maskaddr[i], inet_ntoa(tmpaddr));
                   printf("mask address: %s\n", maskaddr[i]);
                 }
              i++;
		
    }//for

        size=i;
        printf("total number of sockets= %d\n",size);
        
////////////////////////////////main-start//////////////////////////////////
	
      printf("server started!\n");
      FD_ZERO(&udpsets);
        
       while (1) {   
                    max_fd=udpfds[0];    
                    for(i=0;i<size;i++)
                    {
                       s=udpfds[i];
                          if(max_fd<s)
                                   max_fd=s;
                       FD_SET(s, &udpsets);    
                    }
                   
                   n= select(max_fd+1, &udpsets, NULL, NULL, NULL);
                   if(n<0 && errno!=EINTR)
                         {  perror("select failed"); 
                            exit(EXIT_FAILURE);   }
                  if(n>0){
                        for(i=0;i<size;i++)
                      {
                          s=udpfds[i];
                        if(FD_ISSET(s, &udpsets))
                         {
                            //receive filename from client
                            socklen_t len=sizeof(cliaddr);
                   rv = recvfrom(s, buf, PSTRLEN, 0, (struct sockaddr *)&cliaddr, &len);
                            if(rv<=0)
                            {perror("received file name error!"); exit(1);}   
                  printf("client ip address: %s, port #: %d\n", inet_ntoa(cliaddr.sin_addr), ntohs(cliaddr.sin_port));
 
                         struct Packet          *recvtp;
                         char			filename[MAXLINE];

                         recvtp= StrtoPacket(recvtp, buf);
			 strcpy(filename, recvtp->buf);
                         printf("file need to transfer: %s\n", filename);
                          
                        //fork a child
		          if(cpid=Fork()==0)
		          {
                             char	ipserver[20]={"\0"}, ipclient[20]={"\0"};
                             int	k;

                          //close all other socket
                           for(k=0;k<size;k++)
                            {
                                if(k!=i)  close(udpfds[k]);
                            }

		          struct in_addr	  caddr, localaddr, maddr;
		          struct sockaddr_in     saddr;
                          char	    strport[MSS],sendcont[PSTRLEN], recvcont[PSTRLEN], ipc[20]={"\0"}, ipserv[20]={"\0"};
			  struct Packet           *sendtp;
                          
			  strcpy(ipc, inet_ntoa(cliaddr.sin_addr));
                          strcpy(ipserv, inet_ntoa(servaddr[i].sin_addr));
                           		  
               //check if it is local              
			 if(strcmp(ipc,ipserv)==0)
         		 	 flag=1;     //same host
           
         		else      //check if it is local
         		{  
                           caddr.s_addr= inet_addr(ipc);
			   maddr.s_addr= inet_addr(maskaddr[i]);
           		   localaddr.s_addr=caddr.s_addr & maddr.s_addr;
                           if(strcmp(inet_ntoa(localaddr), subaddr[i])==0) //local
                                  flag=2;    
                        }//else

                       if(flag==1)  
                       {
                         printf("server is the same host to client\n");
                         strcpy(ipc, "127.0.0.1");
                         strcpy(ipserv, "127.0.0.1");
                       } 

                       if(flag==2)  
			printf("server is local to client\n");
                       else	    
			printf("server is not local to client\n");
	
			 // printf("server ip address: %s\n", ipserv);

                          bzero(&saddr,sizeof(saddr));
         		  connfd = Socket(AF_INET, SOCK_DGRAM, 0);
         		  saddr.sin_family= AF_INET;
         		  saddr.sin_port= 0;
         		  Inet_pton(AF_INET,ipserv, &saddr.sin_addr);

                          socklen_t  len=sizeof(saddr);
                           if(bind(connfd, (struct sockaddr *)&saddr, sizeof(saddr))<0)
        		    {   perror("server binding error!"); 
				exit(EXIT_FAILURE);}

                         getsockname(connfd, (struct sockaddr *)&saddr, &len);
         		        int ptn=ntohs(saddr.sin_port);
				
                                sprintf(strport, "%d", ptn);
                           	sendtp=InitPacket(sendtp, strport);
   				sendtp->seq=0;
   				sendtp->ack_seq=0;
  				strcpy(sendcont, PackettoString(sendtp));
   				//printf("send to client: %s\n", sendcont);
  
                   //send port number to client
                          if( -1 >= (rv = sendto(udpfds[i],sendcont, PSTRLEN, 0, (struct sockaddr *)&cliaddr, sizeof(cliaddr))))
	 perror("sending port number error! ");
             memset(recvcont,'\0', sizeof(recvcont));
      	     len= sizeof(cliaddr);
             rv = recvfrom(connfd, recvcont, PSTRLEN, 0, (struct sockaddr *)&cliaddr, &len);
                if(rv<=0)
                    {perror("receing file name error!"); exit(1);}	          
			// printf("received: %s\n", recvcont);
                         recvtp= StrtoPacket(recvtp, recvcont);
                         if(recvtp->buflen==0 && recvtp->ack==1)
                             printf("received ack from client!\n");
                         else printf("not an ACK!\n");

		printf("server ip: %s, port: %d\n", inet_ntoa(saddr.sin_addr), ntohs(saddr.sin_port));
		printf("client ip: %s, port: %d\n", inet_ntoa(cliaddr.sin_addr), ntohs(cliaddr.sin_port));
              	

	close(udpfds[i]);
      printf("starting file transfer!\n");
   
//////////////////////////////file transfer/////////////////////////////////////

 	 struct SockTb		*socktb;
	 FILE 		 	*f;
	 int			readsize, sendsize,rcvbuf,selectval;
	 char			*dataset;
	 fd_set              	socks;
 	 struct timeval	    	tim;
	 long			pos, res;

         socktb=initsockTb(socktb, connfd);
       	 socktb->sport= ntohs(cliaddr.sin_port);
         socktb->dport= ntohs(saddr.sin_port);
         strcpy(socktb->sip, inet_ntoa(cliaddr.sin_addr));
         strcpy(socktb->dip, inet_ntoa(saddr.sin_addr));         
	 socktb->snd_awnd = atoi(serverinfo[1])*SMSS;      //advertise window size
	 socktb->snd_ssthresh= atoi(clientinfo[3])*SMSS;//= recevied window size

	 printf("file name=%s\n", filename);
	 f = fopen(trim(filename), "rb");
	 if(f==NULL)
	 {fputs("no file has such name can be read!", stderr); exit(1);}

    	 fseek(f, 0, SEEK_END);
    	 pos = ftell(f);
    	 rewind(f);
	// printf("file size=%d\n", pos);
    	 dataset = (char*) malloc(sizeof(char)*(pos+100));
	  if(dataset==NULL)   
	{fputs("memory error\n", stderr); exit(1);}

    	 res= fread(dataset, pos, 1, f);
	  if(res!=1)
	{fputs("reading error\n", stderr); exit(1);}
    	 fclose(f);
	
	socktb->cb_send = CBinsert(socktb->cb_send, dataset, pos); 
	free(dataset);
	
	rv= sendData(connfd, socktb);
		
	while(1){
		FD_ZERO(&socks);
		FD_SET(connfd, &socks);
		tim.tv_sec = TIMEOUTVAL;
		tim.tv_usec = 0;

		selectval = select(connfd+1, &socks, NULL, NULL, &tim);
		if (selectval){
				run(connfd, socktb);
		 if(socktb->sockstate == CLOSE)
			{printf("server recived FIN ack from client\n");
			 break;} 
			
		    }
			else if(selectval == 0)	
				break;	
	       }

		  if(socktb->sockstate == CLOSE)
		{
               //if recived an fin ack from client, close socket, else keep sending fin to C
		  printf("close socket!\n"); 
		  close(connfd);
		  freesockTb(socktb);
		  break;
		}

//////////////////////////file transfer end////////////////////////////////
		          }  
		             
                         }//if FD_ISSET
                      }//for
                   }//if(select>0)
             }//while 

/////////////////////////////main-end//////////////////////////////////////
	free_myinfo(ifihead);
	exit(0);
}

