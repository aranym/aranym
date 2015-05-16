extern unsigned short const atari_to_utf16[256];
extern unsigned short const atarifont_to_utf16[256];
extern const unsigned char (*const utf16_to_atari[256])[256];
void charset_conv_error(unsigned short ch);
