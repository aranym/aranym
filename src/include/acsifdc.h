/* Joy 2001 */
/* Patrice Mandin */

#include "icio.h"

/* Defines */

#define HW_DISKDMA	0xff8600
#define HW_DISKDMA_LEN	0x10

enum {
	DISKDMA_BIT0=0,	/* for scsi: bits 2-0 are ncr5380 register */
	DISKDMA_A0,		/* for floppy: bits A0,A1 are WD1772 register */
	DISKDMA_A1,
	DISKDMA_CS,		/* Select, 0=floppy, 1=acsi/scsi */
	DISKDMA_BIT4,
	DISKDMA_BIT5,
	DISKDMA_DMA,	/* DMA enabled, 0=on, 1=off */
	DISKDMA_DRQ,	/* Run command, 0=off, 1=on */
	DISKDMA_RW		/* DMA direction, 0=read, 1=write */
};

/* The DMA disk class */

class ACSIFDC : public ICio {
private:
	uae_u16 DMAfifo;	/* write to $8606.w */
	uae_u16 DMAstatus;	/* read from $8606.w */
	uae_u16 DMAxor;
	uae_u8 DMAdiskctl;
	uae_u8 FDC_T, HDC_T;     /* Track register */
	uae_u8 FDC_S, HDC_S;     /* Sector register */
	uae_u8 FDC_D, HDC_D;     /* Data register */
	uaecptr DMAaddr;

public:
	ACSIFDC();
	virtual uae_u8 handleRead(uaecptr);
	virtual void handleWrite(uaecptr, uae_u8);

private:
	uae_u8 LOAD_B_ff8604(void);
	uae_u8 LOAD_B_ff8605(void);
	uae_u8 LOAD_B_ff8606(void);
	uae_u8 LOAD_B_ff8607(void);
	void STORE_B_ff8604(uae_u8);
	void STORE_B_ff8605(uae_u8);
	void STORE_B_ff8606(uae_u8);
	void STORE_B_ff8607(uae_u8);
};
