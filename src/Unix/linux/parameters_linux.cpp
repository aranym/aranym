/* MJ 2001 */

#include "sysdeps.h"
#include "tools.h"
#include "parameters.h"

#include <linux/hdreg.h>
#include <cstring>
#include <cstdlib>

int get_geometry(char *dev_path, geo_type geo) {
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

/*
 * Get the path to folder with user-specific files (configuration, NVRAM)
 */
char *getConfFolder(char *buffer, unsigned int bufsize)
{
	// local cache
	static char path[512] = "";

	if (strlen(path) == 0) {
		// Unix-like systems define HOME variable as the user home folder
		char *home = getenv("HOME");
		if (home == NULL)
			home = "";	// alternatively use current directory

		int homelen = strlen(home);
		if (homelen > 0) {
			unsigned int len = strlen(ARANYMHOME);
			if ((homelen + 1 + len + 1) < bufsize) {
				strcpy(path, home);
				if (homelen)
					strcat(path, DIRSEPARATOR);
				strcat(path, ARANYMHOME);
			}
		}
	}

	return safe_strncpy(buffer, path, bufsize);
}

char *getDataFolder(char *buffer, unsigned int bufsize)
{
	// data folder is defined at configure time in DATADIR (using --datadir)
	return safe_strncpy(buffer, DATADIR, bufsize);
}

