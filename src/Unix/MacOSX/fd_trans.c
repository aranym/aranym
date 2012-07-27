/*
 *  fd_trans.c
 *  Used to transfer a file descriptor between two Unix processes
 *
 *  Inspired by Amit Singhs description in the "Mac OS X manual": 
 *  http://topiks.org/mac-os-x/0321278542/ch09lev1sec11.html
 *
 */

#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/uio.h>


#include "fd_trans.h"

#define SOCKET_PATH "/tmp/fd_trans_socket"

#define MSG_CONTROL_SIZE CMSG_SPACE(sizeof(int))

// Prototypes
int receive_fd_using_sockfd(int *fd, int sockfd);
int setup_server(const char *name);
int send_fd_using_sockfd(int fd, int sockfd);



int receive_fd_using_sockfd(int *fd, int sockfd)
{
    ssize_t                ret;
    u_char                 c;
    int                    errcond = 0;
    struct iovec           iovec[1];
    struct msghdr          msg;
    struct cmsghdr        *cmsghdrp;
    union {
        struct cmsghdr cmsghdr;
        u_char         msg_control[MSG_CONTROL_SIZE];
    }  cmsghdr_msg_control;
	
    iovec[0].iov_base = (void*)&c;
    iovec[0].iov_len = 1;
	
    msg.msg_name = (caddr_t)0;
    msg.msg_namelen = 0;
    msg.msg_iov = iovec;
    msg.msg_iovlen = 1;
    msg.msg_control = (void*)cmsghdr_msg_control.msg_control;
    msg.msg_controllen = sizeof(cmsghdr_msg_control.msg_control);
    msg.msg_flags = 0;
	
    if ((ret = recvmsg(sockfd, &msg, 0)) <= 0) 
	{
        perror("recvmsg");
        return (int)ret;
    }
	
    cmsghdrp = CMSG_FIRSTHDR(&msg);
    if (cmsghdrp == NULL) 
	{
        *fd = -1;
        return (int)ret;
    }
	
    if (cmsghdrp->cmsg_len != CMSG_LEN(sizeof(int)))
        errcond++;
	
    if (cmsghdrp->cmsg_level != SOL_SOCKET)
        errcond++;
	
    if (cmsghdrp->cmsg_type != SCM_RIGHTS)
        errcond++;
	
    if (errcond) 
	{
        fprintf(stderr, "receiver: %d errors in received message\n", errcond);
        *fd = -1;
    } else
        *fd = *((int *)CMSG_DATA(cmsghdrp));
	
    return (int)ret;
}


int setup_server(const char *name)
{
    int sockfd;
	socklen_t len;
    struct sockaddr_un server_unix_addr;
	
    if ((sockfd = socket(AF_LOCAL, SOCK_STREAM, 0)) < 0) {
        perror("socket");
        return sockfd;
    }
	
    unlink(name);
    bzero((char *)&server_unix_addr, sizeof(server_unix_addr));
    server_unix_addr.sun_family = AF_LOCAL;
    strcpy(server_unix_addr.sun_path, name);
    len = (socklen_t)strlen(name) + 1;
    len += sizeof(server_unix_addr.sun_family);
	
    if (bind(sockfd, (struct sockaddr *)&server_unix_addr, len) < 0) {
        close(sockfd);
        return -1;
    }
	
    return sockfd;
}


int send_fd_using_sockfd(int fd, int sockfd)
{
    ssize_t                ret;
    struct iovec           iovec[1];
    struct msghdr          msg;
    struct cmsghdr        *cmsghdrp;
    union {
        struct cmsghdr cmsghdr;
        u_char         msg_control[MSG_CONTROL_SIZE];
    } cmsghdr_msg_control;
	
    iovec[0].iov_base = "";
    iovec[0].iov_len = 1;
	
    msg.msg_name = (caddr_t)0; // address (optional)
    msg.msg_namelen = 0;       // size of address
    msg.msg_iov = iovec;       // scatter/gather array
    msg.msg_iovlen = 1;        // members in msg.msg_iov
    msg.msg_control = (void*)cmsghdr_msg_control.msg_control; // ancillary data
    // ancillary data buffer length
    msg.msg_controllen = sizeof(cmsghdr_msg_control.msg_control);
    msg.msg_flags = 0;          // flags on received message
	
    // CMSG_FIRSTHDR() returns a pointer to the first cmsghdr structure in
    // the ancillary data associated with the given msghdr structure
    cmsghdrp = CMSG_FIRSTHDR(&msg);
	
    cmsghdrp->cmsg_len = CMSG_LEN(sizeof(int)); // data byte count
    cmsghdrp->cmsg_level = SOL_SOCKET;          // originating protocol
    cmsghdrp->cmsg_type = SCM_RIGHTS;           // protocol-specified type
	
    // CMSG_DATA() returns a pointer to the data array associated with
    // the cmsghdr structure pointed to by cmsghdrp
    *((int *)CMSG_DATA(cmsghdrp)) = fd;
	
    if ((ret = sendmsg(sockfd, &msg, 0)) < 0) {
        perror("sendmsg");
        return (int)ret;
    }
	
    return 0;
}


void send_fd(int fd)
{
    int sockfd, len;
    struct sockaddr_un server_unix_addr;
    bzero((char *)&server_unix_addr, sizeof(server_unix_addr));
    strcpy(server_unix_addr.sun_path, SOCKET_PATH);
    server_unix_addr.sun_family = AF_LOCAL;
    len = strlen(SOCKET_PATH) + 1;
    len += sizeof(server_unix_addr.sun_family);
	
    if ((sockfd = socket(AF_LOCAL, SOCK_STREAM, 0)) < 0) {
        perror("socket");
        return;
    }
	
	int i, ret = -1;
	for(i=1; i<3 && ret<0; i++) {
		ret = connect(sockfd, (struct sockaddr *)&server_unix_addr, len);
		if (ret < 0) {
			perror("sender: connect");
			sleep(1);
		}
	}
	if (ret >= 0) 
	{
		// Send file descriptor message 
		if ((send_fd_using_sockfd(fd, sockfd) < 0))
			fprintf(stderr, "sender: failed to send file descriptor (fd = %d)\n", fd);
	}
	else {
		perror("sender: connect");
	}
	sleep(1);
	
	close(sockfd);
}


int receive_fd(void)
{
    int fd = -1, sockfd;
	if ((sockfd = setup_server(SOCKET_PATH)) < 0) 
	{
		fprintf(stderr, "receiver: failed to create socket.\n");
		return fd;
	}		
	
	// Wait until a connection is established 
	listen(sockfd, 0);

	int i;
	for (i=1;i<3 && fd<0; i++) 
	{
		struct sockaddr_un cl_un;
		socklen_t cl_un_len = sizeof(cl_un);
		int csockfd = accept(sockfd, (struct sockaddr *)&cl_un, &cl_un_len);
		if (csockfd < 0) 
		{
			perror("accept");
			sleep(1);
		}
		else
		{
			int ret = receive_fd_using_sockfd(&fd, csockfd);
			if ((ret < 0) || (fd < 0))
				fprintf(stderr, "receiver: failed to receive file descriptor\n");
		}

		close(csockfd);
		sleep(1);
	}
	close(sockfd);

	return fd;
}