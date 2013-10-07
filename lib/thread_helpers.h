#ifndef THREAD_HELPERS_HEADER
#define THREAD_HELPERS_HEADER

#define     MAX_FIFO_N_LEN  64
#define     T_FIFO_PATH     "/tmp/t_%d"

void* toThreadDelegator(void *args);

#endif
