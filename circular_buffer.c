#include "SockTb.h"
#include "para.h"
#include <stdlib.h>

struct CircularBuffer* 
initCB(struct CircularBuffer *cb)
{
  cb = Calloc(1, sizeof(struct CircularBuffer));
  cb->m_uiHi=0;			//Head index
  cb->m_uiTi=0;			//Tail index
  cb->m_uiSize=MAX_CIRCULAR_BUF;	        //size of the queue
  cb->m_uiTotEl=0;		//total number of elements in the queue
  cb->m_buf= (char*) malloc(MAX_CIRCULAR_BUF*sizeof(char));
  if(cb->m_buf==NULL)
    {printf("memory malloc for circular buffer error\n"); }
  return cb;
}


void 
freeCB(struct CircularBuffer *cb)
{
//free the buffer if not NULL
  if(NULL != cb->m_buf) {
    free(cb->m_buf);
  }
  free(cb);
}


struct CircularBuffer*  
CBinsert(struct CircularBuffer *cb, char* buf, int n)
{
         int start, end, ret = 0, an=0;
         if(n <= 0) {	return 0;}
	/* Check whether the size is available or not in circular buffer */
	if(cb->m_uiSize - cb->m_uiTotEl> n) 
	{
	   an=cb->m_uiSize - cb->m_uiTotEl;	//empty space in the queue 
          if( an<=0) 
		return 0;
	//if number of elements to be inserted are more than the size of the
	//queue, we can only insert an elements.
	if(n > an ) {
		n=an;
	}

	//now insert the elements in the buffer
	//two steps

	//(1) first insert at the end of the queue
	//empty space from current position to the end of the queue
	unsigned int first = cb->m_uiSize - cb->m_uiTi; // first would be how mcuh left of queue
	unsigned int second = 0;
	start = cb->m_uiTi;

	if(n > first) 
		second = n-first;
	 else 
		first = n;

	memcpy(cb->m_buf+cb->m_uiTi, buf, first);  //copy the first
	cb->m_uiTi=(cb->m_uiTi+first)% cb->m_uiSize;    //compute the tail index

	//(2) if still remaining, insert at the beginning of the queue
	if(second > 0) {
		memcpy(cb->m_buf+cb->m_uiTi, buf+first, second);
		cb->m_uiTi=second;
	}//end if
	//end index
	end = cb->m_uiTi;
	cb->m_uiTotEl+=n;

	ret=  n; //number of bytes that could be inserted
	}
	return cb;

}

//Remove n bytes from the start of the queue. If n > size, then only remove n-size elements
int 
removeBytes(struct CircularBuffer *cb, int n)
{
	//if total number of elements are zero or n <= 0
  	if(0==cb->m_uiTotEl || n <= 0) 
		return 0;
	
if(n > cb->m_uiTotEl) {
		cb->m_uiHi = cb->m_uiTi;  //head index = current index
		n= cb->m_uiTotEl;      
		cb->m_uiTotEl=0;      //total number of elements is zero
		return n;         //return total number of elements removed
	} 

	cb->m_uiHi = (cb->m_uiHi+n)% cb->m_uiSize;//set the head index
	cb->m_uiTotEl = cb->m_uiTotEl - n;//update the total number of elements in the queue
	return n;
}


  //0 if 0<=n, or empty queue, Get n bytes from the start of the queue. returns the index of the queue where to fetch again
int 
getAt(struct CircularBuffer *cb, char* buf, int n, int end)
{
  
  //if no elements in the buffer
  if(0 >= n || 0== cb->m_uiTotEl) {
    return 0;
  }

  //if we are requesting more elements than there are in the queue,
  //adjust the size of the requested elements
  if(n > cb->m_uiTotEl) 
	  n = cb->m_uiTotEl;
  
  unsigned int first, second;
  first= cb->m_uiSize- cb->m_uiHi;  //head of the queue
  second=0;
  
  //there is a wrap around
  if(n > first) 
    second = n-first;
   else 
    first = n;
  
  memcpy(buf, cb->m_buf+ cb->m_uiHi, first);
  end=(cb->m_uiHi+first)% cb->m_uiSize;  
  
  //if there was a wrap around, copy the remaining buffer
  if(second > 0) {
    memcpy( buf+first, cb->m_buf, second);
    end=second;
  } 
  return n;
}

//Get the size which are available in the queue	
int 
getAvSize(struct CircularBuffer *cb)
{
    return cb->m_uiSize- cb->m_uiTotEl;
}

//get the total number of elements in queue
int 
getTotalElements(struct CircularBuffer *cb)
{
   return cb->m_uiTotEl;
}


