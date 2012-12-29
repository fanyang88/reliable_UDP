this project implemented flow control, congestion control, fast recovery in TCP over UDP.
I defined a struct called "Packet" to structure the datagram form.
the packet is defined as:

      int			seq;   	// Sequence Number: 16 bits
	    int			ack_seq;// Acknowledgment Number: 16 bits
		
	    u_short		ack:1;	// Acknowledgement: 1 bit
	    u_short		syn:1;	// Synchronize sequence numbers: 1 bit
	    u_short		fin:1;	// No more data from sender: 1 bit
	    u_short		res:5;	// Reserved: 5 bit
		
	    char		*buf;	// Payload
	    int			buflen;	// Length of payload


   I also defined a struct called "SockTb" to store the destion address, source address, congestion state, 
   ACK delayed state, next expected sequence, current window size, a lostbuffer which store the packet has 
   been dropped by client, a buffer called outoforderbuf which store the packet arrived at client side that 
   are out of order, a list called ackqueue which store the acks that come eariler which has sequence bigger 
   than the expected sequence at that time, etc. I used standard circular buffer to store the data needs to 
   be send at source side, and the data has been received at the destination side.

   To make sure client and server side can be closed when the last datagram has been send. The idea is when 
   the circular buffer and lost buffer at server side are empty, that would trigger the server to send a FIN 
   to client, while server would set its state to be "WAIT_TO_CLOSE". When the client got the FIN from server, 
   it ll send a FIN ACK to server first, then close its socket. When the server received the FIN ACK from client, 
   it would close it socket as well.
