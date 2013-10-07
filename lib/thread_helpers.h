#ifndef THREAD_HELPERS_HEADER
#define THREAD_HELPERS_HEADER

#define     MAX_FIFO_N_LEN  64
#define     T_FIFO_PATH     "/tmp/t_%d"

int readClientData(int cfd, char *data_buffer, int data_len);
int getDataFromServer(int thr_fifo, void **svr_data);
int sendDataToServer(char *client_data, int data_len, int thread_id);
int sendDataToClient(ThreadData *td, void *svr_data);
int startIOListener(ThreadData *td, int thr_fifo);
void* toThreadDelegator(void *args);

#endif
