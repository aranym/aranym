/*
 *  vm_alloc.cpp - Wrapper to various virtual memory allocation schemes
 *                 (supports mmap, vm_allocate or fallbacks to malloc)
 *
 * Copyright (c) 2000-2005 ARAnyM developer team (see AUTHORS)
 * 
 * Originally derived from Basilisk II (C) 1997-2000 Christian Bauer
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

#include "sysdeps.h"
#include "vm_alloc.h"

#if defined(OS_freebsd) && defined(CPU_x86_64)
#	include <sys/resource.h>
#endif

# include <cstdlib>
# include <cstring>
#ifdef HAVE_WIN32_VM
    #undef WIN32_LEAN_AND_MEAN
	#define WIN32_LEAN_AND_MEAN 1 /* avoid including junk */
	#include <windows.h>
    #undef WIN32_LEAN_AND_MEAN /* to avoid redefinition in SDL headers */
#endif

#ifdef HAVE_FCNTL_H
	/* for O_RDWR - RedHat FC2 need this */
	#include <fcntl.h>
#endif

#ifdef HAVE_MACH_VM
	#ifndef HAVE_MACH_TASK_SELF
		#ifdef HAVE_TASK_SELF
			#define mach_task_self task_self
		#else
			#error "No task_self(), you lose."
		#endif
	#endif
#endif

/* We want MAP_32BIT, if available, for SheepShaver and BasiliskII
   because the emulated target is 32-bit and this helps to allocate
   memory so that branches could be resolved more easily (32-bit
   displacement to code in .text), on AMD64 for example.  */
#if !defined(MAP_32BIT) && defined(MAP_LOW32)
	#define MAP_32BIT MAP_LOW32
#endif
#ifndef MAP_32BIT
	#define MAP_32BIT 0
#endif
#ifndef MAP_ANON
	#define MAP_ANON 0
#endif
#ifndef MAP_ANONYMOUS
	#define MAP_ANONYMOUS 0
#endif

#define MAP_EXTRA_FLAGS 0

#ifdef HAVE_MACH_VM
#elif defined(HAVE_MMAP_VM)
	#if defined(__linux__) && defined(CPU_i386)
		/*  Force a reasonnable address below 0x80000000 on x86 so that we
			don't get addresses above when the program is run on AMD64.
			NOTE: this is empirically determined on Linux/x86.  */
		#define MAP_BASE	0x10000000
	#else
		#define MAP_BASE	0x00000000
	#endif
	static char * next_address = (char *)MAP_BASE;
	static char * next_address_32bit = (char *)MAP_BASE;
#endif

#ifdef HAVE_MMAP_VM
	#ifdef HAVE_MMAP_ANON
	#define map_flags	(MAP_ANON | MAP_EXTRA_FLAGS)
	#define zero_fd		-1
	#elif defined(HAVE_MMAP_ANONYMOUS)
	#define map_flags	(MAP_ANONYMOUS | MAP_EXTRA_FLAGS)
	#define zero_fd		-1
	#else
	#define map_flags	(MAP_EXTRA_FLAGS)
	static int zero_fd	= -1;
	#endif
#endif

/* Translate generic VM map flags to host values.  */

#ifdef HAVE_MACH_VM
#elif defined(HAVE_MMAP_VM)
static int translate_map_flags(int vm_flags)
{
	int flags = 0;
	if (vm_flags & VM_MAP_SHARED)
		flags |= MAP_SHARED;
	if (vm_flags & VM_MAP_PRIVATE)
		flags |= MAP_PRIVATE;
	if (vm_flags & VM_MAP_FIXED)
		flags |= MAP_FIXED;
	if (vm_flags & VM_MAP_32BIT)
		flags |= MAP_32BIT;
	return flags;
}
#endif

/* Align ADDR and SIZE to 64K boundaries.  */

#ifdef HAVE_WIN32_VM
static inline LPVOID align_addr_segment(LPVOID addr)
{
	return (LPVOID)(((DWORD_PTR)addr) & -65536);
}

static inline DWORD align_size_segment(LPVOID addr, DWORD size)
{
	return size + ((DWORD_PTR)addr - (DWORD_PTR)align_addr_segment(addr));
}
#endif

/* Translate generic VM prot flags to host values.  */

#ifdef HAVE_WIN32_VM
static int translate_prot_flags(int prot_flags)
{
	int prot = PAGE_READWRITE;
	if (prot_flags == (VM_PAGE_EXECUTE | VM_PAGE_READ | VM_PAGE_WRITE))
		prot = PAGE_EXECUTE_READWRITE;
	else if (prot_flags == (VM_PAGE_EXECUTE | VM_PAGE_READ))
		prot = PAGE_EXECUTE_READ;
	else if (prot_flags == (VM_PAGE_READ | VM_PAGE_WRITE))
		prot = PAGE_READWRITE;
	else if (prot_flags == VM_PAGE_READ)
		prot = PAGE_READONLY;
	else if (prot_flags == VM_PAGE_NOACCESS)
		prot = PAGE_NOACCESS;
	return prot;
}
#endif

/* Initialize the VM system. Returns 0 if successful, -1 for errors.  */

int vm_init(void)
{
#ifdef HAVE_MMAP_VM
#ifndef zero_fd
	zero_fd = open("/dev/zero", O_RDWR);
	if (zero_fd < 0)
		return -1;
#endif
#endif
	return 0;
}

/* Deallocate all internal data used to wrap virtual memory allocators.  */

void vm_exit(void)
{
#ifdef HAVE_MMAP_VM
#ifndef zero_fd
	if (zero_fd != -1) {
		close(zero_fd);
		zero_fd = -1;
	}
#endif
#endif
}

/* Allocate zero-filled memory of SIZE bytes. The mapping is private
   and default protection bits are read / write. The return value
   is the actual mapping address chosen or VM_MAP_FAILED for errors.  */

void * vm_acquire(size_t size, int options)
{
	void * addr;

	// VM_MAP_FIXED are to be used with vm_acquire_fixed() only
	if (options & VM_MAP_FIXED)
		return VM_MAP_FAILED;

#ifdef HAVE_MACH_VM
	// vm_allocate() returns a zero-filled memory region
	if (vm_allocate(mach_task_self(), (vm_address_t *)&addr, size, TRUE) != KERN_SUCCESS)
		return VM_MAP_FAILED;

	// Sanity checks for 64-bit platforms
	if (sizeof(void *) > 4 && (options & VM_MAP_32BIT) && !(((char *)addr + size) <= (char *)0xffffffff))
	{
		vm_release(addr, size);
		return VM_MAP_FAILED;
	}
#elif defined(HAVE_MMAP_VM)
	int fd = zero_fd;
	int the_map_flags = translate_map_flags(options) | map_flags;
	char **base = (options & VM_MAP_32BIT) ? &next_address_32bit : &next_address;

//
// FREEBSD has no MAP_32BIT on x64
// Hack to limit allocation to lower 32 Bit
#if defined(OS_freebsd) && defined(CPU_x86_64)
	static int mode32 = 0;
	static rlimit oldlim;
        if (!mode32 && (options & VM_MAP_32BIT)) {
	  getrlimit(RLIMIT_DATA, &oldlim);
          struct rlimit rlim;
          rlim.rlim_cur = rlim.rlim_max = 0x10000000;
          setrlimit(RLIMIT_DATA, &rlim);
          mode32 = 1;
        }
#	define RESTORE_MODE	if (mode32) { setrlimit(RLIMIT_DATA, &oldlim); mode32 = 0;}
#else
#	define RESTORE_MODE
#endif
	
	addr = mmap((void *)(*base), size, VM_PAGE_DEFAULT, the_map_flags, fd, 0);
	RESTORE_MODE;
	if (addr == (void *)MAP_FAILED)
		return VM_MAP_FAILED;

	// Sanity checks for 64-bit platforms
	if (sizeof(void *) > 4 && (options & VM_MAP_32BIT) && !(((char *)addr + size) <= (char *)0xffffffff))
	{
		vm_release(addr, size);
		return VM_MAP_FAILED;
	}

	*base = (char *)addr + size;
	
#else
#ifdef HAVE_WIN32_VM
	if ((addr = VirtualAlloc(NULL, size, MEM_RESERVE | MEM_COMMIT, PAGE_EXECUTE_READWRITE)) == NULL)
		return VM_MAP_FAILED;

	// Sanity checks for 64-bit platforms
	if (sizeof(void *) > 4 && (options & VM_MAP_32BIT) && !(((char *)addr + size) <= (char *)0xffffffff))
	{
		vm_release(addr, size);
		return VM_MAP_FAILED;
	}
#else
	if ((addr = calloc(size, 1)) == 0)
		return VM_MAP_FAILED;
	
	// Sanity checks for 64-bit platforms
	if (sizeof(void *) > 4 && (options & VM_MAP_32BIT) && !(((char *)addr + size) <= (char *)0xffffffff))
	{
		free(addr);
		return VM_MAP_FAILED;
	}

	// Omit changes for protections because they are not supported in this mode
	return addr;
#endif
#endif

	// Explicitely protect the newly mapped region here because on some systems,
	// say MacOS X, mmap() doesn't honour the requested protection flags.
	if (vm_protect(addr, size, VM_PAGE_DEFAULT) != 0)
	{
		vm_release(addr, size);
		return VM_MAP_FAILED;
	}
	
	return addr;
}

/* Allocate zero-filled memory at exactly ADDR (which must be page-aligned).
   Retuns 0 if successful, -1 on errors.  */

bool vm_acquire_fixed(void * addr, size_t size, int options)
{
	// Fixed mappings are required to be private
	if (options & VM_MAP_SHARED)
		return false;

#ifdef HAVE_MACH_VM
	// vm_allocate() returns a zero-filled memory region
	if (vm_allocate(mach_task_self(), (vm_address_t *)&addr, size, 0) != KERN_SUCCESS)
		return false;
#elif defined(HAVE_MMAP_VM)
	const int extra_map_flags = translate_map_flags(options);

	if (mmap((void *)addr, size, VM_PAGE_DEFAULT, extra_map_flags | map_flags | MAP_FIXED, zero_fd, 0) == MAP_FAILED)
		return false;
#else
#ifdef HAVE_WIN32_VM
	// Windows cannot allocate Low Memory
	if (addr == NULL)
		return false;

	// Allocate a possibly offset region to align on 64K boundaries
	LPVOID req_addr = align_addr_segment(addr);
	DWORD  req_size = align_size_segment(addr, size);
	LPVOID ret_addr = VirtualAlloc(req_addr, req_size, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
	if (ret_addr != req_addr)
		return false;
#else
	// Unsupported
	return false;
#endif
#endif

	// Explicitely protect the newly mapped region here because on some systems,
	// say MacOS X, mmap() doesn't honour the requested protection flags.
	if (vm_protect(addr, size, VM_PAGE_DEFAULT) != 0)
		return false;

	return true;
}

/* Deallocate any mapping for the region starting at ADDR and extending
   LEN bytes. Returns 0 if successful, -1 on errors.  */

int vm_release(void * addr, size_t size)
{
	// Safety check: don't try to release memory that was not allocated
	if (addr == VM_MAP_FAILED)
		return 0;

#ifdef HAVE_MACH_VM
	if (vm_deallocate(mach_task_self(), (vm_address_t)addr, size) != KERN_SUCCESS)
		return -1;
#elif defined(HAVE_MMAP_VM)
	if (munmap((void *)addr, size) != 0)
		return -1;

#else
#ifdef HAVE_WIN32_VM
	if (VirtualFree(align_addr_segment(addr), 0, MEM_RELEASE) == 0)
		return -1;
	(void) size;
#else
	free(addr);
#endif
#endif
	
	return 0;
}

/* Change the memory protection of the region starting at ADDR and
   extending LEN bytes to PROT. Returns 0 if successful, -1 for errors.  */

int vm_protect(void * addr, size_t size, int prot)
{
#ifdef HAVE_MACH_VM
	int ret_code = vm_protect(mach_task_self(), (vm_address_t)addr, size, 0, prot);
	return ret_code == KERN_SUCCESS ? 0 : -1;
#elif defined(HAVE_MMAP_VM)
	int ret_code = mprotect((void *)addr, size, prot);
	return ret_code == 0 ? 0 : -1;
#elif defined(HAVE_WIN32_VM)
	DWORD old_prot;
	int ret_code = VirtualProtect(addr, size, translate_prot_flags(prot), &old_prot);
	return ret_code != 0 ? 0 : -1;
#else
	// Unsupported
	return -1;
#endif
}

/* Returns the size of a page.  */

int vm_get_page_size(void)
{
#ifdef _WIN32
	SYSTEM_INFO si;
	GetSystemInfo(&si);
	if (si.dwAllocationGranularity > si.dwPageSize)
		return si.dwAllocationGranularity;
	return si.dwPageSize;
#else
    return getpagesize();
#endif
}

#ifdef CONFIGURE_TEST_VM_MAP
/* Tests covered here:
   - TEST_VM_PROT_* program slices actually succeeds when a crash occurs
   - TEST_VM_MAP_ANON* program slices succeeds when it could be compiled
*/
#include <signal.h>
void handler(int sig)
{
	exit(2);
}
int main(void)
{
	vm_init();
	signal(SIGSEGV, handler);
	
#define page_align(address) ((char *)((uintptr)(address) & -page_size))
	const unsigned long page_size = vm_get_page_size();
	
	const int area_size = 6 * page_size;
	volatile char * area = (volatile char *) vm_acquire(area_size);
	volatile char * fault_address = area + (page_size * 7) / 2;

#if defined(TEST_VM_MMAP_ANON) || defined(TEST_VM_MMAP_ANONYMOUS)
	if (area == VM_MAP_FAILED)
		return 1;

	if (vm_release((char *)area, area_size) < 0)
		return 1;
	
	return 0;
#endif

#if defined(TEST_VM_PROT_NONE_READ) || defined(TEST_VM_PROT_NONE_WRITE)
	if (area == VM_MAP_FAILED)
		return 0;
	
	if (vm_protect(page_align(fault_address), page_size, VM_PAGE_NOACCESS) < 0)
		return 0;
#endif

#if defined(TEST_VM_PROT_RDWR_WRITE)
	if (area == VM_MAP_FAILED)
		return 1;
	
	if (vm_protect(page_align(fault_address), page_size, VM_PAGE_READ) < 0)
		return 1;
	
	if (vm_protect(page_align(fault_address), page_size, VM_PAGE_READ | VM_PAGE_WRITE) < 0)
		return 1;
#endif

#if defined(TEST_VM_PROT_READ_WRITE)
	if (vm_protect(page_align(fault_address), page_size, VM_PAGE_READ) < 0)
		return 0;
#endif

#if defined(TEST_VM_PROT_NONE_READ)
	// this should cause a core dump
	char foo = *fault_address;
	// if we get here vm_protect(VM_PAGE_NOACCESS) did not work
	return 0;
#endif

#if defined(TEST_VM_PROT_NONE_WRITE) || defined(TEST_VM_PROT_READ_WRITE)
	// this should cause a core dump
	*fault_address = 'z';
	// if we get here vm_protect(VM_PAGE_READ) did not work
	return 0;
#endif

#if defined(TEST_VM_PROT_RDWR_WRITE)
	// this should not cause a core dump
	*fault_address = 'z';
	return 0;
#endif
}
#endif
