/*
 * bpf_helper.c - ethernet driver support tool (enabling Berkley Packet Filter access)
 *
 * Copyright (c) 2009 ARAnyM dev team (see AUTHORS)
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
 */
#include <sys/fcntl.h>
#include <sys/errno.h>
#include <sys/wait.h>

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "fd_trans.h"

#ifdef OS_darwin

#include <Security/Authorization.h>
#include <Security/AuthorizationTags.h>

static int fix_permissions(const char* filename)
{
	printf("fix_permissions(%s)\n", filename);
    OSStatus err_status = noErr;
	AuthorizationFlags auth_flags = kAuthorizationFlagDefaults;
	AuthorizationRef auth_ref;

	err_status = AuthorizationCreate(NULL, kAuthorizationEmptyEnvironment, auth_flags, &auth_ref);
	if (err_status != errAuthorizationSuccess)
		return 0;

	printf("Getting authorizsation for root execution...\n");
	AuthorizationItem auth_items = { kAuthorizationRightExecute, 0, NULL, 0 };
	AuthorizationRights auth_rights = { 1, &auth_items };
	auth_flags = kAuthorizationFlagDefaults |
		kAuthorizationFlagInteractionAllowed |
		kAuthorizationFlagPreAuthorize |
		kAuthorizationFlagExtendRights;
	err_status = AuthorizationCopyRights(auth_ref, &auth_rights, NULL, auth_flags, NULL);

	if (err_status == errAuthorizationSuccess)
	{
		char fn_copy[strlen(filename)+1];
		strcpy(fn_copy, filename);
		char *exec_args[3] = {"root:admin", fn_copy, NULL};

		printf("Executing chown %s %s\n", exec_args[0], exec_args[1]);
		err_status = AuthorizationExecuteWithPrivileges(auth_ref, "/usr/sbin/chown", kAuthorizationFlagDefaults, exec_args, NULL);
		if (err_status != errAuthorizationSuccess)
		{
			fprintf(stderr, "Error while executing chown: %d\n", (int)err_status);
		} else
		{
			int child;
			if (wait(&child) < 0 || !WIFEXITED(child) || WEXITSTATUS(child) != 0)
			{
				fprintf(stderr, "Executing chown failed: %d\n", (int)WEXITSTATUS(child));
				err_status = errAuthorizationToolExecuteFailure;
			} else
			{
				exec_args[0] = "ug+s,g+x";
				exec_args[1] = fn_copy;
				exec_args[2] = NULL;
				printf("Executing chmod %s %s\n", exec_args[0], exec_args[1]);
				err_status = AuthorizationExecuteWithPrivileges(auth_ref, "/bin/chmod", kAuthorizationFlagDefaults, exec_args, NULL);
				if (err_status != errAuthorizationSuccess)
				{
					fprintf(stderr, "Error while executing chmod: %d\n", (int)err_status);
				} else {
					if (wait(&child) < 0 || !WIFEXITED(child) || WEXITSTATUS(child) != 0)
					{
						fprintf(stderr, "Executing chmod failed: %d\n", (int)WEXITSTATUS(child));
						err_status = errAuthorizationToolExecuteFailure;
					}
				}
			}
		}
	}
	AuthorizationFree(auth_ref, kAuthorizationFlagDefaults);
	return err_status == noErr;
}

#else

static int fix_permissions(const char* filename)
{
	fprintf(stderr, "This tool must be run as root. Please fix the permissions manually:\n"
			"   chown root %s\n"
			"   chmod u+s %s\n",
			filename, filename);
	return 0;
}

#endif


int main(int argc, char **argv)
{
	char *exec_name = argv[0];
    char bpf_dev[14];
    int bpf;
	int i;
	int exit_code = 0;

	// check if we are running as root user
	if(geteuid() != 0)
	{
		printf("%s: Running as user %d instead of root. Fixing permissions.\n", exec_name, geteuid());
		// try fixing the permissions
		if (fix_permissions(argv[0])) {
			printf("%s: Permissions fixed. Restarting.\n", exec_name);
			int ret = execvp(argv[0], argv);
			_exit(ret);
		}
    	fprintf(stderr, "%s: Unable to fix permissions.\n", exec_name);
		exit_code = 2;
		bpf = -1;
	} else
	{

		// try opening the next free BPF device
		for(i=0; i <= 99; i++)
		{
			sprintf(bpf_dev, "/dev/bpf%d", i);
			bpf=open(bpf_dev, O_RDWR);
			if (bpf > 0) {
				fprintf(stderr, "%s: %s opened\n", exec_name, bpf_dev);
				break;
			}
			else if (errno == ENOENT) {
				break;
			}
			else {
				fprintf(stderr, "%s: opening %s failed. %s\n", exec_name, bpf_dev, strerror(errno));
			}
		}

		if (bpf < 0)
		{
			fprintf(stderr, "%s: Unable to open any bpf device. Giving up.\n", exec_name);
			exit_code = 1;
		} else
		{
			fd_send(bpf);
		}
	}

	printf("%s: done\n", exec_name);
    return exit_code;
}
