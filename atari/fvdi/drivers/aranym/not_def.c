/*
 *	Dummy routines for unimplemented stuff
 */
/* Move dummies for functions that are implemented inside the '#if 0'. */


/*
 * Assembly functions
 */
#if 0
void write_pixel (void) { }
void read_pixel  (void) { }
void line_draw   (void) { }
void expand_area (void) { }
void blit_area   (void) { }
void fill_area   (void) { }
void mouse_draw  (void) { }
#endif
void get_colour  (void) { }
void set_colours (void) { }
void fill_polygon(void) { }
void text_area   (void) { }


/*
 * C functions
 */
void c_write_pixel (void) { }
void c_read_pixel  (void) { }
void c_line_draw   (void) { }
void c_expand_area (void) { }
void c_blit_area   (void) { }
void c_fill_area   (void) { }
#if 0
void c_get_colour  (void) { }
void c_set_colours (void) { }
#endif
void c_fill_polygon(void) { }
void c_text_area   (void) { }
void c_mouse_draw  (void) { }

