#ifndef _MINGW_INCLUDE_H
#define _MINGW_INCLUDE_H

/*	FIXME: O_SYNC not defined in mingw, is there a replacement value ?
 *	Does it make floppy working even without it ?
 */
#define O_SYNC 0

/*	FIXME: These are not defined in my current mingw setup, is it
 *  available in more uptodates mingw packages ? I took these from
 *  the wine project
 */
#ifndef FILE_DEVICE_MASS_STORAGE
#define FILE_DEVICE_MASS_STORAGE        0x0000002d
#endif
#ifndef IOCTL_STORAGE_BASE
#define IOCTL_STORAGE_BASE FILE_DEVICE_MASS_STORAGE
#endif
#ifndef IOCTL_STORAGE_EJECT_MEDIA
#define IOCTL_STORAGE_EJECT_MEDIA        CTL_CODE(IOCTL_STORAGE_BASE, 0x0202, METHOD_BUFFERED, FILE_READ_ACCESS)
#endif

#define bcopy(src, dest, size)	memcpy(dest, src, n)

#define usleep(microseconds)	{}

#endif /* _MINGW_INCLUDE_H */
