/* Joy 2001 */

#include "sysdeps.h"
#include "hardware.h"
#include "cpu_emulation.h"
#include "memory.h"
#include "acia.h"

#define DEBUG 0
#include "debug.h"

static const int HW = 0xfffc00;

ACIA::ACIA(uaecptr addr) {
	baseaddr = addr;
}

uae_u8 ACIA::handleRead(uaecptr addr) {
	addr -= baseaddr;
	if (addr == 0)
		return getStatus();
	else if (addr == 2)
		return getData();
	else
		return 0;
}

void ACIA::handleWrite(uaecptr addr, uae_u8 value) {
	addr -= baseaddr;
	if (addr == 0)
		setMode(value);
	else if (addr == 2)
		setData(value);
}

/*******************************/
IKBD::IKBD() : ACIA(0xfffc00) {
	status = 0x0e;
	ikbd_inbuf = ikbd_bufpos = 0;
	inTransmit = false;
	rwLock = SDL_CreateMutex();
	inGet = false;
};
IKBD::~IKBD() {
	SDL_DestroyMutex(rwLock);
}

bool IKBD::isBufferEmpty() {
	return ikbd_inbuf == 0;
}

uae_u8 IKBD::getStatus() {
	return status | 0x02;
}

void IKBD::setMode(uae_u8 value) {
}

#define SC_MOUSEMOVE 0xf8

void IKBD::compressMouseMove( int &pos ) {
	if( ikbd_inbuf < 6 )
		return;

	int currpack = buffer[pos];
	int x = (int) (char) buffer[pos+1];
	int y = (int) (char) buffer[pos+2];
	char currx, curry;
	
	while (ikbd_inbuf >= 6
	       && pos+6 < MAXBUF
	       && buffer[pos+3] == currpack) {
		currx = (char) buffer[pos+4];
		curry = (char) buffer[pos+5];
		if (abs(x + (int) currx) > 63
		    || abs(y + (int) curry > 63))
			break;
		x += (int) currx;
		y += (int) curry;
		ikbd_inbuf -= 3;
		pos += 3;
	}

	buffer[pos+1] = (unsigned char) x;
	buffer[pos+2] = (unsigned char) y;

}

uae_u8 IKBD::getData() {
	inTransmit = false;
	int pos = (ikbd_bufpos - ikbd_inbuf) & MAXBUF;
	if( (buffer[pos] & 0xfc) == SC_MOUSEMOVE )
		compressMouseMove( pos );
	if (ikbd_inbuf-- > 0) {
		if (ikbd_inbuf ==0) {
#undef IKBD_LOCKS
#ifdef IKBD_LOCKS
			while (SDL_mutexP(rwLock)==-1)  // lock the read
				SDL_Delay(20);
#else
			inGet = true;
#endif
		
			/* Clear GPIP/I4 */
			status = 0;
			uae_u8 x = ReadAtariInt8(0xfffa01);
			x |= 0x10;
			WriteAtariInt8(0xfffa01, x);
		
#ifdef IKBD_LOCKS
			while (SDL_mutexV(rwLock)==-1)  // unlock
				SDL_Delay(20);
#endif
		}
		D(bug("IKBD read code %2x (%d left)", buffer[pos], ikbd_inbuf));
		doTransmit();
		inGet = false;
		return buffer[pos];
	}
	else {
		ikbd_inbuf = 0;
		inGet = false;
		return 0xa2;
	}
}

void IKBD::send(int value)
{
#ifdef IKBD_LOCKS
	while (SDL_mutexP(rwLock)==-1)  // lock the read
		SDL_Delay(20);
#else
	while (inGet) ;
#endif

	value &= 0xff;
	if (ikbd_inbuf <= MAXBUF) {
		buffer[ikbd_bufpos] = value;
		ikbd_bufpos++;
		ikbd_bufpos &= MAXBUF;
		ikbd_inbuf++;
		D(bug("IKBD sends %2x (->buffer pos %d)", value, ikbd_bufpos-1));
	}
	doTransmit();

#ifdef IKBD_LOCKS
	while (SDL_mutexV(rwLock)==-1)  // unlock
		SDL_Delay(20);
#endif
}

void IKBD::doTransmit(void)
{
	if (inTransmit) return;
	if ((HWget_b(0xfffa09) & 0x40) == 0) return;
	if (ikbd_inbuf == 0) return;
	/* set Interrupt Request */
	status |= 0x81;
	inTransmit = true;
	/* signal ACIA interrupt */
	mfp.setGPIPbit(0x10, 0);
}

static inline uint8 int2bcd(int a)
{
	return (a % 10) + ((a / 10) << 4);
}

void IKBD::setData(uae_u8 value)
{
	/* send data */
	if (value == 0x1c) {
		printf("IKBD_ReadClock\n");
		// Get current time
		time_t tim = time(NULL);
		struct tm *curtim = localtime(&tim);	// current time

		// Return packet
		send(0xfc);
		// Return time-of-day clock as yy-mm-dd-hh-mm-ss as BCD
		send(int2bcd(curtim->tm_year % 100));
		send(int2bcd(curtim->tm_mon+1));
		send(int2bcd(curtim->tm_mday));
		send(int2bcd(curtim->tm_hour));
		send(int2bcd(curtim->tm_min));
		send(int2bcd(curtim->tm_sec));

		return;
	}

	D(bug("IKBD data = %02x", value));
}
