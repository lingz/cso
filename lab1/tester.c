/* Unit tests for the rkmatch.c file 
   main() in the rkmatch.c file must be commented out */

#include "rkmatch.h"
#include <stdio.h>
#include <string.h>

int
main(int argc, char **argv)
{
  /* Testing whitespace replacement */
  char testString[] = "This   is   my\v\ftest\fstring\nwhich has lots of artifacts\n";
  printf("Test String:\n%s\n", testString);
  int len = strlen(testString);
  normalize(testString, len);
  printf("Normalized String:\n%s\n", testString);

  if (strcmp(testString, "this is my test string which has lots of artifacts ") == 0)
  {
    printf("Normalize was successful!\n\n");
  }
  else
  {
    printf("Normalize was unsuccessful!\n\n");
  }

  /* Testing simple match */
  char testString2[] = "The quick brown fox jumps over the lazy dog";
  int len2 = strlen(testString2);
  char query[] = "own";
  printf("Test String:\n%s\n", testString2);
  printf("Query;\n%s\n", query);
  int result = simple_match(query, 3, testString2, len2);
  printf("The score was: %d\n", result);

  char query2[] = "quickly";
  printf("Query;\n%s\n", query2);
  int result2 = simple_match(query2, 7, testString2, len2);
  printf("The score was: %d\n", result2);

  if (result == 1 && result2 == 0)
  {
    printf("Simple match was successful!\n\n");
  }
  else
  {
    printf("Simple match was unsuccessful...\n\n");
  }

  /* Testing initial rabin karp encoding */
  char testString3[] = "this is encoded";
  int len3 = 4;
  long long result3 = rabin_hash(testString3, len3);
  printf("Rabin Hash of Encoded:\n%lld\n\n", result3);

  if (result3 == 1952999795)
  {
    printf("Rabin hash was successful!\n\n");
  }
  else
  {
    printf("Rabin hash was unsuccessful...\n\n");
  }

  /* Testing rabin's rolling hash */
  char testString4[] = "this is the target";
  char query3[] = "his";
  int result4 = rabin_karp_match(query3, 3, testString4, strlen(testString4)); 
  printf("Rabin rolling hash of '%s' on '%s'.\n", query3, testString4);
  printf("Result: %d\n\n", result4);
  char query4[] = "tag";
  int result5 = rabin_karp_match(query4, 3, testString4, strlen(testString4)); 
  printf("Rabin rolling hash of '%s' on '%s'.\n", query4, testString4);
  printf("Result: %d\n\n", result5);

  if (result4 == 1 && result5 == 0)
  {
    printf("Rabin rolling match was successful!\n\n");
  }
  else
  {
    printf("Rabin rolling match was unsuccessful...\n\n");
  }

  return 0;
}
