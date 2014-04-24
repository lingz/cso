/***********************************************************
 Implementation of bloom filter goes here 
 **********************************************************/

#include "bloom.h"

/* Constants for bloom filter implementation */
const int H1PRIME = 4189793;
const int H2PRIME = 3296731;
const int BLOOM_HASH_NUM = 10;

/* The hash function used by the bloom filter */
int
hash_i(int i, /* which of the BLOOM_HASH_NUM hashes to use */ 
       long long x /* a long long value to be hashed */)
{
	return ((x % H1PRIME) + i*(x % H2PRIME) + 1 + i*i);
}

/* Initialize a bloom filter by allocating a character array that can pack bsz bits.
   (each char represents 8 bits)
   Furthermore, clear all bits for the allocated character array. 
   Hint:  use the malloc and bzero library function 
	 Return value is the newly initialized bloom_filter struct.*/
bloom_filter 
bloom_init(int bsz /* size of bitmap to allocate in bits*/ )
{
	bloom_filter f;
	f.bsz = bsz;
  /*convert the memory size to bytes by dividing by 8 and adding one*/
  /*depending on if it is a multiple of 8.*/
  int memSize = (bsz >> 3) + (bsz % 8 ? 0 : 1);
  f.buf = (char *) malloc(memSize);
  memset(f.buf, 0, memSize);
	return f;
}

/* Add elm into the given bloom filter*/
void
bloom_add(bloom_filter f,
          long long elm /* the element to be added (a RK hash value) */)
{
  int i;
  int bloom_pos;
  /*For each element, turn on BLOOM_HASH_NUM to reduce the number*/
    /*of false positives.*/
  for (i = 0; i < BLOOM_HASH_NUM; i++)
  {
    /*calculate the bit position to switch on */
    bloom_pos = hash_i(i, elm) % f.bsz;
    /*shift a bit to the right position and write with a bitwise-or*/
    f.buf[bloom_pos / 8] |= 1 << (7 - bloom_pos % 8);
  }
	return;
}

/* Query if elm is probably in the given bloom filter */ 
int
bloom_query(bloom_filter f,
            long long elm /* the query element */ )
{	
  /*repeat the same loop, as bloom_add, checking*/
  int i;
  int bloom_pos;
  for (i = 0; i < BLOOM_HASH_NUM; i++)
  {
    /*hash the element and check if the bit is on*/
    bloom_pos = hash_i(i, elm) % f.bsz;
    /*bitwise-and the bit and check if the result contains any bits */
    if (!( (f.buf[bloom_pos / 8]) & (1 << (7 - bloom_pos % 8)) ))
      return 0;
  }
	return 1;
}

void 
bloom_free(bloom_filter *f)
{
	free(f->buf);
	f->buf = f->bsz = 0;
}

/* print out the first count bits in the bloom filter */
void
bloom_print(bloom_filter f,
            int count     /* number of bits to display*/ )
{
	int i;

	assert(count % 8 == 0);

	for(i=0; i< (f.bsz>>3) && i < (count>>3); i++) {
		printf("%02x ", (unsigned char)(f.buf[i]));
	}
	printf("\n");
	return;
}

