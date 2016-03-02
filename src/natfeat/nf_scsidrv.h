#ifndef _ARANYM_SCSIDRV_H
#define _ARANYM_SCSIDRV_H

#include "nf_base.h"

// The maximum number of SCSI Driver handles, must be the same as in stub
#define SCSI_MAX_HANDLES 32

class SCSIDriver : public NF_Base
{
private:
	struct {
		int fd;
		int id_lo;
		int error;
	} handle_meta_data[SCSI_MAX_HANDLES];

#ifdef HAVE_LIBUDEV
	struct udev *udev;
	struct udev_monitor *mon;
	int udev_mon_fd;
	struct timeval tv;
#endif
	
	bool check_mchg_udev(void);
	int32 check_device_file(Uint32 id);
	void set_error(Uint32 rwflag, Uint32 errbit);
	int32 interface_features(memptr busName, memptr features, memptr transferLen);
	int32 inquire_bus(Uint32 id);
	int32 Open(Uint32 handle, Uint32 id);
	int32 Close(Uint32 handle);
	int32 inout(Uint32 handle, Uint32 dir, unsigned char *cmd, Uint32 cmd_len, unsigned char *buffer, Uint32 transfer_len, unsigned char *sense_buffer, Uint32 timeout);
	int32 Error(Uint32 handle, Uint32 rwflag, Uint32 errnum);
	int32 check_dev(Uint32 id);
	
public:
	SCSIDriver();
	~SCSIDriver();

	const char *name() { return "NF_SCSIDRV"; }
	bool isSuperOnly() { return true; }
	void reset();
	int32 dispatch(uint32 fncode);
};

#endif /* _ARANYM_SCSIDRV_H */
