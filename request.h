#ifndef __REQUEST_H__

void requestHandle(int fd);

void requestHandleInitial (int fd, work_queue_t* job);

void requestHandleFinal ( work_queue_t* job);
#endif
