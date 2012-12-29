#include	"get_myinfo.h"
#include        <stdio.h>
#include	"SockTb.h"
#include	"para.h"
#include	"unprtt.h"
#include	<setjmp.h>

#define TIMEOUTVAL	6
				
static sigjmp_buf jmpbuf;

static void 
sig_alrm(int signo)
{
	siglongjmp(jmpbuf, 1);
}

void 
proc_delack(struct SockTb *stb, double p) {
	
		 struct sockaddr_in	sockaddrs;
		 struct Packet		*tou_pkt;
                 char			strpkt[PSTRLEN];
		 int			rv;
	    	
	     tou_pkt= initAck(tou_pkt);
	     //client send ack to server
             bzero(&sockaddrs, sizeof(sockaddrs));
	     sockaddrs.sin_family = AF_INET;
	     sockaddrs.sin_port = htons(stb->dport);
	     inet_pton(AF_INET, stb->dip, &sockaddrs.sin_addr);
	     tou_pkt->seq= stb->snd_nxt;
	     tou_pkt->ack_seq= stb->rcv_nxt;
   	     tou_pkt->ack = 1;
	     strcpy(strpkt, PackettoString(tou_pkt));

	if (stb->t_delack_state == DELACK_IMMED_UPDATE_XMIT ||stb->t_delack_state == DELACK_XMIT) 
          {	printf("[PROCESS] DELAY_XMIT!\n");
		int maxseq =stb->rcv_nxt-1; 
		while(maxseq>=0)
		{		 
		   if(find(stb->ackqueue, maxseq)>0)
		       stb->ackqueue= DeleteNode(stb->ackqueue, maxseq);
	           maxseq--;
		}

	   if( -1 >= (rv =sendto(stb->sockd, strpkt, PSTRLEN, 0, (struct sockaddr *)&sockaddrs, sizeof(sockaddrs))))
	        perror("sending ACK error! ");
		free(tou_pkt); 
		printf("[PROCESS] transmit an ack with ack_seq= %d\n", stb->rcv_nxt);
	}

            else if (stb->t_delack_state == DELACK_QUE) 
           {
	       printf("[PROCESS] DELACK_DEQUEUE: xmit an ack %d\n", stb->rcv_nxt);
		
	     if( -1 >= (rv =sendto(stb->sockd, strpkt, PSTRLEN, 0, (struct sockaddr *)&sockaddrs, sizeof(sockaddrs))))
	        perror("sending ACK error! ");
	        free(tou_pkt);
		printf("[PROCESS] transmit an ack with ack_seq= %d\n", stb->rcv_nxt);
		

	   }else {
		perror( "Error in process::proc_delack()\n"); 
		exit(1);
	   }

printf("[PROCESS] the next packet client expected is #%d\n", stb->rcv_nxt);
}



int proc_pkt_rec(struct SockTb *stb, int sockfd) {

	int retval = 0;
    if(length(stb->outoforderbuf)<=1)
		{printf("[PROCESS] no packet on outoforder buffer!\n"); return retval;}
    printf("[PROCESS] Recovery Start from packet with seq#: %d", stb->rcv_nxt);
	retval = do_proc_pkt_rec(stb, sockfd);
        stb->t_delack_state = DELACK_IMMED_UPDATE_XMIT;
	return retval;
}


int do_proc_pkt_rec(struct SockTb *stb, int sockfd) {
	
	int datasz, lenofcb;	//byte stored in HpRecvBuf, payload size
	while(length(stb->outoforderbuf)>1)
	{
	  if(Minseq(stb->outoforderbuf)> stb->rcv_nxt)
		return -1;

	    struct Packet *pkt;
	    pkt= search( stb->rcv_nxt, stb->outoforderbuf);
          if (pkt!=NULL) 
          {
	    printf("[PROCESS] packet with seq#: %d put onto receive buffer\n",pkt->seq);
	    datasz = strlen(pkt->buf);
	    stb->cb_recv = CBinsert(stb->cb_recv, pkt->buf,  datasz);
            stb->rcv_nxt = pkt->seq + 1;	
	  //  printf("[PROCESS] next expected packet has seq#=: %d\n", stb->rcv_nxt);		  
	  } 
       	
	   stb->outoforderbuf= DeleteNode(stb->outoforderbuf, pkt->seq);
	}/* End of while */
	return 0;
}	



int run(int sockfd, struct SockTb *socktb, double p) {
	int			rv, state, bytes;
	char   			recv_cnt[PSTRLEN];
	struct Packet      	*tp, *fintp, *pkt, *acktp;
	struct sockaddr_in 	sockaddrs, sockaddrc;
	socklen_t		len;
	double			rnum;
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
			printf("[PROCESS] received a new packet!\n");
			rnum= ((double) rand())/(RAND_MAX+1);
		 	if(rnum<0)
			rnum=rnum*(-1);
			if(rnum<p) {
			socktb->t_delack_state = DELACK_QUE;
			state= PROCESS_GET_PKT;
			printf("[PROCESS] drop this packet!\n ");break;
			}

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
	                  
		 printf("[PROCESS] client received packet with seq#: %d\n", tp->seq);
		
	     if(tp->fin == 1)   {state= PROCESS_FIN; break;}		 
             if(tp->ack == 1&&tp->seq==socktb->rcv_nxt && socktb->sockstate == ESTABLISHED)
		 { state= PROCESS_ACK_WITH_DATA_MATCH_EXPECTED_SEQ; break;}
             if(tp->ack == 1 &&tp->seq < socktb->rcv_nxt &&socktb->sockstate == ESTABLISHED)
		   { state= PROCESS_ACK_WITH_DATA_LESS_EXPECTED_SEQ; break;}
	     if(tp->ack == 1 &&tp->seq > socktb->rcv_nxt &&socktb->sockstate == ESTABLISHED)
		    { state= PROCESS_ACK_WITH_DATA_MORE_EXPECTED_SEQ; break;}

	         else
		 {printf( "[PROCESS] state error : PROCESS_END \n");
	           state= PROCESS_END;
		   break;	
		  }

		  /* Check for FIN flag (Close). Connection Control: Chinmay add here.*/
		  case PROCESS_FIN: 
			printf("[PROCESS] client received FIN from server\n");		
			memset(recv_cnt, '\0', sizeof(recv_cnt));        
			fintp= initAck(fintp);
			fintp->fin= FLAGON;
       			strncpy(recv_cnt, PackettoString(fintp), 13);
   			printf("[PROCESS] client send FIN ack to server: %s\n", recv_cnt);
			if( -1 >= (rv = sendto(sockfd, recv_cnt, strlen(recv_cnt), 0, (struct sockaddr *)&sockaddrs, sizeof(sockaddrs))))
	 		perror("[PROCESS] client sending FIN ack error! ");

	      		socktb->sockstate = CLOSE_WAIT;
	      		state = PROCESS_END;
	      		break;

	               // recover loss pkts from outoforderbuf.
		  case PROCESS_PKT_RECOVERY:  
			  printf("[PROCESS] enter fast recovery!\n");
			  if (0 == proc_pkt_rec(socktb, sockfd)) {
		 /* if recovery successfully, set init runstate to "GET_PKT" */
			 socktb->runstate = PROCESS_GET_PKT;
			 state = PROCESS_ACK_DATARECSUCC_SENDBACK_ACK;

			  }else {
			  socktb->runstate = PROCESS_PKT_RECOVERY;
			  state = PROCESS_ACK_DATARECSUCC_NO_SENDBACK_ACK;
			  }
			  break;

		 case PROCESS_ACK_WITH_DATA_MATCH_EXPECTED_SEQ:
		
			socktb->rcv_nxt = tp->seq + 1;
	          	socktb->cb_recv= CBinsert(socktb->cb_recv, tp->buf, strlen(tp->buf));
                        state = PROCESS_PKT_RECOVERY; //try if recovery is possible
			socktb->t_delack_state = DELACK_XMIT;		
		  	break;

		  case PROCESS_ACK_WITH_DATA_LESS_EXPECTED_SEQ:
		     	 if(getAvSize(socktb->cb_recv)>= MSS) 
		        	state = PROCESS_GET_PKT;	
		      	else 
		        	{printf("[PROCESS] there's no space for the incoming packet!\n"); state = PROCESS_END;}
			break;

		  case PROCESS_ACK_WITH_DATA_MORE_EXPECTED_SEQ:
			
		 	  pinitPacket(pkt, tp);
		          socktb->outoforderbuf=add(socktb->outoforderbuf, pkt);
 			  socktb->t_delack_state = DELACK_QUE;
			 printf("[PROCESS] Push packet #%d onto outoforderbuf.\n ",tp->seq);
			//push an ack of tp to ackqueue
			  printf("push ACK for packet seq=%d to ackqueue.\n", tp->seq+1);
			  acktp= initAck(acktp);
			  acktp->ack_seq= tp->seq+1;  
			  socktb->ackqueue=add(socktb->ackqueue, acktp);
			  state = PROCESS_ACK_DATARECSUCC_SENDBACK_ACK;
			  break;


		  case PROCESS_ACK_DATARECSUCC_SENDBACK_ACK:
		      printf("[PROCESS] process delayed acks\n");
		      proc_delack(socktb, p);//client send ack to server

		  case PROCESS_ACK_DATARECSUCC_NO_SENDBACK_ACK:
	          if (getAvSize(socktb->cb_recv)>=MSS && socktb->runstate==PROCESS_GET_PKT) 
		     state = PROCESS_GET_PKT;	
                   else 
		     state = PROCESS_END;	
	           break;
		  default:
	             printf("Error in PROCESS SWITCH STATE\n" );
	             state = PROCESS_END;
		   break;

		}/* END OF SWITCH */
	}/* END OF WHILE(STATE) LOOP */

        printf("[PROCESS] ****** PROCESS END ******\n");
	freePacket(tp);
	return rv;
}/* END of run */



int
main(int argc, char **argv)
{
	char		 filename[MAXLINE];
        char             clientinfo[6][MAXLINE]={"\0"};
        char            *portserver;
	struct myinfo	*ifi, *ifihead;
	struct sockaddr	*sa;
	int              i=0, max=0, bitnumber=0, flag=0,rv, k=0;
        char             ipclient[20]={"\0"}, ipc[20]={"\0"}, ipserv[20]={"\0"};
        struct in_addr  tmpaddr, ipserver, subserver;
        unsigned long   temp;
        struct sockaddr_in     cliaddr, servaddr;
        int                    udpfd;
        int                    size, n;
        double			p;

       FILE *file= fopen("client.in", "r");
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
       
           ipserver.s_addr= inet_addr(clientinfo[0]); //string to in_addr
         
	   p=atof(clientinfo[5]);

	for (ifihead = ifi = Get_myinfo();
		 ifi != NULL; ifi = ifi->next) {
		   printf("   %dth address list:\n", k);

                   printf("subnet address: %s\n", ifi->subnetaddr);
		
		if ( (sa = ifi->ipaddr) != NULL)
                {
                   strcpy(ipc,inet_ntoa(((struct sockaddr_in *) ifi->ipaddr)->sin_addr));
                        printf("IP address: %s\n",ipc);
                 }


		if ( (sa = ifi->maskaddr) != NULL)
                 {
                 tmpaddr= ((struct sockaddr_in *) ifi->maskaddr)->sin_addr;
                 printf("mask address: %s\n",Sock_ntop_host(sa, sizeof(*sa)));
                 }
           	
           if(strcmp(inet_ntoa(ipserver),ipc)==0)
          { flag=1;     //same host
            break;   }
          

         else      //check if it is local
         {  bitnumber=0;
            subserver.s_addr=ipserver.s_addr & tmpaddr.s_addr;
            if(strcmp(inet_ntoa(subserver), ifi->subnetaddr)==0) //local
            {
                flag=2;    
                temp= tmpaddr.s_addr;
                temp= temp-((temp>>1) & 0x55555555);
                temp= (temp & 0x33333333)+ ((temp >>2) & 0x33333333);
                bitnumber= (((temp+(temp>>4)) & 0xF0F0F0F0)*0x10101010)>>24;

               // printf("bit number= %d\n", bitnumber); //1 bits in mask 
                                
             if(max< bitnumber)
              { 
                  max= bitnumber;
                  strcpy(ipclient,ipc);
              }
            }//if

        }//else
	k++;
    }//for

          bzero(&cliaddr,sizeof(cliaddr));
          udpfd = Socket(AF_INET, SOCK_DGRAM, 0);
          cliaddr.sin_family= AF_INET;
          cliaddr.sin_port= 0;

        switch(flag)
        {
          case 0: 
               printf("server is not local to client\n");
               Inet_pton(AF_INET, ipc, &cliaddr.sin_addr);
               printf("bind to address: %s\n",ipc);
               break;
         case 1:
               printf("server is the same host to client\n");
               Inet_pton(AF_INET, "127.0.0.1", &cliaddr.sin_addr);
               ipserver.s_addr= inet_addr("127.0.0.1");
               printf("bind to address: 127.0.0.1\n");
               break;
         case 2:
               printf("server is local to client\n");
               Inet_pton(AF_INET, ipclient, &cliaddr.sin_addr);
               printf("bind to address: %s\n",ipclient);
               break;
        default: break;
        }

        if(bind(udpfd, (SA *) &cliaddr, sizeof(cliaddr))<0)
         {perror("client binding error!"); exit(EXIT_FAILURE);}
        
        strcpy(ipserv, inet_ntoa(ipserver));
        socklen_t len=sizeof(cliaddr);
        if(getsockname(udpfd, (struct sockaddr *)&cliaddr, &len)==-1)
          perror("getsockname error");
        else
       printf("client address: %s port: %d\n", inet_ntoa(cliaddr.sin_addr), ntohs(cliaddr.sin_port));

    //connect to server
     bzero(&servaddr,sizeof(servaddr));
     servaddr.sin_family= AF_INET;
     servaddr.sin_port= htons(atoi(clientinfo[1]));
     Inet_pton(AF_INET, ipserv, &servaddr.sin_addr);
     
     getpeername(udpfd, (struct sockaddr *)&servaddr, &size);
     printf("server address: %s port: %d\n", inet_ntoa(servaddr.sin_addr), ntohs(servaddr.sin_port));

       
/////////////////////////////////main-start/////////////////////////////////
//send file name to server, serverinfo into a packet send to server
   struct Packet          *sendtp, *recvtp;
   char			  sendcont[PSTRLEN], recvcont[PSTRLEN];
   int			  portnumber;

	 struct SockTb		*socktb;
         socktb=initsockTb(socktb, udpfd);
       	 socktb->sport= ntohs(cliaddr.sin_port);
         
         strcpy(socktb->sip, inet_ntoa(cliaddr.sin_addr));
         strcpy(socktb->dip, inet_ntoa(servaddr.sin_addr));
       //  printf("server ip: %s port:%d\n",socktb->dip, socktb->dport);
       //  printf("client ip: %s port:%d\n",socktb->sip, socktb->sport);

         socktb->snd_awnd= atoi(clientinfo[3])*SMSS;//= recevied window size
	 if (socktb->rttinit == 0) {
		rtt_init(&socktb->rttinfo);		/* first time we're called */
		socktb->rttinit = 1;
		//rtt_d_flag = 1;
	}
	uint32_t ts;
   sendtp=InitPacket(sendtp, clientinfo[2]);
   sendtp->seq=0;
   sendtp->ack_seq=0;
   strcpy(sendcont, PackettoString(sendtp));

sendagain:
	ts = rtt_ts(&socktb->rttinfo);

   printf("client send file name to server!\n");
   if( -1 >= (rv = sendto(udpfd, sendcont, PSTRLEN, 0, (struct sockaddr *)&servaddr, sizeof(servaddr))))
	 perror("sending error! ");
	alarm(rtt_start(&socktb->rttinfo));
	if (sigsetjmp(jmpbuf, 1) != 0) {
		if (rtt_timeout(&socktb->rttinfo) < 0) {
			perror("dg_send_recv: no response from server, giving up");
			socktb->rttinit = 0;	/* reinit in case we're called again */
			errno = ETIMEDOUT;
			return(-1);
		}
	goto sendagain;
	}
   //get port number
          len=sizeof(servaddr);
        if(-1 >=(rv = recvfrom(udpfd, recvcont, PSTRLEN, 0, (struct sockaddr *)&servaddr, &len)))
         perror("receiving port number error! ");
	
	alarm(0);			/* stop SIGALRM timer */
		/* 4calculate & store new RTT estimator values */
	rtt_stop(&socktb->rttinfo, rtt_ts(&socktb->rttinfo) - ts);


         recvtp= StrtoPacket(recvtp, recvcont);
        int portnum= atoi(recvtp->buf);
         printf("the ephemeral port number of server: %d\n", portnum);
        socktb->dport= portnum;
        //reconnect to server
        bzero(&servaddr,sizeof(servaddr));
        servaddr.sin_family= AF_INET;
        servaddr.sin_port= htons(portnum);
        Inet_pton(AF_INET, ipserv, &servaddr.sin_addr);
      
     //send ack to connfd, if time out, need to send to listening ds and connfd, 2 copy
	memset(sendcont, '\0', sizeof(sendcont));        
	sendtp= initAck(sendtp);
        strncpy(sendcont, PackettoString(sendtp), 13);
   	printf("send ACK to server!\n");
          if( -1 >= (rv = sendto(udpfd, sendcont, strlen(sendcont), 0, (struct sockaddr *)&servaddr, sizeof(servaddr))))
	 perror("sending ack error! ");
                    
         printf("starting file transfer!\n");
       
//////////////////////////////file transfer////////////
	srand(atoi(clientinfo[4]));
	
        char			recv_data[MSS];
	int			end,sd, clearrs;
	struct timeval 		tim;
        fd_set 			socks;

	    while(1)
	       {    
		FD_ZERO(&socks);
		FD_SET(udpfd, &socks);
		tim.tv_sec = TIMEOUTVAL;
		if (select(udpfd+1, &socks, NULL, NULL, &tim))
		{
			
			run(udpfd, socktb, p);	 //get packet
		       if(socktb->sockstate == CLOSE_WAIT) 
		             { printf("client send FIN ack to server then close\n"); break;}
		}
		else
		{
				printf( "Select Timeout: Exit!\n");
				break;
		}      
	       }

		if(socktb->sockstate == CLOSE_WAIT)
		{
		//send an FIN ACK to server
			
		  printf("received content from server: \n");
                  printf("%s\n", socktb->cb_recv->m_buf);
		  close(udpfd);
		  freesockTb(socktb);  
		}
		
/////////////////////////////////main-end/////////////////////////////

	free_myinfo(ifihead);
	exit(0);
}

