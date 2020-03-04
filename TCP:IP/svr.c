#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/select.h>

#define MAX_FDS	100

void (*callback[MAX_FDS])(int s);

void got_input(int s){
	char buf[512];

	int n = read(s, buf, sizeof(buf));
	if (n >= 0) {
		printf("GOT '%.*s'\n", n, buf);
		for (int fd = 0; fd < MAX_FDS; fd++) {
			if (callback[fd] == got_input && fd != s) {
				write(fd, buf, n);
			}
		}
	}
	if (n <= 0) {
		callback[s] = 0;
		close(s);
	}
}

void got_client(int s){
	struct sockaddr_in addr;
	socklen_t len = sizeof(addr);

	int clt = accept(s, (struct sockaddr *) &addr, &len);
	assert(clt > 0);

	printf("GOT A CLIENT\n");
	callback[clt] = got_input;
}

int main(){
	printf("hello world\n");

	int s = socket(PF_INET, SOCK_STREAM, 0);
	assert(s > 0);

	int enable = 1;
	setsockopt(s, 0, SO_REUSEADDR, &enable, sizeof(enable));

	callback[s] = got_client;

	struct sockaddr_in addr;
	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(5555);
	addr.sin_addr.s_addr = INADDR_ANY;

	int r = bind(s, (struct sockaddr *) &addr, sizeof(addr));
	assert(r == 0);

	r = listen(s, 10);
	assert(r == 0);

	for (;;) {
		fd_set fds;
		FD_ZERO(&fds);

		for (int fd = 0; fd < MAX_FDS; fd++) {
			if (callback[fd] != 0) {
				FD_SET(fd, &fds);
			}
		}

		int n = select(MAX_FDS, &fds, 0, 0, 0);
		printf("GOT SOME INPUT\n");

		for (int fd = 0; fd < MAX_FDS; fd++) {
			if (FD_ISSET(fd, &fds)) {
				(*callback[fd])(fd);
			}
		}
	}


	return 0;
}
