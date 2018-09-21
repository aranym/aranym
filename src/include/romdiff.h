
#ifndef _romdiff_h
#define _romdiff_h

typedef struct {
	unsigned int start;
	unsigned int value;
	int len;
	const unsigned char *patch;
} ROMdiff;

extern ROMdiff const tosdiff[];

#endif	/* !_romdiff_h */
