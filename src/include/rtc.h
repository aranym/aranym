/* Joy 2001 */

class RTC {
private:
	uae_u8 addr;
	uae_u8 data;

public:
	RTC();
	void setAddr(uae_u8 value);
	uae_u8 getData();
	void setData(uae_u8);
};
