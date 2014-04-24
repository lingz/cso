/* Match every k-character snippet of the query_doc document
	 among a collection of documents doc1, doc2, ....

	 ./rkmatch snippet_size query_doc doc1 [doc2...]

*/

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <strings.h>
#include <assert.h>
#include <time.h>
#include <ctype.h>
#include "bloom.c"

enum algotype { SIMPLE = 0, RK, RKBATCH};

/* a large prime for RK hash (BIG_PRIME*256 does not overflow)*/
long long BIG_PRIME = 5003943032159437; 

/* constants used for printing debug information */
const int PRINT_RK_HASH = 5;
const int PRINT_BLOOM_BITS = 160;

/* modulo addition */
long long
madd(long long a, long long b)
{
	return ((a+b)>BIG_PRIME?(a+b-BIG_PRIME):(a+b));
}

/* modulo substraction */
long long
mdel(long long a, long long b)
{
	return ((a>b)?(a-b):(a+BIG_PRIME-b));
}

/* modulo multiplication*/
long long
mmul(long long a, long long b)
{
	return ((a*b) % BIG_PRIME);
}

/* read the entire content of the file 'fname' into a 
	 character array allocated by this procedure.
	 Upon return, *doc contains the address of the character array
	 *doc_len contains the length of the array
	 */
void
read_file(const char *fname, char **doc, int *doc_len) 
{
	struct stat st;
	int fd;
	int n = 0;

	fd = open(fname, O_RDONLY);
	if (fd < 0) {
		perror("read_file: open ");
		exit(1);
	}

	if (fstat(fd, &st) != 0) {
		perror("read_file: fstat ");
		exit(1);
	}

	*doc = (char *)malloc(st.st_size);
	if (!(*doc)) {
		fprintf(stderr, " failed to allocate %d bytes. No memory\n", (int)st.st_size);
		exit(1);
	}

	n = read(fd, *doc, st.st_size);
	if (n < 0) {
		perror("read_file: read ");
		exit(1);
	} else if (n != st.st_size) {
		fprintf(stderr,"read_file: short read!\n");
		exit(1);
	}
	
	close(fd);
	*doc_len = n;
}


/* The normalize procedure examines a character array of size len 
	 in ONE PASS and does the following:
	 1) turn all upper case letters into lower case ones
	 2) turn any white-space character into a space character and, 
	    shrink any n>1 consecutive spaces into exactly 1 space only
			Hint: use C library function isspace() 
	 You must do the normalization IN PLACE so that when the procedure
	 returns, the character array buf contains the normalized string and 
	 the return value is the length of the normalized string.
*/
int
normalize(char *buf,	/* The character array containing the string to be normalized*/
					int len			/* the size of the original character array */)
{
  /* Keep a backtrack variable to know how far back we need to skip back
   * to add characters. This compacts our string to remove consecutive whitespaces. */
  int backtrack = 0;
  int i = 0;
  /* leading whitespace */
  while (isspace(*(buf + i)))
    i++;
  for (i; i < len; i++)
  {
    /* Convert all non-space whitespaces to space */
    if (isspace(*(buf+i)))
    {
      /* If there are too consecutive spaces in a row, increase the backtrack
       * This is in effect, a skip, to make our string length shorter */
      if (!(isspace(*(buf + backtrack - 1))))
      {
        *(buf + backtrack++) = ' ';
      }
    }
    else 
    {
      *(buf + backtrack++) = tolower(*(buf + i));
    }
  }
  if (isspace(*(buf + backtrack - 1)))
    backtrack--;
  *(buf + backtrack) = '\0';
	return backtrack;
}

/* check if a query string ps (of length k) appears 
	 in ts (of length n) as a substring 
	 If so, return 1. Else return 0
	 You may want to use the library function strncmp
	 */
int
simple_match(const char *ps,	/* the query string */
						 int k, 					/* the length of the query string */
						 const char *ts,	/* the document string (Y) */ 
						 int n						/* the length of the document Y */)
{
  int i;
  /* move the pointer k steps forward each time, always verifying
     that there is enough space forward to continue searching */
  for (i = 0; i < n - k + 1; i++)
  {
    if (!strncmp(ps, (ts + i), k)) /* ! is the same as == 0 */
    {
      return 1;
    }
  }
	return 0;
}

/* Produces the initial rabin hash of length len at the start 
 of the string */
long long
rabin_hash(const char *to_encode, /* the string to encode */
           int len                /* the length of the encoded string */
           )
{
  long long hash = 0;
  int i;
  /* Iterate backwards over the range to encode it */
  for (i = 0; i < len; i++)
  {
    hash = madd(*(to_encode + i), mmul(hash, 256));
  }
  return hash;
}


/* Check if a query string ps (of length k) appears 
	 in ts (of length n) as a substring using the rabin-karp algorithm
	 If so, return 1. Else return 0
	 In addition, print the first 'PRINT_RK_HASH' hash values of ts
	 Example:
	 $ ./rkmatch -t 1 -k 20 X Y
   605818861882592 812687061542252 1113263531943837 1168659952685767
   4992125708617222 
	 0.01 matched: 1 out of 148
	 */
int
rabin_karp_match(const char *ps,	/* the query string */
								 int k, 					/* the length of the query string */
								 const char *ts,	/* the document string (Y) */ 
								 int n						/* the length of the document Y */ )
{
  /* The hash of the query string */
  int toPrint = PRINT_RK_HASH;
  long long check = rabin_hash(ps, k);
  if (n < k)
    return 0; /* fail if document is shorter than the query */
  long long query_hash = rabin_hash(ts, k);
  /* Compute 256 ^ k - 1 */
  long long hash_factor = 1;
  int i;
  for (i = 0; i < k - 1; i++)
  {
    hash_factor = mmul(hash_factor, 256);
  }
  /* roll the hash, comparing each time. We superfluously compute the last hash and discard the value. */
  for (i = 0; i < n - k + 1; i++)
  {
    if (toPrint-- > 0)    
    {
      printf("%lld", query_hash);
      if (toPrint == 0) 
      {
        printf("\n");
      }
      else
      {
        printf(" ");
      }
    }
    /* if the hashes match, then check the strings are actually equal. If so, return true */
    if (query_hash == check && !strncmp(ts + i, ps, k))
        return 1;
    /* rabin's algorithm, computing y_(i+1) */
    query_hash = madd(mmul(mdel(query_hash, mmul(*(ts + i), hash_factor)), 256), *(ts + k + i));
  }
	return 0;
}

/* Initialize the bitmap for the bloom filter using bloom_init().
	 Insert all m/k RK hashes of qs into the bloom filter using bloom_add().
	 Then, compute each of the n-k+1 RK hashes of ts and check if it's in the filter using bloom_query().
	 Use the given procedure, hash_i(i, p), to compute the i-th bloom filter hash value for the RK value p.

	 Return the number of matched chunks. 
	 Additionally, print out the first PRINT_BLOOM_BITS of the bloom filter using the given bloom_print 
	 after inserting m/k substrings from qs.
*/
int
rabin_karp_batchmatch(int bsz,        /* size of bitmap (in bits) to be used */
                      int k,          /* chunk length to be matched */
                      const char *qs, /* query docoument (X)*/
                      int m,          /* query document length */ 
                      const char *ts, /* to-be-matched document (Y) */
                      int n           /* to-be-matched document length*/)
{
  /*if the to-be-matched document is less than k length, return false*/
  if (n < k)
    return 0;
  /*start our filter, allocating the necessary memory*/
  bloom_filter filter = bloom_init(bsz);
  /*compute the hash factor.*/
  long long hash_factor = 1;
  int i;
  int j;
  for (i = 0; i < k - 1; i++)
  {
    hash_factor = mmul(hash_factor, 256);
  }
  int score = 0;
  for (i = 0 ; i < m / k ; i++)
  {
    /*add each length k chunk into the bloom filter.*/
    bloom_add(filter, rabin_hash(qs + i * k, k));
  }
  bloom_print(filter, PRINT_BLOOM_BITS);
  long long rolling_hash = rabin_hash(ts, k);
  for (i = 0 ; i < n - k + 1 ; i++)
  {
    /*If we get a match on the rolling hash, check that the substring exists.*/
    if (bloom_query(filter, rolling_hash))
    {
    /* iterate through each chunk doing a string compare */
      for (j = 0 ; j < m / k ; j++)
      {
        /*If we find a match, increment the count.*/
        if (!strncmp(qs + j * k, ts + i, k))
        {
          score++;
          break;
        }
      }
    }
    /*roll the hash forward.*/
    rolling_hash = madd(mmul(mdel(rolling_hash, mmul(*(ts + i), hash_factor)), 256), *(ts + k + i));
  }
  
	return score;
}

int 
main(int argc, char **argv)
{
  int k = 100; /*[> default match size is 100<]*/
  int which_algo = SIMPLE; /*[> default match algorithm is simple <]*/

  char *qdoc, *doc; 
  int qdoc_len, doc_len;
  int i;
  int num_matched = 0;
  int to_be_matched;
  int c;

  /*[> Refuse to run on platform with a different size for long long<]*/
  assert(sizeof(long long) == 8);

  /*[>getopt is a C library function to parse command line options <]*/
  while (( c = getopt(argc, argv, "t:k:q:")) != -1) {
    switch (c) 
    {
      case 't':
        /*optarg is a global variable set by getopt() */
          /*it now points to the text following the '-t' */
        which_algo = atoi(optarg);
        break;
      case 'k':
        k = atoi(optarg);
        break;
      case 'q':
        BIG_PRIME = atoi(optarg);
        break;
      default:
        fprintf(stderr,
            "Valid options are: -t <algo type> -k <match size> -q <prime modulus>\n");
        exit(1);
      }
  }

   /*optind is a global variable set by getopt() */
     /*it now contains the index of the first argv-element */
     /*that is not an option*/
  if (argc - optind < 1) {
    printf("Usage: ./rkmatch query_doc doc\n");
    exit(1);
  }

  /*[> argv[optind] contains the query_doc argument <]*/
  read_file(argv[optind], &qdoc, &qdoc_len); 
  qdoc_len = normalize(qdoc, qdoc_len);

  /*[> argv[optind+1] contains the doc argument <]*/
  read_file(argv[optind+1], &doc, &doc_len);
  doc_len = normalize(doc, doc_len);

  switch (which_algo) 
    {
      case SIMPLE:
         /*for each of the qdoc_len/k chunks of qdoc, */
           /*check if it appears in doc as a substring*/
        for (i = 0; (i+k) <= qdoc_len; i += k) {
          if (simple_match(qdoc+i, k, doc, doc_len)) {
            num_matched++;
          }
        }
        break;
      case RK:
         /*for each of the qdoc_len/k chunks of qdoc, */
           /*check if it appears in doc as a substring using */
           /*the rabin-karp substring matching algorithm */
        for (i = 0; (i+k) <= qdoc_len; i += k) {
          if (rabin_karp_match(qdoc+i, k, doc, doc_len)) {
            num_matched++;
          }
        }
        break;
      case RKBATCH:
        /*[> match all qdoc_len/k chunks simultaneously (in batch) by using a bloom filter<]*/
        num_matched = rabin_karp_batchmatch(((qdoc_len*10/k)>>3)<<3, k, qdoc, qdoc_len, doc, doc_len);
        break;
      default :
        fprintf(stderr,"Wrong algorithm type, choose from 0 1 2\n");
        exit(1);
    }
  
  to_be_matched = qdoc_len / k;
  printf("%.2f matched: %d out of %d\n", (double)num_matched/to_be_matched, 
      num_matched, to_be_matched);

  free(qdoc);
  free(doc);

  return 0;
}
