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

#define nf_stderr(text)	\
	(((long CDECL (*)(long, const char *))&_NF_call)(nfGetID(("NF_STDERR")), (text)))

#define nf_stderrprintf(text, par1)	\
	(((long CDECL (*)(long, const char *, ...))&_NF_call)(nfGetID(("NF_STDERR"))+1, text, par1))

/* Example code */
int main()
{
	char buf[80]="";
	long version;

	nf_getFullName(buf, sizeof(buf));
	printf("Machine name and version: '%s'\n", buf);

	version = nf_getVersion();

	printf("NatFeat API version = %d.%d\n", (int)(version >> 16), (int)(version & 0xffff));
	
	nf_stderr("Debug output\n");

	nf_stderrprintf("Debug output with one decimal parameter: %d\n", (unsigned long)105);
	nf_stderrprintf("Debug output with one string parameter: '%s'\n", "Hello world!");

	getchar();
	return 0;
}
