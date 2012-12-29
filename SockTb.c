
#include "SockTb.h"
#include "para.h"
#include <stdlib.h>

struct SockTb *
initsockTb(struct SockTb *tc, int sockfd)
{
        tc = Calloc(1, sizeof(struct SockTb));
        tc->runstate = PROCESS_GET_PKT;
        tc->sockd= sockfd;
        tc->sockstate= ESTABLISHED;
	tc->cc_state= CC_SS;
	tc->t_delack_state= NODELAY;
	tc->iss=0;	//initial send seq #
	tc->irs=0;	//initial rcv seq #
	tc->snd_una=0;   //send # unacked
	tc->snd_nxt=0;//send # next
	tc->rcv_nxt=0;//rcv  # next
	tc->dupackcount=0;//duplicate ack(should count to three)
	tc->snd_wnd=0;	//sender's window
	tc->rcv_wnd=0;	//rcv window
	tc->snd_cwnd=SMSS;	//congestion-controlled wnd
	tc->snd_awnd=0;//sender's advertised window from recver    
	//tc->snd_ssthresh= MAX_SSTHRESH;   //snd_cwnd size threshold for slow start
		
	tc->rttinit=0;
        tc->cb_send= initCB(tc->cb_send);
        tc->cb_recv= initCB(tc->cb_recv);
        tc->sip= malloc(20*sizeof(char));
        tc->sport=0;
        tc->dip=malloc(20*sizeof(char));
        tc->dport=0;
	tc->t_delack_state = DELACK_XMIT;

	tc->outoforderbuf= Create(tc->outoforderbuf);
	tc->lostbuffer= Create(tc->lostbuffer);
	tc->ackqueue= Create(tc->ackqueue);
	//rtt_init(tc->rttinfo);
	return tc;
}

void
freesockTb( struct SockTb *tc)
{
        if (tc->cb_send != NULL)
			free(tc->cb_send);
	if (tc->cb_recv != NULL)
			free(tc->cb_recv);
	 if (tc->sip != NULL)
			free(tc->sip);	
        if (tc->dip != NULL)
			free(tc->dip);	
        if (tc->outoforderbuf != NULL)
			free(tc->outoforderbuf);	
        if (tc->lostbuffer != NULL)
			free(tc->lostbuffer);	
        free(tc);
}


struct SockTb*	 
addwnd(struct SockTb *tc) {
	
   int err = 0;
  switch(tc->cc_state) {
    case CC_ERR:
  	  /* some tou_congestion_control err msg here */
		  err = 1;
		  break;

		case CC_SS:
			/* while cwnd less than ssthresh, we adopt slow start.*/
			if( tc->snd_cwnd < tc->snd_ssthresh ){
                                 tc->snd_cwnd += SMSS ;
				//tc->snd_cwnd = ((tc->snd_cwnd>(MAX_CIRCULAR_BUF/2)) ? (unsigned long)(MAX_CIRCULAR_BUF/2) : tc->snd_cwnd);
			}else{
			//go to CA
				tc->cc_state = CC_CA;
				tc->snd_cwnd += (SMSS*SMSS/tc->snd_cwnd);
				//tc->snd_cwnd = ((tc->snd_cwnd>(MAX_CIRCULAR_BUF/2)) ? (unsigned long)(MAX_CIRCULAR_BUF/2) : tc->snd_cwnd);
			}
			tc->dupackcount = 0;
			break;

		case CC_CA:
			tc->snd_cwnd += (SMSS*SMSS/tc->snd_cwnd);
			//tc->snd_cwnd = ((tc->snd_cwnd>(MAX_CIRCULAR_BUF/2)) ? (unsigned long)(MAX_CIRCULAR_BUF/2) : tc->snd_cwnd);
			tc->dupackcount = 0;
			break;

		case CC_FR:
			//go to CA
			tc->cc_state = CC_CA;
			tc->snd_cwnd = tc->snd_ssthresh;
			tc->dupackcount = 0;
			break;

		default:
			/* some err msg here */
			err = 1;
			break;
	}
	return tc;
}/* end of addwnd() */


 /**
 * setwnd();
 * set wnd while receives duplicate ack
 * return 1 if failure.
 * return 0 if success.	     
 */
 
struct SockTb*  
setwnd(struct SockTb *tc, int ack_seq) {
    int err = 0;
	switch(tc->cc_state) {
		case CC_ERR:
			/* some err msg here */
			err = 1;
			break;

		case CC_SS:
		case CC_CA:
			/* dup on != snd_una can only happend when re-ordering
			 * or reproduction of network. Either case can we omit
			 * the effect of dup ACKs.*/
			if (ack_seq == tc->snd_una) 
				tc->dupackcount++;
			else 
				//not continuous ACK seq
				tc->dupackcount = 1;		

			if(tc->dupackcount >= 3) {
				/* 3dup ack should go to FR */
				tc->cc_state = CC_FR;
				tc->snd_ssthresh = tc->snd_cwnd/2;
				tc->snd_cwnd = tc->snd_ssthresh;
			}
			break;

		case CC_FR:
			if (ack_seq == tc->snd_una) {
				tc->dupackcount++;
			} else {
				//not continuous ACK seq
				tc->dupackcount = 1;
			}

			tc->snd_cwnd += SMSS;
                         
		    break;

		default:
			/* some err msg here */
			err = 1;
			break;
	}
	return tc;
}/* end of int setdwnd() */


/**
 * get current window
 */
int 
getwnd(struct SockTb *tc) {
    tc->snd_wnd =tc->snd_cwnd;
    if(tc->snd_cwnd>tc->snd_awnd)
	  tc->snd_wnd =tc->snd_awnd;
	return tc->snd_wnd;
}
