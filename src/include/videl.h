/* Joy 2001 */

class VIDEL {
private:
	uae_u8 shifter;
	uae_u16 videl;
	uae_u8 videoctrl, videoctrl2;
	uae_u8 syncmode;
	uae_u8 videomode;

public:
	VIDEL(void);
	uae_u8 handleRead(uaecptr);
	void handleWrite(uaecptr, uae_u8);

private:
};
