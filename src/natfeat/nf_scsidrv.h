#ifndef _ARANYM_SCSIDRV_H
#define _ARANYM_SCSIDRV_H

#include "nf_base.h"

// The maximum number of SCSI Driver handles, must be the same as in stub
#define SCSI_MAX_HANDLES 32

class SCSIDriver : public NF_Base
{
private:
	int fds[SCSI_MAX_HANDLES];

	int32 check_device_file(Uint32 id);
	int32 interface_features(memptr busName, memptr features, memptr transferLen);
	int32 inquire_bus(Uint32 id);
	int32 Open(Uint32 handle, Uint32 id);
	int32 Close(Uint32 handle);
	int32 inout(Uint32 handle, Uint32 dir, Uint32 id, unsigned char *cmd, Uint32 cmd_len, unsigned char *buffer, Uint32 transfer_len, unsigned char *sense_buffer, Uint32 timeout);
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
