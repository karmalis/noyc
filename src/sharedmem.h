#ifndef SHAREDMEM_H_
#define SHAREDMEM_H_

#include <unistd.h>

static void randname(char *buf);
static int create_shm_file(void);
int allocate_shm_file(size_t size);

#endif // SHAREDMEM_H_
