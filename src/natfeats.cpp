/**
 * NatFeats (Native Features main dispatcher)
 *
 * Petr Stehlik (c) 2002-2005
 *
 * GPL
 */

#include "natfeats.h"
#include "nf_objs.h"
#include "maptab.h"
#ifdef OS_darwin
#include <CoreFoundation/CoreFoundation.h>
#endif

#define DEBUG 0
#include "debug.h"

#define ID_SHIFT	20
#define IDX2MASTERID(idx)	(((idx)+1) << ID_SHIFT)
#define MASTERID2IDX(id)	(((id) >> ID_SHIFT)-1)
#define MASKOUTMASTERID(id)	((id) & ((1L << ID_SHIFT)-1))

static memptr context = 0;

uint32 nf_get_id(memptr stack)
{
	char name[80];
	memptr name_ptr = ReadInt32(stack);
	Atari2HostSafeStrncpy(name, name_ptr, sizeof(name));
	D(bug("nf_get_id '%s'", name));

	for(unsigned int i=0; i < nf_objs_cnt; i++) {
		if (strcasecmp(name, nf_objects[i]->name()) == 0) {
			D(bug("Found the NatFeat at %d", i));
			return IDX2MASTERID(i);
		}
	}

	return 0;		/* ID with given name not found */
}

int32 nf_call(memptr stack, bool inSuper)
{
	uint32 fncode = ReadInt32(stack);
	unsigned int idx = MASTERID2IDX(fncode);
	if (idx >= nf_objs_cnt) {
		D(bug("nf_call: wrong ID %d", idx));
		return 0;	/* returning an undefined value */
	}

	fncode = MASKOUTMASTERID(fncode);
	context = stack + 4;	/* parameters follow on the stack */

	NF_Base *obj = nf_objects[idx];
	if (strcmp(obj->name(), "NF_STDERR") != 0)
	{
		D(bug("nf_call(%s, %d)", obj->name(), fncode));
	}

	if (obj->isSuperOnly() && !inSuper) {
		panicbug("nf_call(%s, %d): privilege violation", obj->name(), fncode);
		THROW(8);	// privilege exception
	}

	return obj->dispatch(fncode);
}

uint32 nf_getparameter(int i)
{
	if (i < 0)
		return 0;

	return ReadInt32(context + i*4);
}


void Atari2HostUtf8Copy(char *dst, memptr src, size_t count)
{
	unsigned short ch;
	
	while ( count > 1)
	{
		ch = ReadNFInt8(src++);
		if (ch == 0)
			break;
		ch = atari_to_utf16[ch];
		if (ch < 0x80)
		{
			*dst++ = ch;
			count--;
		} else if (ch < 0x800 || count < 3)
		{
			*dst++ = ((ch >> 6) & 0x3f) | 0xc0;
			*dst++ = (ch & 0x3f) | 0x80;
			count -= 2;
		} else 
		{
			*dst++ = ((ch >> 12) & 0x0f) | 0xe0;
			*dst++ = ((ch >> 6) & 0x3f) | 0x80;
			*dst++ = (ch & 0x3f) | 0x80;
			count -= 3;
		}
	}
	if (count > 0)
		*dst = '\0';
}


void Host2AtariUtf8Copy(memptr dst, const char *src, size_t count)
{
#ifdef OS_darwin
	/* MacOSX uses decomposed strings, normalize them first */
	CFMutableStringRef theString = CFStringCreateMutable(NULL, 0);
	CFStringAppendCString(theString, src, kCFStringEncodingUTF8);
	CFStringNormalize(theString, kCFStringNormalizationFormC);
	UniChar ch;
	unsigned short c;
	CFIndex idx;
	CFIndex len = CFStringGetLength(theString);
	
	idx = 0;
	while ( count > 1 && idx < len )
	{
		ch = CFStringGetCharacterAtIndex(theString, idx);
		c = utf16_to_atari[ch];
		if (c >= 0x100)
		{
			charset_conv_error(ch);
			/* not convertible. return utf8-sequence to avoid producing duplicate filenames */
			if (ch < 0x80)
			{
				WriteNFInt8(dst++, ch);
				count--;
			} else if (ch < 0x800 || count < 3)
			{
				WriteNFInt8(dst++, ((ch >> 6) & 0x3f) | 0xc0);
				WriteNFInt8(dst++, (ch & 0x3f) | 0x80);
				count -= 2;
			} else 
			{
				WriteNFInt8(dst++, ((ch >> 12) & 0x0f) | 0xe0);
				WriteNFInt8(dst++, ((ch >> 6) & 0x3f) | 0x80);
				WriteNFInt8(dst++, (ch & 0x3f) | 0x80);
				count -= 3;
			}
		} else
		{
			WriteNFInt8( dst++, c );
			count -= 1;
		}
		idx++;
	}
	if (count > 0)
	{
		WriteNFInt8( dst, 0 );
	}
	CFRelease(theString);
#else
	unsigned short ch;
	unsigned short c;
	size_t bytes;
	
	while ( count > 1 && *src )
	{
		c = (unsigned char) *src;
		ch = c;
		if (ch < 0x80)
		{
			bytes = 1;
		} else if ((ch & 0xe0) == 0xc0 || count < 3)
		{
			ch = ((ch & 0x1f) << 6) | (src[1] & 0x3f);
			bytes = 2;
		} else
		{
			ch = ((((ch & 0x0f) << 6) | (src[1] & 0x3f)) << 6) | (src[2] & 0x3f);
			bytes = 3;
		}
		c = utf16_to_atari[ch];
		if (c >= 0x100)
		{
			charset_conv_error(ch);
			/* not convertible. return utf8-sequence to avoid producing duplicate filenames */
			WriteNFInt8(dst++, *src++);
			if (bytes >= 2)
				WriteNFInt8(dst++, *src++);
			if (bytes >= 3)
				WriteNFInt8(dst++, *src++);
			count -= bytes;
		} else
		{
			WriteNFInt8( dst++, c );
			src += bytes;
			count -= 1;
		}
	}
	if (count > 0)
	{
		WriteNFInt8( dst, 0 );
	}
#endif
}

void charset_conv_error(unsigned short ch)
{
	bug("cannot convert $%04x to atari codeset", ch);
}

/*
vim:ts=4:sw=4:
*/
