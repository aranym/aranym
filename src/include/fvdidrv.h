/*
 * $Header$
 *
 * STanda 2001
 */

#ifndef _FVDIDRV_H
#define _FVDIDRV_H


class FVDIDriver {
private:

  struct MFDB {
	uint16 *address;
	uint16 width;
	uint16 height;
	uint16 wdwidth;
	uint16 standard;
	uint16 bitplanes;
	uint16 reserved[3];
  };

public:
  FVDIDriver() {
	// This is the default drv (shouldn't be used)
  }
  ~FVDIDriver() {
  }

  void dispatch( uint32 fncode, M68kRegisters *r );

  MFDB* FVDIDriver::fetchMFDB( MFDB* mfdb, uint32 pmfdb );
  //  MFDB*  fetchMFDB( MFDB* mfdb, uint32 pmfdb );
  uint32 write_pixel( void *vwk, MFDB *dst, uint32 x, uint32 y, uint32 colour );
  uint32 read_pixel( void *vwk, MFDB *src, uint32 x, uint32 y );
};

#endif


/*
 * $Log$
 *
 */
