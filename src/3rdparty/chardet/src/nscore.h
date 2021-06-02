#ifndef INCLUDED_NSCORE_H
#define INCLUDED_NSCORE_H

#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>

typedef uint32_t nsresult;

#define PR_Malloc(size) malloc(size)
#define PR_Free(size) free(size)
#define PR_FREEIF(ptr) if (ptr) { free(ptr); (ptr) = 0; }

#define nsnull 0
#define NS_OK 0
#define NS_ERROR_OUT_OF_MEMORY ((nsresult)(0x8007000eL))

#endif /* INCLUDED_NSCORE_H */
