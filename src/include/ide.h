/* Joy 2001 */

class IDE {
private:
	uae_u32 data;
	uae_u8 error;
	uae_u8 status;
	uae_u8 altstatus;
	uae_u8 scount;
	uae_u8 snumber;
	uae_u8 clow;
	uae_u8 chigh;
	uae_u8 dhead;
	uae_u8 dataout;
	uae_u8 cmd;

public:
	IDE();
	uae_u8 handleRead(uaecptr);
	void handleWrite(uaecptr, uae_u8);

private:
};
