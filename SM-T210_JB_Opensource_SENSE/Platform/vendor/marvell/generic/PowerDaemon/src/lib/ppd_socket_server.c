#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <poll.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <time.h>
#include <unistd.h>
#include "ppd.h"
#include "ppd_socket_server.h"

int ppd_socket_open(void){
	int client_fd;
	socklen_t alen;
	struct sockaddr_un sck;

	sck.sun_family = AF_UNIX;
	/* Use an abstract socket address */
	sck.sun_path[0] = '\0';
	alen = (socklen_t) strlen((const char*) PPD_SOCKET_SVR);
	if (alen > 107)
		alen = 107;
	strncpy(sck.sun_path + 1, PPD_SOCKET_SVR, (size_t) alen);
	sck.sun_path[107] = '\0';
	alen += (socklen_t)(1 + offsetof(struct sockaddr_un, sun_path));

	if (((client_fd = socket(PF_UNIX, SOCK_STREAM, 0)) == -1)) {
		return -1;
	}

	if (connect(client_fd, (struct sockaddr *) &sck, alen) == -1) {
		close(client_fd);
		return -1;
	}

	return client_fd;
}

int ppd_socket_write(int sock_fd, void *data) {
    int ret = -1, data_len;

	if ((sock_fd > 0)&&(data)) {
        data_len = *((int *)data + 1);
        data_len += 2*sizeof(int);
		ret = write(sock_fd, data, data_len);

    }
	return ret;

}

int ppd_socket_close(int sock_fd) {
    int ret = 0;

    if (sock_fd > 0)
        ret = close(sock_fd);
    return ret;
}


