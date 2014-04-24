/***********************************************************
 File Name: rkmatch.h
 Description: definition of rkmatching functions
 **********************************************************/

int normalize(char *buf, int len);
int simple_match(const char *ps, int k, const char *ts, int n);
long long rabin_hash(const char *to_encode, int len);
int rabin_karp_match(const char *ps, int k, const char *ts, int n); 
