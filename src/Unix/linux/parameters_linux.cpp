/* MJ 2001 */

#include "sysdeps.h"
#include "tools.h"
#include "parameters.h"

#include <linux/hdreg.h>
#include <cstring>
#include <cstdlib>

/*
 * avoid an undefined reference when compiling against
 * older glibc ABI, and using _FORTIFY_SOURCE
 */
#if defined(FD_SETSIZE) && defined(__NFDBITS)
long int __fdelt_chk(long int d)
{
	if (d < 0 || d >= FD_SETSIZE)
		abort();
	return d / __NFDBITS;
}
#endif


int get_geometry(const char *dev_path, geo_type geo) {
  int fd;
  int par = -1;
  struct hd_geometry g;
  struct stat s;

  fd = open (dev_path, O_RDONLY);
  if (fd < 0) return -1;

  switch (geo) {
    case geoCylinders:
                       if (strstr(dev_path, "/dev/") == dev_path) {
                         if (ioctl(fd, HDIO_GETGEO, &g)) break;
                           else { par = g.cylinders; break; }
                       } else if (fstat(fd, &s) == 0) {
                         if (s.st_size%(16*63*512*2)) break;
                         par = s.st_size/(16*63*512);
                         if (par > 1024) { par = -1; break; }
                         break;
		       } else break;
    case geoHeads:
                       if (strstr(dev_path, "/dev/") == dev_path) {
                         if (ioctl(fd, HDIO_GETGEO, &g)) break;
                           else { par = g.heads; break; }
                       } else if (fstat(fd, &s) == 0) {
                         if (s.st_size%(16*63*512*2)) break;
                         par = s.st_size/(16*63*512);
                         if (par > 1024) { par = -1; break; }
                         par = 16;
                         break;
                       } else break;
    case geoSpt:
                       if (strstr(dev_path, "/dev/") == dev_path) {
                         if (ioctl(fd, HDIO_GETGEO, &g)) break;
                           else { par = g.sectors; break; }
                       } else if (fstat(fd, &s) == 0) {
                         if (s.st_size%(16*63*512*2)) break;
                         par = s.st_size/(16*63*512);
                         if (par > 1024) { par = -1; break; }
                         par = 63;
                         break;
                       } else break;
    case geoByteswap:  break;
  }
  close(fd);
  return par;
}
