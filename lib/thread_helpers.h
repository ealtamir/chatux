#ifndef THREAD_HELPERS_HEADER
#define THREAD_HELPERS_HEADER


int readClientData(int cfd, char *data_buffer, int data_len);
int getDataFromServer(int *pipe_fd, void *svr_data);
int sendDataToServer(char *client_data, int data_len, int *pipe_fd);
int sendDataToClient(ThreadData *td, void *svr_data);
int startIOListener(ThreadData *td, int *pipe_fd);
void* toThreadDelegator(void *args);

#endif
