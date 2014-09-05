#ifndef __PPD_SOCKET_SVR_H
#define __PPD_SOCKET_SVR_H

int ppd_socket_open(void);
int ppd_socket_write(int sock_fd, void *data);
int ppd_socket_close(int sock_fd);

#endif
