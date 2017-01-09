/*
 * sysconfig.h - (WIN)UAE defines for ARAnyM
 *
 * Copyright (c) 2017 ARAnyM dev team (see AUTHORS)
 * 
 * This file is part of the ARAnyM project which builds a new and powerful
 * TOS/FreeMiNT compatible virtual machine running on almost any hardware.
 *
 * ARAnyM is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * ARAnyM is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with ARAnyM; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef _SYSCONFIG_H
#define _SYSCONFIG_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#define WINUAE_ARANYM 1
#undef UAE /* just in case */

/* ARAnyM uses USE_JIT; WINUAE uses JIT to enable JIT support */
#ifdef USE_JIT
#define JIT 1
#endif

/* WINUAE makes this conditional */
#define NOFLAGS_SUPPORT 1

#if SIZEOF_VOID_P != 4
#define CPU_64_BIT 1
#endif

#endif
