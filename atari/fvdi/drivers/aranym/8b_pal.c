/*
 *      32 bit colour index handling
 */

#define GETNAME	    c_get_colours_8
#define GET1NAME	c_get_colour_8
#define SETNAME	    c_set_colours_8
#define PIXEL		unsigned long

#define red_bits   8	/* 5 for all normal 16 bit hardware */
#define green_bits 8	/* 6 for Falcon TC and NOVA 16 bit, 5 for NOVA 15 bit */
			/* (I think 15 bit Falcon TC disregards the green LSB) */
#define blue_bits  8	/* 5 for all normal 16 bit hardware */


#include "palette.c"

