/* MJ 2001 */

#include "sysdeps.h"
#include "hardware.h"
#include "cpu_emulation.h"

void HWInit (void) {
	put_long(0x00000420,MEM_VALID_1);
	put_long(0x0000043a,MEM_VALID_2);
	put_long(0x0000051a,MEM_VALID_3);
	put_byte(0xffff8000,MEM_CTL);
	put_word(0xffff8006,SYS_CTL);
}
