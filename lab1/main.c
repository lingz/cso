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
