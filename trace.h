#ifndef LIBRDX_TRACE_H
#define LIBRDX_TRACE_H

#include <stdio.h>

#ifndef ABC_NOTRACE
#define trace(...) fprintf(stderr, __VA_ARGS__)
#else
#define trace(...) ;
#endif

#endif
