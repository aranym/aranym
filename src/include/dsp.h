/* Joy 2001 */

#include "icio.h"

class DSP : public BASE_IO {
public:
	DSP();
	virtual uae_u8 handleRead(uaecptr addr);
};
