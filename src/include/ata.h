
//  Copyright (C) 2000  MandrakeSoft S.A.
//
//    MandrakeSoft S.A.
//    43, rue d'Aboukir
//    75002 Paris - France
//    http://www.linux-mandrake.com/
//    http://www.mandrakesoft.com/
//
//  This library is free software; you can redistribute it and/or
//  modify it under the terms of the GNU Lesser General Public
//  License as published by the Free Software Foundation; either
//  version 2 of the License, or (at your option) any later version.
//
//  This library is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
//  Lesser General Public License for more details.
//
//  You should have received a copy of the GNU Lesser General Public
//  License along with this library; if not, write to the Free Software
//  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA

#define BX_USE_HD_SMF	0
// muj trick
#define LOWLEVEL_CDROM cdrom_interface
// end of muj trick

typedef enum _sense {
      SENSE_NONE = 0, SENSE_NOT_READY = 2, SENSE_ILLEGAL_REQUEST = 5
} sense_t;

typedef enum _asc {
      ASC_INV_FIELD_IN_CMD_PACKET = 0x24,
      ASC_MEDIUM_NOT_PRESENT = 0x3a,
      ASC_SAVING_PARAMETERS_NOT_SUPPORTED = 0x39,
      ASC_LOGICAL_BLOCK_OOR = 0x21
} asc_t;

class LOWLEVEL_CDROM;

#define loff_t	long
#ifdef ssize_t
#undef ssize_t
#endif
#define ssize_t long
#ifdef size_t
#undef size_t
#endif
#define size_t long
#define Boolean bool
#define Bit8u unsigned char
#define Bit16u unsigned short
#define Bit32u unsigned int
#define uint8 unsigned char
#define uint16 unsigned short
#define uint32 unsigned int

class device_image_t
{
  public:
      // Open a image. Returns non-negative if successful.
      virtual int open (const char* pathname) = 0;

      // Close the image.
      virtual void close () = 0;

      // Position ourselves. Return the resulting offset from the
      // beginning of the file.
      virtual loff_t lseek (loff_t offset, int whence) = 0;

      // Read count bytes to the buffer buf. Return the number of
      // bytes read (count).
      virtual ssize_t read (void* buf, size_t count) = 0;

      // Write count bytes from buf. Return the number of bytes
      // written (count).
      virtual ssize_t write (const void* buf, size_t count) = 0;

      unsigned cylinders;
      unsigned heads;
      unsigned sectors;
      bool byteswap;
};

class default_image_t : public device_image_t
{
  public:
      // Open a image. Returns non-negative if successful.
      int open (const char* pathname);

      // Close the image.
      void close ();

      // Position ourselves. Return the resulting offset from the
      // beginning of the file.
      loff_t lseek (loff_t offset, int whence);

      // Read count bytes to the buffer buf. Return the number of
      // bytes read (count).
      ssize_t read (void* buf, size_t count);

      // Write count bytes from buf. Return the number of bytes
      // written (count).
      ssize_t write (const void* buf, size_t count);

  private:
      int fd;
      
};

#if EXTERNAL_DISK_SIMULATOR
#include "external-disk-simulator.h"
#endif

typedef struct {
  struct {
    Boolean busy;
    Boolean drive_ready;
    Boolean write_fault;
    Boolean seek_complete;
    Boolean drq;
    Boolean corrected_data;
    Boolean index_pulse;
    unsigned index_pulse_count;
    Boolean err;
    } status;
  Bit8u    error_register;
  // Bit8u    drive_select; this field was moved :^(
  Bit8u    head_no;
  union {
    Bit8u    sector_count;
    struct {
      unsigned c_d : 1;
      unsigned i_o : 1;
      unsigned rel : 1;
      unsigned tag : 5;
    } interrupt_reason;
  };
  Bit8u    sector_no;
  union {
    Bit16u   cylinder_no;
    Bit16u   byte_count;
  };
  Bit8u    buffer[2048];
  unsigned buffer_index;
  Bit8u    current_command;
  Bit8u    sectors_per_block;
  Bit8u    lba_mode;
  struct {
    Boolean reset;       // 0=normal, 1=reset controller
    Boolean disable_irq; // 0=allow irq, 1=disable irq
    } control;
  Bit8u    reset_in_progress;
  Bit8u    features;
  } controller_t;

struct sense_info_t {
  sense_t sense_key;
  struct {
    Bit8u arr[4];
  } information;
  struct {
    Bit8u arr[4];
  } specific_inf;
  struct {
    Bit8u arr[3];
  } key_spec;
  Bit8u fruc;
  Bit8u asc;
  Bit8u ascq;
};

struct error_recovery_t {
  unsigned char data[8];

  error_recovery_t ();
};

uint16 read_16bit(const uint8* buf);
uint32 read_32bit(const uint8* buf);


#ifdef LOWLEVEL_CDROM
#  include "cdrom.h"
#endif


struct cdrom_t
{
  Boolean ready;
  Boolean locked;
#ifdef LOWLEVEL_CDROM
  LOWLEVEL_CDROM* cd;
#endif
  uint32 capacity;
  int next_lba;
  int remaining_blocks;
  struct currentStruct {
    error_recovery_t error_recovery;
  } current;
};

struct atapi_t
{
  uint8 command;
  int drq_bytes;
  int total_bytes_remaining;
};

#if BX_USE_HD_SMF
#  define BX_HD_SMF  static
#  define BX_HD_THIS bx_hard_drive.
#else
#  define BX_HD_SMF
#  define BX_HD_THIS this->
#endif

#define BX_SELECTED_HD BX_HD_THIS s[BX_HD_THIS drive_select]
#define CDROM_SELECTED (BX_HD_THIS s[BX_HD_THIS drive_select].device_type == IDE_CDROM)
#define DEVICE_TYPE_STRING ((CDROM_SELECTED) ? "CD-ROM" : "DISK")

typedef enum {
      IDE_DISK, IDE_CDROM
} device_type_t;

class bx_hard_drive_c {
public:

  bx_hard_drive_c(void);
  ~bx_hard_drive_c(void);
  BX_HD_SMF void   close_harddrive(void);
  BX_HD_SMF void   init(/*bx_devices_c *d, bx_cmos_c *cmos*/);

#if !BX_USE_HD_SMF
  Bit32u read(Bit32u address, unsigned io_len);
  void   write(Bit32u address, Bit32u value, unsigned io_len);
#endif

  static Bit32u read_handler(void *this_ptr, Bit32u address, unsigned io_len);
  static void   write_handler(void *this_ptr, Bit32u address, Bit32u value, unsigned io_len);

private:

  BX_HD_SMF Bit32u calculate_logical_address();
  BX_HD_SMF void increment_address();
  BX_HD_SMF void identify_drive(unsigned drive);
  BX_HD_SMF void identify_ATAPI_drive(unsigned drive);
  BX_HD_SMF void command_aborted(unsigned command);

  BX_HD_SMF void init_send_atapi_command(Bit8u command, int req_length, int alloc_length, bool lazy = false);
  BX_HD_SMF void ready_to_send_atapi();
  BX_HD_SMF void raise_interrupt();
  BX_HD_SMF void atapi_cmd_error(sense_t sense_key, asc_t asc);
  BX_HD_SMF void init_mode_sense_single(const void* src, int size);
  BX_HD_SMF void atapi_cmd_nop();

  struct sStruct {
    device_image_t* hard_drive;
    device_type_t device_type;
    // 512 byte buffer for ID drive command
    // These words are stored in native word endian format, as
    // they are fetched and returned via a return(), so
    // there's no need to keep them in x86 endian format.
    Bit16u id_drive[256];

    controller_t controller;
    cdrom_t cdrom;
    sense_info_t sense;
    atapi_t atapi;

    } s[2];

  unsigned drive_select;

  // bx_devices_c *devices;
  };

#if BX_USE_HD_SMF
extern bx_hard_drive_c bx_hard_drive;
#endif

// emil.h
#ifndef UNUSED
#  define UNUSED(x) ((void)x)
#endif

#if 0
typedef struct {
  Boolean present;
  Boolean byteswap;
  char path[512];
  unsigned int cylinders;
  unsigned int heads;
  unsigned int spt;
  } bx_disk_options;
 
struct bx_cdrom_options
{
  Boolean present;
  char dev[512];
  Boolean inserted;
};

typedef struct {
  char      *path;
  Boolean   cmosImage;
  unsigned int time0;
  } bx_cmos_options;
 
typedef struct {
  bx_disk_options   diskc;
  bx_disk_options   diskd;
  bx_cdrom_options  cdromd;
  char              bootdrive[2];
  bx_cmos_options   cmos;
  Boolean           newHardDriveSupport;
  } bx_options_t;
#endif
#define bx_ptr_t void *

// extern bx_options_t bx_options;
// extern bx_devices_c bx_devices;

#define bx_panic	printf
#define bx_printf	printf

typedef struct {
  Boolean floppy;
  Boolean keyboard;
  Boolean video;
  Boolean disk;
  Boolean pit;
  Boolean pic;
  Boolean bios;
  Boolean cmos;
  Boolean a20;
  Boolean interrupts;
  Boolean exceptions;
  Boolean unsupported;
  Boolean temp;
  Boolean reset;
  Boolean debugger;
  Boolean mouse;
  Boolean io;
  Boolean xms;
  Boolean v8086;
  Boolean paging;
  Boolean creg;
  Boolean dreg;
  Boolean dma;
  Boolean unsupported_io;
  Boolean serial;
  Boolean cdrom;
#ifdef MAGIC_BREAKPOINT
  Boolean magic_break_enabled;
#endif /* MAGIC_BREAKPOINT */
  void* record_io;
  } bx_debug_t;

extern bx_debug_t bx_dbg;

