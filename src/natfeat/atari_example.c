#include <stdio.h>
#include <tos.h>

/* NatFeat common defines */
#if __PUREC__
#define CDECL cdecl
#else
#define CDECL
#endif

typedef struct
{
	long magic;
	long CDECL (* nfGetID)(const char *);
	long CDECL (* nfCall)(long ID, ...);
} NatFeatCookie;

NatFeatCookie *nf_ptr;	/* must point to the NatFeat cookie */

long nfidGetName(void)
{
	static long id = 0;
	if (id == 0)
		id = nf_ptr->nfGetID("NF_NAME");
	return id;
}

long nfGetName(char *buffer, long size)
{
	return nf_ptr->nfCall(nfidGetName(), buffer, size);
}

long nfGetFullName(char *buffer, long size)
{
	return nf_ptr->nfCall(nfidGetName() | 0x0001, buffer, size);
}

long nfGetVersion(void)
{
	static long id = 0;
	if (id == 0)
		id = nf_ptr->nfGetID("NF_VERSION");

	return nf_ptr->nfCall(id);
}	

long nfidStdErr(void)
{
	static long id = 0;
	if (id == 0)
		id = nf_ptr->nfGetID("NF_STDERR");
	return id;
}

long nfStdErr(char *text)
{
	return nf_ptr->nfCall(nfidStdErr(), text);
}

#define nfStdErrPrintf1(format, a)	nf_ptr->nfCall(nfidStdErr() | 0x0001, format, a)
#define nfStdErrPrintf2(format, a, b)	nf_ptr->nfCall(nfidStdErr() | 0x0001, format, a, b)
#define nfStdErrPrintf3(format, a, b, c)	nf_ptr->nfCall(nfidStdErr() | 0x0001, format, a, b, c)
#define nfStdErrPrintf4(format, a, b, c, d)	nf_ptr->nfCall(nfidStdErr() | 0x0001, format, a, b, c, d)

typedef struct
{
    long cookie;
    long value;
} COOKIE;

int getcookie( long target, long *p_value )
{
    char *oldssp;
    COOKIE *cookie_ptr;

    oldssp = (char *)Super(0L);

    cookie_ptr = *(COOKIE **)0x5A0;

    if(oldssp)
        Super( oldssp );

    if(cookie_ptr != NULL)
    {
        do
        {
            if(cookie_ptr->cookie == target)
            {
                if(p_value != NULL)
                    *p_value = cookie_ptr->value;

                return 1;
            }
        } while((cookie_ptr++)->cookie != 0L);
    }

    return 0;
}


/* Example code */
int main()
{
	char buf[80]="";
	long version;
	
	if (getcookie(0x5f5f4e46L, &(long)nf_ptr) == 0) {
		puts("NatFeat cookie not found");
		return 0;
	}
	
	if (nf_ptr->magic != 0x20021021L) {
		puts("NatFeat cookie magic value does not match");
		return 0;
	}

	nfGetFullName(buf, sizeof(buf));
	printf("Machine name and version: '%s'\n", buf);

	version = nfGetVersion();

	printf("NatFeat API version = %d.%d\n", (int)(version >> 16), (int)(version & 0xffff));
	
	nfStdErr("Plain debug output\n");

	nfStdErrPrintf1("Debug output with one decimal parameter: %d\n", (unsigned long)105);
	nfStdErrPrintf2("Debug output with string and number: '%s' is %d bytes long\n", "Hello world!", (long)sizeof("Hello world!"));

	getchar();
	return 0;
}
