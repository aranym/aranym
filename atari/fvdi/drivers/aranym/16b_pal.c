/*
 *      16 bit colour index handling
 */

#define GETNAME	c_get_colours_16
#define GET1NAME	c_get_colour_16
#define SETNAME	c_set_colours_16
#define PIXEL		unsigned short

#define red_bits   5	/* 5 for all normal 16 bit hardware */
#define green_bits 6	/* 6 for Falcon TC and NOVA 16 bit, 5 for NOVA 15 bit */
			/* (I think 15 bit Falcon TC disregards the green LSB) */
#define blue_bits  5	/* 5 for all normal 16 bit hardware */


#include "palette.c"
