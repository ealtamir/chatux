#ifndef AUXHEADER_H
#define AUXHEADER_H

#include <sys/types.h>  /* Defines basic POSIX data types */
#include <sys/stat.h>   /* Contains permission constants */

#include <stdio.h>      /* */
#include <stdlib.h>

#include <unistd.h>     /* Prototypes for many system calls */
#include <errno.h>      /* Declares errno and defines error constants */
#include <string.h>

#include <assert.h>

//#include "get_num.h"

//#include "error_functions.h"

#ifdef TRUE
#undef TRUE
#endif

#ifdef FALSE
#undef FALSE
#endif

typedef enum  {
    FALSE, TRUE
} Boolean;

typedef enum  {
    FALSE, TRUE
} bool;

#define min(m,n) ((m) < (n) ? (m) : (n))
#define max(m,n) ((m) < (n) ? (m) : (n))

#endif
