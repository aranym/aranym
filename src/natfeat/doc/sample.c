/*
 * sample.c - implementation of some Native functions 
 *
 * Copyright (c) 2002 Laurent Vogel
 *
 * TODO: license (LGPL?)
 */

#include "nf.h"

/*
 * this is the list of functions defined in this module.
 * they are made static in order to guarantee that they will not
 * interfere with names of other functions???
 *
 * TODO: suggest a naming rule for implementations?
 * example: nfi_ followed by the name of the feature?
 */

static void shutdown(nf_addr_t addr);

static void open_channel(nf_addr_t addr);
static void print_channel(nf_addr_t addr);

/*
 * the actual implementation of functions defined in this module
 */

/* quit the emulator */
static void shutdown(nf_addr_t addr)
{
  nf_quit();	
}



static void open_channel(nf_addr_t addr)
{
  /* TODO: bus error */
  char * channel = nf_get_str(addr);
  int32_t token = nf_to_token(channel);
  nf_set_d0l(token);
}

static void print_channel(nf_addr_t addr)
{
  /* TODO: bus error */
  int32_t token = nf_get_l(addr);
  char * channel = nf_from_token(token);
  char * str;
  if(channel == NULL) channel = "";
  str = nf_get_str(addr+4);
  printf("%s: %s\n", channel, str);
  nf_free(str);
}

static void native_print(nf_addr_t addr)
{
  /* TODO: bus error */
  char * str = nf_get_str(addr);
  printf("%s", str);
  nf_free(str);
}


/* TODO, basic set emulator name, version */

static void emulator_name(nf_addr_t addr)
{
  char *name = nf_emulator_name();
  ...
}

/*
 * module initialisation. This function is called by the 
 * emulator in an emulator-dependent way after the native
 * feature services have been setup, to register the native
 * features.
 *
 * TODO: suggest a naming convention for such functions?
 */

void sample_init(void)
{
  nf_register("shutdown", shutdown);
}


