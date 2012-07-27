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
	
	int receive_fd(void);
	void send_fd(int fd);
	
#ifdef  __cplusplus
}
#endif
