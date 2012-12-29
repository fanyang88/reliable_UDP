#include "SockTb.h"
#include "para.h"
#include <stdlib.h>
#include <limits.h>

Linklist 
Create(Linklist p_list)
{
	PNode p=NULL;
	p_list = (Linklist)malloc(sizeof(Node));
	p_list->packet =initAck(p_list->packet);
	p_list->packet->seq= -1;
	p_list->next = NULL;
	return p_list;
	
}


int 
find(Linklist list, int num)
{
  int count=0;
  PNode cur_ptr;
  cur_ptr= list;
  while(cur_ptr != NULL)
  {
     if(cur_ptr->packet->seq==num)
	return 1;
      cur_ptr=cur_ptr->next;
    
  }
  return -1;
}


int 
length(Linklist list)
{
  int count=0;
  PNode cur_ptr;
  cur_ptr= list;
  while(cur_ptr != NULL)
  {
     cur_ptr=cur_ptr->next;
     count++;
  }
  return count;
}


struct Packet*
search(int num, Linklist list)
{
    struct Packet  *pt;
    PNode cur_ptr;
    cur_ptr= list;
    while(cur_ptr->packet->seq != num && cur_ptr!= NULL)
    {
     cur_ptr=cur_ptr->next;
    }
  
    if(cur_ptr->packet->seq == num && cur_ptr!= NULL)
	pt=pinitPacket(pt, cur_ptr->packet);
	 
	else  pt=NULL;

        return pt;
           
}


Linklist   
add(Linklist list, struct Packet *pt)
{
	PNode p,q;
	int i=0, pos;
	p = list;

	pos= length(list);
	
	while(p!=NULL && i<pos-1)
	{
		p = p->next;
		i++;
	}
	
	q = (Node *)malloc(sizeof(Node));
	q->packet= pinitPacket(q->packet, pt);
	q->next = p->next;
	p->next = q;
	printf("insert node with seq#: %d success!\n", pt->seq);
	return list;
	
}


Linklist  
DeleteNode(Linklist list, int num)
{
	PNode p,q, prev;
	int i=0, flag=0;
	p = list;
	if(p==NULL)
        {printf("list in empty!\n"); return list;}

	if(list->packet->seq==num)
        {
		q = list;
		list = p->next;
		free(q);
		return list;
        }


	while(p!=NULL)
	{	prev=p;
	        p = p->next;
		if(p->packet->seq==num)
			{flag=1; break;}
	}
	
	if(flag==0)
	   {printf("packet not in list!\n"); return list; } //num not exit;

	q = p;
	prev->next = p->next;
	free(q);
	printf("deleted the packet with seq#; %d in the list\n", num);
	return list;
}

int 
Minseq(Linklist list)
{

  int mseq=INT_MAX;
  PNode cur_ptr;
  cur_ptr= list;
  while(cur_ptr != NULL)
  {
    if(cur_ptr->packet->seq < mseq && cur_ptr->packet->seq!=-1)
	mseq= cur_ptr->packet->seq;

     cur_ptr=cur_ptr->next;
  }
  return mseq;

}
