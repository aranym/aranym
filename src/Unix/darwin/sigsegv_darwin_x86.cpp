/*
 * sigsegv_darwin_x86.cpp - x86 Darwin SIGSEGV handler
 *
 * Copyright (c) 2006 Milan Jurik of ARAnyM dev team (see AUTHORS)
 * 
 * Inspired by Bernie Meyer's UAE-JIT and Gwenole Beauchesne's Basilisk II-JIT
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
 *
 * Last modified: 2013-06-16 Jens Heitmann
 *
 */

#include "sysdeps.h"
#include "cpu_emulation.h"
#include "memory-uae.h"
#define DEBUG 0
#include "debug.h"

//
//
//  Darwin segmentation violation handler
//  based on the code of Basilisk II
//
#include <pthread.h>

// Address type
typedef char * sigsegv_address_t;

// SIGSEGV handler return state
enum sigsegv_return_t {
  SIGSEGV_RETURN_SUCCESS,
  SIGSEGV_RETURN_FAILURE,
//  SIGSEGV_RETURN_SKIP_INSTRUCTION,
};

// Define an address that is bound to be invalid for a program counter
const sigsegv_address_t SIGSEGV_INVALID_PC = (sigsegv_address_t)(-1);

extern "C" {
#include <mach/mach.h>
#include <mach/mach_error.h>
	
#ifdef CPU_i386
#	undef MACH_EXCEPTION_CODES
#	define MACH_EXCEPTION_CODES						0
#	define MACH_EXCEPTION_DATA_T					exception_data_t
#	define MACH_EXCEPTION_DATA_TYPE_T				exception_data_type_t
#	define MACH_EXC_SERVER							exc_server
#	define CATCH_MACH_EXCEPTION_RAISE				catch_exception_raise
#	define MACH_EXCEPTION_RAISE						exception_raise
#	define MACH_EXCEPTION_RAISE_STATE				exception_raise_state
#	define MACH_EXCEPTION_RAISE_STATE_IDENTITY		exception_raise_state_identity

#else

#	define MACH_EXCEPTION_DATA_T					mach_exception_data_t
#	define MACH_EXCEPTION_DATA_TYPE_T				mach_exception_data_type_t
#	define MACH_EXC_SERVER							mach_exc_server
#	define CATCH_MACH_EXCEPTION_RAISE				catch_mach_exception_raise
#	define MACH_EXCEPTION_RAISE						mach_exception_raise
#	define MACH_EXCEPTION_RAISE_STATE				mach_exception_raise_state
#	define MACH_EXCEPTION_RAISE_STATE_IDENTITY		mach_exception_raise_state_identity

#endif

	
// Extern declarations of mach functions
// dependend on the underlying architecture this are extern declarations
// for "mach" or "non mach" function names. 64 Bit requieres "mach_xxx"
//	functions
//
extern boolean_t MACH_EXC_SERVER(mach_msg_header_t *, mach_msg_header_t *);

extern kern_return_t CATCH_MACH_EXCEPTION_RAISE(mach_port_t, mach_port_t,
	mach_port_t, exception_type_t, MACH_EXCEPTION_DATA_T, mach_msg_type_number_t);

extern kern_return_t MACH_EXCEPTION_RAISE(mach_port_t, mach_port_t, mach_port_t,
	exception_type_t, MACH_EXCEPTION_DATA_T, mach_msg_type_number_t);

extern kern_return_t MACH_EXCEPTION_RAISE_STATE(mach_port_t, exception_type_t,
	MACH_EXCEPTION_DATA_T, mach_msg_type_number_t, thread_state_flavor_t *,
	thread_state_t, mach_msg_type_number_t, thread_state_t, mach_msg_type_number_t *);
	
extern kern_return_t MACH_EXCEPTION_RAISE_STATE_IDENTITY(mach_port_t, mach_port_t, mach_port_t,
	exception_type_t, MACH_EXCEPTION_DATA_T, mach_msg_type_number_t, thread_state_flavor_t *,
	thread_state_t, mach_msg_type_number_t, thread_state_t, mach_msg_type_number_t *);
	
extern kern_return_t catch_mach_exception_raise_state(mach_port_t,
	exception_type_t, MACH_EXCEPTION_DATA_T, mach_msg_type_number_t,
	int *, thread_state_t, mach_msg_type_number_t, thread_state_t, mach_msg_type_number_t *);
	   
extern kern_return_t catch_mach_exception_raise_state_identity(mach_port_t,
	mach_port_t, mach_port_t, exception_type_t, MACH_EXCEPTION_DATA_T, mach_msg_type_number_t,
    int *, thread_state_t, mach_msg_type_number_t, thread_state_t, mach_msg_type_number_t *);

}

// Could make this dynamic by looking for a result of MIG_ARRAY_TOO_LARGE
#define HANDLER_COUNT 64

// structure to tuck away existing exception handlers
typedef struct _ExceptionPorts {
	mach_msg_type_number_t maskCount;
	exception_mask_t masks[HANDLER_COUNT];
	exception_handler_t handlers[HANDLER_COUNT];
	exception_behavior_t behaviors[HANDLER_COUNT];
	thread_state_flavor_t flavors[HANDLER_COUNT];
} ExceptionPorts;

#ifdef CPU_i386
#	define STATE_REGISTER_TYPE	uint32
#	ifdef i386_SAVED_STATE
#		define SIGSEGV_THREAD_STATE_TYPE		struct i386_saved_state
#		define SIGSEGV_THREAD_STATE_FLAVOR		i386_SAVED_STATE
#		define SIGSEGV_THREAD_STATE_COUNT		i386_SAVED_STATE_COUNT
#		define SIGSEGV_REGISTER_FILE			((unsigned long *)&state->edi) /* EDI is the first GPR we consider */
#	else
#		ifdef x86_THREAD_STATE32
/* MacOS X 10.5 or newer introduces the new names and deprecates the old ones */
#			define SIGSEGV_THREAD_STATE_TYPE		x86_thread_state32_t
#			define SIGSEGV_THREAD_STATE_FLAVOR		x86_THREAD_STATE32
#			define SIGSEGV_THREAD_STATE_COUNT		x86_THREAD_STATE32_COUNT
#			define SIGSEGV_REGISTER_FILE			((unsigned long *)&state->__eax) /* EAX is the first GPR we consider */
#			define SIGSEGV_FAULT_INSTRUCTION		state->__eip

#		else
/* MacOS X 10.4 and below */
#			define SIGSEGV_THREAD_STATE_TYPE		struct i386_thread_state
#			define SIGSEGV_THREAD_STATE_FLAVOR		i386_THREAD_STATE
#			define SIGSEGV_THREAD_STATE_COUNT		i386_THREAD_STATE_COUNT
#			define SIGSEGV_REGISTER_FILE			((unsigned long *)&state->eax) /* EAX is the first GPR we consider */
#			define SIGSEGV_FAULT_INSTRUCTION		state->eip

#		endif
#	endif
#endif

#ifdef CPU_x86_64
#	define STATE_REGISTER_TYPE	uint64
#	ifdef x86_THREAD_STATE64
#		define SIGSEGV_THREAD_STATE_TYPE		x86_thread_state64_t
#		define SIGSEGV_THREAD_STATE_FLAVOR		x86_THREAD_STATE64
#		define SIGSEGV_THREAD_STATE_COUNT		x86_THREAD_STATE64_COUNT
#		define SIGSEGV_REGISTER_FILE			((unsigned long *)&state->__rax) /* EAX is the first GPR we consider */
#		define SIGSEGV_FAULT_INSTRUCTION		state->__rip
#	endif
#endif

#define SIGSEGV_ERROR_CODE				KERN_INVALID_ADDRESS
#define SIGSEGV_ERROR_CODE2				KERN_PROTECTION_FAILURE

#define SIGSEGV_FAULT_ADDRESS			code[1]

enum {
#ifdef CPU_i386
#ifdef i386_SAVED_STATE
	// same as FreeBSD (in Open Darwin 8.0.1)
	REG_RIP = 10,
	REG_RAX = 7,
	REG_RCX = 6,
	REG_RDX = 5,
	REG_RBX = 4,
	REG_RSP = 13,
	REG_RBP = 2,
	REG_RSI = 1,
	REG_RDI = 0
#else
	// new layout (MacOS X 10.4.4 for x86)
	REG_RIP = 10,
	REG_RAX = 0,
	REG_RCX = 2,
	REG_RDX = 3,
	REG_RBX = 1,
	REG_RSP = 7,
	REG_RBP = 6,
	REG_RSI = 5,
	REG_RDI = 4
#endif
#else
#ifdef CPU_x86_64
	REG_R8  = 8,
	REG_R9  = 9,
	REG_R10 = 10,
	REG_R11 = 11,
	REG_R12 = 12,
	REG_R13 = 13,
	REG_R14 = 14,
	REG_R15 = 15,
	REG_RDI = 4,
	REG_RSI = 5,
	REG_RBP = 6,
	REG_RBX = 1,
	REG_RDX = 3,
	REG_RAX = 0,
	REG_RCX = 2,
	REG_RSP = 7,
	REG_RIP = 16
#endif
#endif
};

// Type of a SIGSEGV handler. Returns boolean expressing successful operation
typedef sigsegv_return_t (*sigsegv_fault_handler_t)(sigsegv_address_t fault_address, sigsegv_address_t instruction_address,
										 SIGSEGV_THREAD_STATE_TYPE *state );

// Install a SIGSEGV handler. Returns boolean expressing success
extern bool sigsegv_install_handler(sigsegv_fault_handler_t handler);

// exception handler thread
static pthread_t exc_thread;

static mach_port_t _exceptionPort = MACH_PORT_NULL;

// place where old exception handler info is stored
static ExceptionPorts ports;

// User's SIGSEGV handler
static sigsegv_fault_handler_t sigsegv_fault_handler = 0;


#define MACH_CHECK_ERROR(name,ret) \
if (ret != KERN_SUCCESS) { \
	mach_error(#name, ret); \
	exit (1); \
}

#define MSG_SIZE 512
static char msgbuf[MSG_SIZE];
static char replybuf[MSG_SIZE];

#define CONTEXT_NAME	state
#define CONTEXT_TYPE	SIGSEGV_THREAD_STATE_TYPE
#define CONTEXT_ATYPE	CONTEXT_TYPE *
#define CONTEXT_REGS    SIGSEGV_REGISTER_FILE
#define CONTEXT_AEIP	CONTEXT_REGS[REG_RIP]
#define CONTEXT_AEAX	CONTEXT_REGS[REG_RAX]
#define CONTEXT_AEBX	CONTEXT_REGS[REG_RBX]
#define CONTEXT_AECX	CONTEXT_REGS[REG_RCX]
#define CONTEXT_AEDX	CONTEXT_REGS[REG_RDX]
#define CONTEXT_AEBP	CONTEXT_REGS[REG_RBP]
#define CONTEXT_AESI	CONTEXT_REGS[REG_RSI]
#define CONTEXT_AEDI	CONTEXT_REGS[REG_RDI]

#ifdef CPU_x86_64
#define CONTEXT_AEFLAGS	CONTEXT_NAME->__rflags
#else
#ifdef x86_THREAD_STATE32
#define CONTEXT_AEFLAGS	CONTEXT_NAME->__eflags
#else
#define CONTEXT_AEFLAGS	CONTEXT_NAME->eflags
#endif
#endif

#if defined(CPU_i386) || defined(CPU_x86_64)

#include "sigsegv_common_x86.h"

static sigsegv_return_t sigsegv_handler(sigsegv_address_t fault_address,
										sigsegv_address_t /* fault_instruction */,
										 SIGSEGV_THREAD_STATE_TYPE *state) {
	memptr addr = (memptr)(uintptr)((char *)fault_address - fixed_memory_offset);
#if DEBUG
	if (addr >= 0xff000000)
		addr &= 0x00ffffff;
	if (addr < 0x00f00000 || addr > 0x00ffffff) // YYY
		bug("\nsegfault: pc=%08x, " REG_RIP_NAME " =%p, addr=%p (0x%08x)", m68k_getpc(), (void *)CONTEXT_AEIP, fault_address, addr);
	if (fault_address < (uintptr)(fixed_memory_offset - 0x1000000UL)
#ifdef CPU_x86_64
		|| fault_address >= ((uintptr)fixed_memory_offset + 0x100000000UL)
#endif
		)
	{
#ifdef HAVE_DISASM_X86
		if (CONTEXT_AEIP != 0)
		{
			char buf[256];
			
			x86_disasm((const uint8 *)CONTEXT_AEIP, buf, 1);
			panicbug("%s", buf);
		}
#endif
		// raise(SIGBUS);
	}
#endif
	if (fault_address == 0 || CONTEXT_AEIP == 0)
	{
		real_segmentationfault();
		/* not reached (hopefully) */
		return SIGSEGV_RETURN_FAILURE;
	}
	handle_access_fault((CONTEXT_ATYPE) CONTEXT_NAME, addr);
	return SIGSEGV_RETURN_SUCCESS;
}
										 
#endif /* CPU_i386 || CPU_x86_64 */


/*
 *  SIGSEGV global handler
 */

// This function handles the badaccess to memory.
// It is called from the signal handler or the exception handler.
static bool handle_badaccess(mach_port_t thread, MACH_EXCEPTION_DATA_T code, SIGSEGV_THREAD_STATE_TYPE *state)
{
	// We must match the initial count when writing back the CPU state registers
	kern_return_t krc;
	mach_msg_type_number_t count;

	D2(panicbug("handle badaccess"));

	count = SIGSEGV_THREAD_STATE_COUNT;
	krc = thread_get_state(thread, SIGSEGV_THREAD_STATE_FLAVOR, (thread_state_t)state, &count);
	MACH_CHECK_ERROR (thread_get_state, krc);

	sigsegv_address_t fault_address = (sigsegv_address_t)SIGSEGV_FAULT_ADDRESS;
	sigsegv_address_t fault_instruction = (sigsegv_address_t)SIGSEGV_FAULT_INSTRUCTION;

	D2(panicbug("code:%lx %lx %lx %lx", (long)code[0], (long)code[1], (long)code[2], (long)code[3]));
	D2(panicbug("regs eax:%lx", CONTEXT_AEAX));
	D2(panicbug("regs ebx:%lx", CONTEXT_AEBX));
	D2(panicbug("regs ecx:%lx", CONTEXT_AECX));
	D2(panicbug("regs edx:%lx", CONTEXT_AEDX));
	D2(panicbug("regs ebp:%lx", CONTEXT_AEBP));
	D2(panicbug("regs esi:%lx", CONTEXT_AESI));
	D2(panicbug("regs edi:%lx", CONTEXT_AEDI));
	D2(panicbug("regs eip:%lx", CONTEXT_AEIP));
	D2(panicbug("regs eflags:%lx", CONTEXT_AEFLAGS));

	// Call user's handler 
	if (sigsegv_fault_handler(fault_address, fault_instruction, state) == SIGSEGV_RETURN_SUCCESS) {
		D2(panicbug("esi:%lx", CONTEXT_AESI));
		krc = thread_set_state(thread,
								SIGSEGV_THREAD_STATE_FLAVOR, (thread_state_t)state,
								count);
		MACH_CHECK_ERROR (thread_set_state, krc);
		D(panicbug("return from handle bad access with true"));
		return true;
	}

	D(panicbug("return from handle bad access with false"));
	return false;
}


/*
 * We need to forward all exceptions that we do not handle.
 * This is important, there are many exceptions that may be
 * handled by other exception handlers. For example debuggers
 * use exceptions and the exception handler is in another
 * process in such a case. (Timothy J. Wood states in his
 * message to the list that he based this code on that from
 * gdb for Darwin.)
 */
static inline kern_return_t
forward_exception(mach_port_t thread_port,
				  mach_port_t task_port,
				  exception_type_t exception_type,
				  MACH_EXCEPTION_DATA_T exception_data,
				  mach_msg_type_number_t data_count,
				  ExceptionPorts *oldExceptionPorts)
{
	kern_return_t kret;
	unsigned int portIndex;
	mach_port_t port;
	exception_behavior_t behavior;
	thread_state_flavor_t flavor;
	thread_state_data_t thread_state;
	mach_msg_type_number_t thread_state_count;
	
	D(panicbug("forward_exception\n"));

	for (portIndex = 0; portIndex < oldExceptionPorts->maskCount; portIndex++) {
		if (oldExceptionPorts->masks[portIndex] & (1 << exception_type)) {
			// This handler wants the exception
			break;
		}
	}

	if (portIndex >= oldExceptionPorts->maskCount) {
		panicbug("No handler for exception_type = %d. Not fowarding\n", exception_type);
		return KERN_FAILURE;
	}

	port = oldExceptionPorts->handlers[portIndex];
	behavior = oldExceptionPorts->behaviors[portIndex];
	flavor = oldExceptionPorts->flavors[portIndex];
	
	if (flavor && !VALID_THREAD_STATE_FLAVOR(flavor)) {
		fprintf(stderr, "Invalid thread_state flavor = %d. Not forwarding\n", flavor);
		return KERN_FAILURE;
	}

	/*
	 fprintf(stderr, "forwarding exception, port = 0x%x, behaviour = %d, flavor = %d\n", port, behavior, flavor);
	 */

	if (behavior != EXCEPTION_DEFAULT) {
		thread_state_count = THREAD_STATE_MAX;
		kret = thread_get_state (thread_port, flavor, (natural_t *)&thread_state,
								 &thread_state_count);
		MACH_CHECK_ERROR (thread_get_state, kret);
	}

	switch (behavior) {
	case EXCEPTION_DEFAULT:
	  // fprintf(stderr, "forwarding to exception_raise\n");
	  kret = MACH_EXCEPTION_RAISE(port, thread_port, task_port, exception_type,
							 exception_data, data_count);
	  MACH_CHECK_ERROR (MACH_EXCEPTION_RAISE, kret);
	  break;
	case EXCEPTION_STATE:
	  // fprintf(stderr, "forwarding to exception_raise_state\n");
	  kret = MACH_EXCEPTION_RAISE_STATE(port, exception_type, exception_data,
								   data_count, &flavor,
								   (natural_t *)&thread_state, thread_state_count,
								   (natural_t *)&thread_state, &thread_state_count);
	  MACH_CHECK_ERROR (MACH_EXCEPTION_RAISE_STATE, kret);
	  break;
	case EXCEPTION_STATE_IDENTITY:
	  // fprintf(stderr, "forwarding to exception_raise_state_identity\n");
	  kret = MACH_EXCEPTION_RAISE_STATE_IDENTITY(port, thread_port, task_port,
											exception_type, exception_data,
											data_count, &flavor,
											(natural_t *)&thread_state, thread_state_count,
											(natural_t *)&thread_state, &thread_state_count);
	  MACH_CHECK_ERROR (MACH_EXCEPTION_RAISE_STATE_IDENTITY, kret);
	  break;
	default:
	  panicbug("forward_exception got unknown behavior");
	  kret = KERN_FAILURE;
	  break;
	}

	if (behavior != EXCEPTION_DEFAULT) {
		kret = thread_set_state (thread_port, flavor, (natural_t *)&thread_state,
								 thread_state_count);
		MACH_CHECK_ERROR (thread_set_state, kret);
	}

	return kret;
}

/*
 * This is the code that actually handles the exception.
 * It is called by exc_server. For Darwin 5 Apple changed
 * this a bit from how this family of functions worked in
 * Mach. If you are familiar with that it is a little
 * different. The main variation that concerns us here is
 * that code is an array of exception specific codes and
 * codeCount is a count of the number of codes in the code
 * array. In typical Mach all exceptions have a code
 * and sub-code. It happens to be the case that for a
 * EXC_BAD_ACCESS exception the first entry is the type of
 * bad access that occurred and the second entry is the
 * faulting address so these entries correspond exactly to
 * how the code and sub-code are used on Mach.
 *
 * This is a MIG interface. No code in Basilisk II should
 * call this directley. This has to have external C
 * linkage because that is what exc_server expects.
 */
__attribute__ ((visibility("default")))
kern_return_t
CATCH_MACH_EXCEPTION_RAISE(mach_port_t exception_port,
					  mach_port_t thread,
					  mach_port_t task,
					  exception_type_t exception,
					  MACH_EXCEPTION_DATA_T code,
					  mach_msg_type_number_t codeCount)
{
	SIGSEGV_THREAD_STATE_TYPE state;
	kern_return_t krc;

	D(panicbug("catch_exception_raise: %d", exception));

	if (exception == EXC_BAD_ACCESS) {
		switch (code[0]) {
			case KERN_PROTECTION_FAILURE:
			case KERN_INVALID_ADDRESS:
				if (handle_badaccess(thread, code, &state))
					return KERN_SUCCESS;
				break;
		}
	}
	

	// In Mach we do not need to remove the exception handler.
	// If we forward the exception, eventually some exception handler
	// will take care of this exception.
	krc = forward_exception(thread, task, exception, code, codeCount, &ports);
	
	return krc;
}

/* XXX: borrowed from launchd and gdb */
kern_return_t
catch_mach_exception_raise_state(mach_port_t exception_port,
								 exception_type_t exception,
								 MACH_EXCEPTION_DATA_T code,
								 mach_msg_type_number_t code_count,
								 int *flavor,
								 thread_state_t old_state,
								 mach_msg_type_number_t old_state_count,
								 thread_state_t new_state,
								 mach_msg_type_number_t *new_state_count)
{
	memcpy(new_state, old_state, old_state_count * sizeof(old_state[0]));
	*new_state_count = old_state_count;
	return KERN_SUCCESS;
}

/* XXX: borrowed from launchd and gdb */
kern_return_t
catch_mach_exception_raise_state_identity(mach_port_t exception_port,
										  mach_port_t thread_port,
										  mach_port_t task_port,
										  exception_type_t exception,
										  MACH_EXCEPTION_DATA_T code,
										  mach_msg_type_number_t code_count,
										  int *flavor,
										  thread_state_t old_state,
										  mach_msg_type_number_t old_state_count,
										  thread_state_t new_state,
										  mach_msg_type_number_t *new_state_count)
{
	kern_return_t kret;
	
	memcpy(new_state, old_state, old_state_count * sizeof(old_state[0]));
	*new_state_count = old_state_count;
	
	kret = mach_port_deallocate(mach_task_self(), task_port);
	MACH_CHECK_ERROR(mach_port_deallocate, kret);
	kret = mach_port_deallocate(mach_task_self(), thread_port);
	MACH_CHECK_ERROR(mach_port_deallocate, kret);
	
	return KERN_SUCCESS;
}

/*
 * This is the entry point for the exception handler thread. The job
 * of this thread is to wait for exception messages on the exception
 * port that was setup beforehand and to pass them on to exc_server.
 * exc_server is a MIG generated function that is a part of Mach.
 * Its job is to decide what to do with the exception message. In our
 * case exc_server calls catch_exception_raise on our behalf. After
 * exc_server returns, it is our responsibility to send the reply.
 */
static void *
handleExceptions(void * /*priv*/)
{
	D(panicbug("handleExceptions\n"));

	mach_msg_header_t *msg, *reply;
	kern_return_t krc;

	msg = (mach_msg_header_t *)msgbuf;
	reply = (mach_msg_header_t *)replybuf;
			
	for (;;) {
		krc = mach_msg(msg, MACH_RCV_MSG, MSG_SIZE, MSG_SIZE,
				_exceptionPort, 0, MACH_PORT_NULL);
		MACH_CHECK_ERROR(mach_msg, krc);

		if (!MACH_EXC_SERVER(msg, reply)) {
			fprintf(stderr, "exc_server hated the message\n");
			exit(1);
		}

		krc = mach_msg(reply, MACH_SEND_MSG, reply->msgh_size, 0,
				 msg->msgh_local_port, 0, MACH_PORT_NULL);
		if (krc != KERN_SUCCESS) {
			fprintf(stderr, "Error sending message to original reply port, krc = %d, %s",
				krc, mach_error_string(krc));
			exit(1);
		}
	}
}

static bool sigsegv_do_install_handler(sigsegv_fault_handler_t handler)
{
	D(panicbug("sigsegv_do_install_handler\n"));

	//
	//Except for the exception port functions, this should be
	//pretty much stock Mach. If later you choose to support
	//other Mach's besides Darwin, just check for __MACH__
	//here and __APPLE__ where the actual differences are.
	//
	if (sigsegv_fault_handler != NULL) {
		sigsegv_fault_handler = handler;
		return true;
	}

	kern_return_t krc;

	// create the the exception port
	krc = mach_port_allocate(mach_task_self(),
			  MACH_PORT_RIGHT_RECEIVE, &_exceptionPort);
	if (krc != KERN_SUCCESS) {
		mach_error("mach_port_allocate", krc);
		return false;
	}

	// add a port send right
	krc = mach_port_insert_right(mach_task_self(),
			      _exceptionPort, _exceptionPort,
			      MACH_MSG_TYPE_MAKE_SEND);
	if (krc != KERN_SUCCESS) {
		mach_error("mach_port_insert_right", krc);
		return false;
	}

	// get the old exception ports
	ports.maskCount = sizeof (ports.masks) / sizeof (ports.masks[0]);
	krc = thread_get_exception_ports(mach_thread_self(), EXC_MASK_BAD_ACCESS, ports.masks,
 				&ports.maskCount, ports.handlers, ports.behaviors, ports.flavors);
 	if (krc != KERN_SUCCESS) {
 		mach_error("thread_get_exception_ports", krc);
 		return false;
 	}

	// set the new exception port
	//
	// We could have used EXCEPTION_STATE_IDENTITY instead of
	// EXCEPTION_DEFAULT to get the thread state in the initial
	// message, but it turns out that in the common case this is not
	// neccessary. If we need it we can later ask for it from the
	// suspended thread.
	//
	// Even with THREAD_STATE_NONE, Darwin provides the program
	// counter in the thread state.  The comments in the header file
	// seem to imply that you can count on the GPR's on an exception
	// as well but just to be safe I use MACHINE_THREAD_STATE because
	// you have to ask for all of the GPR's anyway just to get the
	// program counter. In any case because of update effective
	// address from immediate and update address from effective
	// addresses of ra and rb modes (as good an name as any for these
	// addressing modes) used in PPC instructions, you will need the
	// GPR state anyway.
	krc = thread_set_exception_ports(mach_thread_self(), EXC_MASK_BAD_ACCESS, _exceptionPort,
				EXCEPTION_DEFAULT | MACH_EXCEPTION_CODES, SIGSEGV_THREAD_STATE_FLAVOR);
	if (krc != KERN_SUCCESS) {
		mach_error("thread_set_exception_ports", krc);
		return false;
	}

	// create the exception handler thread
	if (pthread_create(&exc_thread, NULL, &handleExceptions, NULL) != 0) {
		panicbug("creation of exception thread failed\n");
		return false;
	}

	// do not care about the exception thread any longer, let is run standalone
	(void)pthread_detach(exc_thread);

	D(panicbug("Sigsegv installed\n"));
	sigsegv_fault_handler = handler;
	return true;
}

void install_sigsegv() {
	sigsegv_do_install_handler(sigsegv_handler);
}

void uninstall_sigsegv()
{
	/* TODO */
}
