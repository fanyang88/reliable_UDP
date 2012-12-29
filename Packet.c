#include "SockTb.h"
#include "para.h"
#include <stdlib.h>

char* 
substring(size_t start, size_t stop, char *src, char *dst, size_t size)
{
   int count= stop- start;
   if(count>=--size)
   {
      count=size;
   }
   sprintf(dst, "%.*s", count, src+start);
   return dst;
}

struct Packet* 
InitPacket(struct Packet *tp, char *str)
{
   tp = Calloc(1, sizeof(struct Packet));
   tp->buf = malloc(strlen(str) * sizeof(char));
   memcpy(tp->buf, str, strlen(str)); 
   tp->buflen = strlen(str);
   tp->fin=0;
   tp->syn=0;
   tp->ack=1;
   tp->seq=0;
   tp->ack_seq= 0;
   return tp;
}

struct Packet*  
pinitPacket(struct Packet *p, struct Packet *pkt)
{
                        p = Calloc(1, sizeof(struct Packet));
                        p->seq= pkt->seq;
			p->ack_seq= pkt->ack_seq;
			p->ack= pkt->ack;
			p->syn= pkt->syn;
			p->fin= pkt->fin;
			p->buf = (char *)malloc(strlen(pkt->buf) * sizeof(char));
			memcpy(p->buf, pkt->buf, strlen(pkt->buf));
			p->buflen = pkt->buflen;
			return p;
}

struct Packet* 
StrtoPacket(struct Packet *tp, char *a)
{  
   tp = Calloc(1, sizeof(struct Packet));
   int		sum=0, divv, div=1, i=0, offset=48;
   while(i<MAXNUM)
   {
          div=div*10;
          i++;
   }
   div=div/10;
   divv=div;

  for(i=0;i<MAXNUM;i++)
    {  sum=sum+(a[i]-offset)*div;
       div=div/10;
     }
   
   tp->seq= sum;
 
   sum=0;
   div=divv;
   for(i=MAXNUM;i<2*MAXNUM;i++)
    {  sum=sum+(a[i]-offset)*div;
       div=div/10;
     }

   tp->ack_seq=sum;
   tp->ack=(u_short)(a[2*MAXNUM]-offset);
   tp->syn=(u_short)(a[2*MAXNUM+1]-offset);
   tp->fin=(u_short)(a[2*MAXNUM+2]-offset);
   
   if(strlen(a)>(2*MAXNUM+3))
     {
      tp->buf = malloc(MSS * sizeof(char));
      strncpy(tp->buf, a+2*MAXNUM+3, strlen(a));
      tp->buflen = strlen(a)-(2*MAXNUM+3);
     }
   else
     {
       tp->buf=NULL;
       tp->buflen =0;
     }

   return tp;
}


struct Packet* 
initAck(struct Packet *tp)
{
   tp = Calloc(1, sizeof(struct Packet));
   tp->buf = "\0";
   tp->buflen = 0;
   tp->fin=0;
   tp->syn=0;
   tp->ack=1;
   tp->seq=0;
   tp->ack_seq= 0;
   return tp; 
			
}

 

struct Packet*  
clearFlag(struct Packet *p)
{
			p->ack = 0;
			p->syn = 0;
			p->fin = 0;
                        return p;
}


struct Packet*    
freePacket(struct Packet *p)
{
                        p->seq = 0;
			p->ack_seq = 0;
			p->ack = 0;
			p->syn = 0;
			p->fin = 0;
			if(p->buf!=NULL)
			    free(p->buf);
			p->buflen = 0;
                        free(p);
		        
}

char* 
PackettoString(struct Packet *tp)
{
   char		tempnum[MAXNUM], str[PSTRLEN];
   memset(&str, '\0', sizeof(str));
   memset(&tempnum, '\0', sizeof(tempnum));   
   memcpy(tempnum,InttoStr(tp->seq), MAXNUM); 
   strncat(str, tempnum, MAXNUM);
   memset(&tempnum, '\0', sizeof(tempnum));   
   memcpy(tempnum,InttoStr(tp->ack_seq), MAXNUM);
   strncat(str, tempnum, MAXNUM);
   str[10]= tp->ack+48;
   str[11]= tp->syn+48;
   str[12]= tp->fin+48;
   
   if(tp->buf!=NULL)
   strncat(str, tp->buf, MSS);
   return str;
   
}

char* 
InttoStr(int num)
{
   int		copy=num, count=0, div=1, i=0, offset=48, rest;
  // char		a[MAXNUM];
   char *a= malloc(MAXNUM*sizeof(char));
  // memset(&a, 0,sizeof(a));
	 strncpy(a, "00000", MAXNUM);
           
   if(copy==0)
     {
	    //strncpy(a, "00000", MAXNUM);
            return a;
     }

   while(copy>0)
   {
          copy=copy/10;
          div=div*10;
          count++;
   }
 
  div=div/10;
  rest= MAXNUM-count;

  while(rest< MAXNUM-1)
  {
    a[rest]= num/div+offset;
    num=num%div;
    div=div/10;
    rest++;
  }

  a[rest]= num+offset; 
 // printf("str= %s\n", a);
  return a;
}


