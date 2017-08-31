/*
 *  fd_trans.h
 *  Used to transfer a file descriptor between two Unix processes
 *
 *  Inspired by Amit Singhs description in the "Mac OS X manual": 
 *  http://topiks.org/mac-os-x/0321278542/ch09lev1sec11.html
 *
 */

#ifdef  __cplusplus
extern "C" {
#endif
	
int fd_receive(int sockfd, pid_t from);
void fd_send(int fd);
int fd_setup_server(void);

#ifdef  __cplusplus
}
#endif
