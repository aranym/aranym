/*
 * $Header$
 *
 * ARAnyM native features interface.
 *
 **/
#ifndef _natfeat_h_
#define _natfeat_h_

#warning "This header files is now deprecated! Use the nf_ops.h instead!"

#ifndef CDECL
#  if __PUREC__
#    define CDECL cdecl
#  else
#    define CDECL
#  endif
#endif


extern long _NF_getid;
extern long _NF_call;


/* NatFeat common defines */
#define nfGetID(n)  (((long CDECL (*)(const char *))&_NF_getid)n)
#define nfCall(n)   (((long CDECL (*)(long, ...))&_NF_call)n)


/* NatFeat functions defined */
#define nf_getName(buffer, size) \
	(((long CDECL (*)(long, char *, unsigned long))&_NF_call)(nfGetID(("NF_NAME")), (buffer), (unsigned long)(size)))

#define nf_getFullName(buffer, size) \
	(((long CDECL (*)(long, char *, unsigned long))&_NF_call)(nfGetID(("NF_NAME"))+1, (buffer), (unsigned long)(size)))

#define nf_getVersion()	\
	(((long CDECL (*)(long, ...))&_NF_call)(nfGetID(("NF_VERSION"))))

#define nf_stderr(text)	\
	(((long CDECL (*)(long, const char *))&_NF_call)(nfGetID(("NF_STDERR")), (text)))

#define nf_stderrprintf(text, par1)	\
	(((long CDECL (*)(long, const char *, ...))&_NF_call)(nfGetID(("NF_STDERR"))+1, text, par1))


#endif /* _natfeat_h_ */
