#include <stdio.h>

/* NatFeat common defines */
#if __PUREC__
#define CDECL cdecl
#else
#define CDECL
#endif

static long _NF_getid = 0x73004e75L;
static long _NF_call  = 0x73014e75L;
#define nfGetID(n)	(((long CDECL (*)(const char *))&_NF_getid)n)
#define nfCall(n)	(((long CDECL (*)(long, ...))&_NF_call)n)


/* NatFeat functions defined */
#define nf_getName(buffer, size) \
	(((long CDECL (*)(long, char *, unsigned long))&_NF_call)(nfGetID(("NF_NAME")), (buffer), (unsigned long)(size)))

#define nf_getFullName(buffer, size) \
	(((long CDECL (*)(long, char *, unsigned long))&_NF_call)(nfGetID(("NF_NAME"))+1, (buffer), (unsigned long)(size)))

#define nf_getVersion()	\
	(((long CDECL (*)(long, ...))&_NF_call)(nfGetID(("NF_VERSION"))))


/* Example code */
int main()
{
	char buf[80]="";
	long version;

	nf_getFullName(buf, sizeof(buf));
	printf("NF_NAME=%s\n", buf);

	version = nf_getVersion();

	printf("NF_VERSION=%d.%d\n", (int)(version >> 16), (int)(version & 0xffff));

	getchar();
	return 0;
}
