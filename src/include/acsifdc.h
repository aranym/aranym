/* Joy 2001 */

#include "icio.h"

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
