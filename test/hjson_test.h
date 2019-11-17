#ifndef HJSONTEST_AENFOAWINFAWENFEFAWEFNWF
#define HJSONTEST_AENFOAWINFAWENFEFAWEFNWF


#undef assert

#define assert(expression) (void)( \
  (!!(expression)) || (printf("Test failure in file %s on line %d: %s\n", __FILE__, __LINE__, #expression), 0) || (exit(1), 0))


#endif
