/* Joy 2001 */

class RTC {
private:
	uae_u8 addr;

public:
	RTC();
	uae_u8 handleRead(uaecptr);
	void handleWrite(uaecptr, uae_u8);

private:
	void setAddr(uae_u8 value);
	uae_u8 getData();
	void setData(uae_u8);
	void setChecksum();
};
