extern unsigned short const atari_to_utf16[256];
extern unsigned short const atarifont_to_utf16[256];
extern unsigned short const utf16_to_atari[0x10000];
void charset_conv_error(unsigned short ch);
