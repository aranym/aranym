/*
 * This is the header file for the MD5 message-digest algorithm.
 * The algorithm is due to Ron Rivest.  This code was
 * written by Colin Plumb in 1993, no copyright is claimed.
 * This code is in the public domain; do with it what you wish.
 *
 * Equivalent code is available from RSA Data Security, Inc.
 * This code has been tested against that, and is equivalent,
 * except that you don't need to include two pages of legalese
 * with every copy.
 *
 * To compute the message digest of a chunk of bytes, declare an
 * MD5Context structure, pass it to MD5Init, call MD5Update as
 * needed on buffers full of bytes, and then call MD5Final, which
 * will fill a supplied 16-byte array with the digest.
 *
 * Changed so as no longer to depend on Colin Plumb's `usual.h'
 * header definitions; now uses stuff from dpkg's config.h
 *  - Ian Jackson <ijackson@nyx.cs.du.edu>.
 * Still in the public domain.
 */

#ifndef MD5_H
#define MD5_H

# include <cstdio>

#define md5byte unsigned char
#define UWORD32	unsigned long

struct MD5Context {
	UWORD32 buf[4];
	UWORD32 bytes[2];
	UWORD32 in[16];
};

class MD5
{
private:
	struct MD5Context *context;

public:
	MD5()	{ this->context = new MD5Context; }
	~MD5()	{ delete this->context; }

	void computeSum(md5byte const *buf, unsigned long size, unsigned char digest[16]);
	bool compareSum(md5byte const *buf, unsigned long size, const unsigned char digest[16]);
	void printSum(const unsigned char digest[16]);

	bool computeSum(FILE *f, unsigned char digest[16]);
	bool compareSum(FILE *f, const unsigned char digest[16]);
	bool printSum(FILE *f);

protected:
	void MD5Init();
	void MD5Update(md5byte const *buf, unsigned len);
	void MD5Final(unsigned char digest[16]);

private:
	void MD5Transform(UWORD32 buf[4], UWORD32 const in[16]);

};

#endif /* !MD5_H */
