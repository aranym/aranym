/* Joy 2001 */

#include "icio.h"

class RTC : public ICio {
private:
	uint8 addr;
	char nvram_filename[512];

public:
	RTC();
	virtual ~RTC();
	void init(void);
	bool load(void);
	bool save(void);
	virtual uint8 handleRead(uaecptr);
	virtual void handleWrite(uaecptr, uint8);

private:
	void setAddr(uint8 value);
	uint8 getData();
	void setData(uint8);
	void setChecksum();
};
