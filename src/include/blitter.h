/* Joy 2001 */

class BLITTER {
private:
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
	uae_u8 handleRead(uaecptr);
	void handleWrite(uaecptr, uae_u8);

private:
	uae_u16 LM_UW(uaecptr);
	void SM_UW(uaecptr, uae_u16);

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

	void (*do_hop_op_N[4][16])(void);

	void _HOP_0_OP_00_N(void);
	void _HOP_0_OP_01_N(void);
	void _HOP_0_OP_02_N(void);
	void _HOP_0_OP_03_N(void);
	void _HOP_0_OP_04_N(void);
	void _HOP_0_OP_05_N(void);
	void _HOP_0_OP_06_N(void);
	void _HOP_0_OP_07_N(void);
	void _HOP_0_OP_08_N(void);
	void _HOP_0_OP_09_N(void);
	void _HOP_0_OP_10_N(void);
	void _HOP_0_OP_11_N(void);
	void _HOP_0_OP_12_N(void);
	void _HOP_0_OP_13_N(void);
	void _HOP_0_OP_14_N(void);
	void _HOP_0_OP_15_N(void);

	void _HOP_1_OP_00_N(void);
	void _HOP_1_OP_01_N(void);
	void _HOP_1_OP_02_N(void);
	void _HOP_1_OP_03_N(void);
	void _HOP_1_OP_04_N(void);
	void _HOP_1_OP_05_N(void);
	void _HOP_1_OP_06_N(void);
	void _HOP_1_OP_07_N(void);
	void _HOP_1_OP_08_N(void);
	void _HOP_1_OP_09_N(void);
	void _HOP_1_OP_10_N(void);
	void _HOP_1_OP_11_N(void);
	void _HOP_1_OP_12_N(void);
	void _HOP_1_OP_13_N(void);
	void _HOP_1_OP_14_N(void);
	void _HOP_1_OP_15_N(void);

	void _HOP_2_OP_00_N(void);
	void _HOP_2_OP_01_N(void);
	void _HOP_2_OP_02_N(void);
	void _HOP_2_OP_03_N(void);
	void _HOP_2_OP_04_N(void);
	void _HOP_2_OP_05_N(void);
	void _HOP_2_OP_06_N(void);
	void _HOP_2_OP_07_N(void);
	void _HOP_2_OP_08_N(void);
	void _HOP_2_OP_09_N(void);
	void _HOP_2_OP_10_N(void);
	void _HOP_2_OP_11_N(void);
	void _HOP_2_OP_12_N(void);
	void _HOP_2_OP_13_N(void);
	void _HOP_2_OP_14_N(void);
	void _HOP_2_OP_15_N(void);

	void _HOP_3_OP_00_N(void);
	void _HOP_3_OP_01_N(void);
	void _HOP_3_OP_02_N(void);
	void _HOP_3_OP_03_N(void);
	void _HOP_3_OP_04_N(void);
	void _HOP_3_OP_05_N(void);
	void _HOP_3_OP_06_N(void);
	void _HOP_3_OP_07_N(void);
	void _HOP_3_OP_08_N(void);
	void _HOP_3_OP_09_N(void);
	void _HOP_3_OP_10_N(void);
	void _HOP_3_OP_11_N(void);
	void _HOP_3_OP_12_N(void);
	void _HOP_3_OP_13_N(void);
	void _HOP_3_OP_14_N(void);
	void _HOP_3_OP_15_N(void);

	void _HOP_0_OP_00_P(void);
	void _HOP_0_OP_01_P(void);
	void _HOP_0_OP_02_P(void);
	void _HOP_0_OP_03_P(void);
	void _HOP_0_OP_04_P(void);
	void _HOP_0_OP_05_P(void);
	void _HOP_0_OP_06_P(void);
	void _HOP_0_OP_07_P(void);
	void _HOP_0_OP_08_P(void);
	void _HOP_0_OP_09_P(void);
	void _HOP_0_OP_10_P(void);
	void _HOP_0_OP_11_P(void);
	void _HOP_0_OP_12_P(void);
	void _HOP_0_OP_13_P(void);
	void _HOP_0_OP_14_P(void);
	void _HOP_0_OP_15_P(void);

	void _HOP_1_OP_00_P(void);
	void _HOP_1_OP_01_P(void);
	void _HOP_1_OP_02_P(void);
	void _HOP_1_OP_03_P(void);
	void _HOP_1_OP_04_P(void);
	void _HOP_1_OP_05_P(void);
	void _HOP_1_OP_06_P(void);
	void _HOP_1_OP_07_P(void);
	void _HOP_1_OP_08_P(void);
	void _HOP_1_OP_09_P(void);
	void _HOP_1_OP_10_P(void);
	void _HOP_1_OP_11_P(void);
	void _HOP_1_OP_12_P(void);
	void _HOP_1_OP_13_P(void);
	void _HOP_1_OP_14_P(void);
	void _HOP_1_OP_15_P(void);

	void _HOP_2_OP_00_P(void);
	void _HOP_2_OP_01_P(void);
	void _HOP_2_OP_02_P(void);
	void _HOP_2_OP_03_P(void);
	void _HOP_2_OP_04_P(void);
	void _HOP_2_OP_05_P(void);
	void _HOP_2_OP_06_P(void);
	void _HOP_2_OP_07_P(void);
	void _HOP_2_OP_08_P(void);
	void _HOP_2_OP_09_P(void);
	void _HOP_2_OP_10_P(void);
	void _HOP_2_OP_11_P(void);
	void _HOP_2_OP_12_P(void);
	void _HOP_2_OP_13_P(void);
	void _HOP_2_OP_14_P(void);
	void _HOP_2_OP_15_P(void);

	void _HOP_3_OP_00_P(void);
	void _HOP_3_OP_01_P(void);
	void _HOP_3_OP_02_P(void);
	void _HOP_3_OP_03_P(void);
	void _HOP_3_OP_04_P(void);
	void _HOP_3_OP_05_P(void);
	void _HOP_3_OP_06_P(void);
	void _HOP_3_OP_07_P(void);
	void _HOP_3_OP_08_P(void);
	void _HOP_3_OP_09_P(void);
	void _HOP_3_OP_10_P(void);
	void _HOP_3_OP_11_P(void);
	void _HOP_3_OP_12_P(void);
	void _HOP_3_OP_13_P(void);
	void _HOP_3_OP_14_P(void);
	void _HOP_3_OP_15_P(void);

	void hop2op3p(void);
	void hop2op3n(void);
};
