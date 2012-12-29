#ifndef	__SockTb_h
#define	__SockTb_h

#include	<net/if.h>
#include	"para.h"
#include	"unprtt.h"
#include	<setjmp.h>

//#include "unprtt.h"
//circular_buffer

struct CircularBuffer {
	        unsigned int		m_uiSize;	//total length of the queue
		unsigned int		m_uiHi;	//head of the queue
		unsigned int		m_uiTi;	//tail of the queue. always points to the location where data should be inserted
		unsigned int		m_uiTotEl;	//total number of elements in the queue
		char			*m_buf;	//queue buffer
	 
};

/* function prototypes */
struct CircularBuffer* initCB(struct CircularBuffer *);
void freeCB(struct CircularBuffer *);
struct CircularBuffer* CBinsert(struct CircularBuffer *, char* buf, int n);
  //Remove n bytes from the start of the queue. If n > size, then only remove n-size elements
int removeBytes(struct CircularBuffer *, int n);
  //0 if 0<=n, or empty queue, Get n bytes from the start of the queue. returns the index of the queue where to fetch again
int getAt(struct CircularBuffer *, char* buf, int n, int end);
//Get the size which are available in the queue	
int getAvSize(struct CircularBuffer *);
//get the total number of elements in queue
int getTotalElements(struct CircularBuffer *);


//Packet

struct Packet {

	    int				seq;		// Sequence Number: 16 bits
	    int				ack_seq;// Acknowledgment Number: 16 bits
		
	    u_short				ack:1;	// Acknowledgement: 1 bit
	    u_short				syn:1;	// Synchronize sequence numbers: 1 bit
	    u_short				fin:1;	// No more data from sender: 1 bit
	    u_short				res:5;	// Reserved: 5 bit
		
		char				*buf;	// Payload
		int				buflen;	// Length of payload
};

struct Packet* InitPacket(struct Packet *, char *str);
struct Packet* pinitPacket(struct Packet *, struct Packet *pkt);
struct Packet* StrtoPacket(struct Packet *, char *a);
struct Packet* initAck(struct Packet *);
struct Packet* clearFlag(struct Packet *);
void clean(struct Packet *);
char* PackettoString(struct Packet *);
char* InttoStr(int num);
struct Packet * freePacket(struct Packet *);


typedef struct node * PNode;
typedef struct node
{
	struct Packet *packet;
	PNode next;
	
}Node,* Linklist;


Linklist Create(Linklist );
int length(Linklist );
struct Packet* search(int num, Linklist );
Linklist  add(Linklist , struct Packet *p);
Linklist  DeleteNode(Linklist , int num);
struct Packet* search(int num, Linklist );
int Minseq(Linklist );
int find(Linklist , int num);

//sockTb

struct SockTb {

		//boost::mutex stb_cb_send_mtx;//circular buffer sned mutex
		//boost::mutex stb_cb_recv_mtx;//circular buffer recv mutex
		
		int	  sockd;//socket file descriptor
		int	sockstate;  // BIND, LISTEN,ESTABLISHED, TERMINATING
		int	runstate;//processtou's run func.	
		u_short	   sport;    //souce port number
		u_short    dport;      //destination port number
		char       *sip;	//source ip address
		char       *dip;	//destination ip address	

		//congestion control
	     short		cc_state;//3 congestion states
	     short		t_delack_state;	//2 delayed ack states
		 
		// struct rtt_info	*rttinfo;	
    	
        int	  	iss;	//initial send seq #
	int		irs;	//initial rcv seq #
        int		snd_una;   //send # unacked
        int		snd_nxt;//send # next
        int		rcv_nxt;//rcv  # next

        short		dupackcount;//duplicate ack(should count to three)
        int		snd_wnd;	//sender's window
        int		rcv_wnd;	//rcv window
        int		snd_cwnd;	//congestion-controlled wnd
        int		snd_awnd;//sender's advertised window from recver    
        int		snd_ssthresh;//snd_cwnd size threshold for slow start
	uint32_t	ts;
	int		rttinit;
		//protected by stb_cb_send_mtx, stb_cb_recv_mtx, stb_minhp_recv_mtx
struct CircularBuffer 	*cb_send;//circular buffer for sending
struct CircularBuffer 	*cb_recv;//circular buffer for recving
 Linklist		outoforderbuf;	//buffer for out-of-order packets
 Linklist		lostbuffer;  //buffer for lost packet
 Linklist		ackqueue;

struct rtt_info		rttinfo;

};

struct SockTb* initsockTb(struct SockTb *, int sockfd);
void freesockTb(struct SockTb *);
struct SockTb* addwnd(struct SockTb *);
struct SockTb* setwnd(struct SockTb *, int ackseq);
int getwnd(struct SockTb *);

#endif	
