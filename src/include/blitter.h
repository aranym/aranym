/* Joy 2001 */

#include "icio.h"

class BLITTER : public ICio {
  public:
	uae_u16 halftone_ram[16];
	uae_u16 end_mask_1,end_mask_2,end_mask_3;
	uae_u8 NFSR,FXSR; 
	uae_u16 x_count,y_count;
	uae_u8 hop,op,line_num,skewreg;
	short int halftone_curroffset,halftone_direction;
	short int source_x_inc, source_y_inc, dest_x_inc, dest_y_inc;
	int source_addr;
	int dest_addr;
	bool blit;

public:
	BLITTER(void);
	virtual uae_u8 handleRead(uaecptr);
	virtual void handleWrite(uaecptr, uae_u8);

	uae_u16 LM_UW(uaecptr);
	void SM_UW(uaecptr, uae_u16);

private:
	void Do_Blit(void);

	char LOAD_B_ff8a28();
	char LOAD_B_ff8a29();
	char LOAD_B_ff8a2a();
	char LOAD_B_ff8a2b();
	char LOAD_B_ff8a2c();
	char LOAD_B_ff8a2d();
	char LOAD_B_ff8a32();
	char LOAD_B_ff8a33();
	char LOAD_B_ff8a34();
	char LOAD_B_ff8a35();
	char LOAD_B_ff8a36();
	char LOAD_B_ff8a37();
	char LOAD_B_ff8a38();
	char LOAD_B_ff8a39();
	char LOAD_B_ff8a3a();
	char LOAD_B_ff8a3b();
	char LOAD_B_ff8a3c();
	char LOAD_B_ff8a3d();

	void STORE_B_ff8a28(char);
	void STORE_B_ff8a29(char);
	void STORE_B_ff8a2a(char);
	void STORE_B_ff8a2b(char);
	void STORE_B_ff8a2c(char);
	void STORE_B_ff8a2d(char);
	void STORE_B_ff8a32(char);
	void STORE_B_ff8a33(char);
	void STORE_B_ff8a34(char);
	void STORE_B_ff8a35(char);
	void STORE_B_ff8a36(char);
	void STORE_B_ff8a37(char);
	void STORE_B_ff8a38(char);
	void STORE_B_ff8a39(char);
	void STORE_B_ff8a3a(char);
	void STORE_B_ff8a3b(char);
	void STORE_B_ff8a3c(char);
	void STORE_B_ff8a3d(char);
};

