#ifndef	__para_h
#define	__para_h

#define		FLAGON     				1
#define		FLAGOFF     				0

//congestion state
#define		CC_CA					2
#define		CC_SS					1
#define		CC_FR					3
#define     	CC_ERR					4

//#define		MAX_SSTHRESH			262144
#define         MAX_CIRCULAR_BUF		6291456  //6MB
#define		MIN_SEND_SIZE			507
#define		RECVBUFSIZE    			512
#define		MSS				507 //header is 9 bytes	
#define		SMSS				512
#define		MAXNUM    			5
#define		PSTRLEN				520


//delayed ack state
#define		DELACK_IMMED_UPDATE_XMIT	5
#define		DELACK_QUEXMIT			3
#define		DELACK_EXPXMIT			4
#define		DELACK_XMIT			1
#define		DELACK_QUE			2
#define		NODELAY				0

//runstate
#define 	PROCESS_END				0
#define		PROCESS_GET_PKT				1
#define		PROCESS_SYN				2
#define		PROCESS_FIN				3
#define		PROCESS_ACK_WITHOUT_DATA				4
#define		PROCESS_ACK_WITH_DATA_MATCH_EXPECTED_SEQ		5
#define		PROCESS_ACK_WITH_DATA_LESS_EXPECTED_SEQ			6
#define		PROCESS_ACK_WITH_DATA_MORE_EXPECTED_SEQ			7
#define		PROCESS_CLEAR_GET_PKT					8
#define		PROCESS_ACK_DATARECSUCC_SENDBACK_ACK			9
#define		PROCESS_ACK_DATARECSUCC_NO_SENDBACK_ACK			10
#define		PROCESS_PKT_RECOVERY					11

//sockstate
#define		BIND						0
#define		ESTABLISHED					1	
#define		CLOSE						2
#define		CLOSE_WAIT					3
#define    	 LISTEN						4
#define		SYN_SENT					5


#endif
