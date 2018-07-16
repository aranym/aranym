#include "sysdeps.h"

#ifdef NFEXEC_SUPPORT
#include "nf_hostexec.h"
#include "toserror.h"

#include <algorithm>
#include <cstdlib>
#include <vector>
#include <errno.h>
#include <signal.h>
#if !defined(_WIN32) || defined(__CYGWIN___)
#include <sys/wait.h>
#ifndef WNOHANG
#define WNOHANG 0
#endif
#endif

#include <unistd.h>
#include "maptab.h"
#include "nf_objs.h"

#define DEBUG 0
#include "debug.h"

// The driver interface version, 1.00
#define INTERFACE_VERSION 0x0100

enum HOSTEXEC_OPERATIONS {
	HOSTEXEC_INTERFACE_VERSION = 0,
	HOSTEXEC_EXEC,
	HOSTEXEC_EXECV,
};



int32 HostExec::dispatch(uint32 fncode)
{
	int32 ret = TOS_EINVFN;

	if (fncode != HOSTEXEC_INTERFACE_VERSION && !bx_options.natfeats.hostexec_enabled)
		return TOS_EPERM;

	switch (fncode)
	{
	case HOSTEXEC_INTERFACE_VERSION:
		ret = INTERFACE_VERSION;
		break;

	case HOSTEXEC_EXEC:
		{
		memptr pPathStr = getParameter(0);
		uint32 length = getParameter(1);
		D(bug("HostExec($%08x, %d)", pPathStr, length));

		// that function should only be issued by the device driver
		if (!regs.s)
			return TOS_EPERM;
		for (uint32 i = 0; i < length; ++i)
		{
			uint8 ch = ReadNFInt8(pPathStr++);
			if (ch == 0 || ch == 0x0a)
			{
				exec(str);
				str.clear();
			} else
			{
				str += getc(ch);
			}
		}

		ret = length;
		}
		break;

	case HOSTEXEC_EXECV:
		ret = execv(getParameter(0), getParameter(1));
		break;
	}

	return ret;
}


void HostExec::reset()
{
	str.clear();
}


std::string HostExec::getc(uint8 c) const
{
	std::string ret;

	if (c == 0x0d)
	{
		/* ignore CRs */
		return ret;
	}

	unsigned short ch = atari_to_utf16[c];
	if (ch < 0x80)
	{
		ret.push_back(c);
	} else if (ch < 0x800)
	{
		ret.push_back(((ch >> 6) & 0x3f) | 0xc0);
		ret.push_back((ch & 0x3f) | 0x80);
	} else
	{
		ret.push_back(((ch >> 12) & 0x0f) | 0xe0);
		ret.push_back(((ch >> 6) & 0x3f) | 0x80);
		ret.push_back((ch & 0x3f) | 0x80);
	}

	return ret;
}

std::string HostExec::translatePath(const std::string& path) const
{
	// loosely inspired by HostFs::xfs_native_init()
	std::string mountedPath = path;

	std::replace(mountedPath.begin(), mountedPath.end(), '/', DIRSEPARATOR[0]);
	std::replace(mountedPath.begin(), mountedPath.end(), '\\', DIRSEPARATOR[0]);

	char drv = 0;
	if (mountedPath.length() > 3 && mountedPath[0] == DIRSEPARATOR[0])
	{
		// /<drive>/path
		drv = mountedPath[1];
		mountedPath = mountedPath.substr(3);
	} else if (mountedPath.length() > 5 && mountedPath.compare(0, 3, "u:" DIRSEPARATOR) == 0)
	{
		// u:/<drive>/path
		drv = mountedPath[3];
		mountedPath = mountedPath.substr(5);
	} else if (mountedPath.length() > 3 && mountedPath[1] == ':')
	{
		// <drive>:/path
		drv = mountedPath[0];
		mountedPath = mountedPath.substr(3);
	} else
	{
		mountedPath.clear();
	}

	if (!mountedPath.empty() && drv != 0)
	{
		int dnum = DriveFromLetter(drv);
		if (dnum >= 0 && dnum < HOSTFS_MAX_DRIVES)
		{
			std::string mountRoot = bx_options.aranymfs.drive[dnum].rootPath;
			if (!mountRoot.empty())
			{
				D(bug("HostExec: %s -> %s", mountedPath.c_str(), mountRoot.c_str()));

				// rootPath is always terminated with '/'
				mountedPath = mountRoot + mountedPath;

				// security check
				char* resolvedPath = my_canonicalize_file_name(mountedPath.c_str(), false);
				if (resolvedPath != NULL)
				{
					mountedPath = resolvedPath;
					if (mountedPath.compare(0, mountRoot.length(), mountRoot) != 0)
					{
						mountedPath.clear();
					}
					free(resolvedPath);
				} else
				{
					mountedPath.clear();
				}
			} else
			{
				mountedPath.clear();
			}
		} else
		{
			mountedPath.clear();
		}

		if (mountedPath.empty())
		{
			D(bug("HostExec: illegal drive number (%c) or path (%s)", drv, path.c_str()));
		}
	} else
	{
		D(bug("HostExec: only absolute paths to an executable are supported"));
	}

	return mountedPath;
}

std::string HostExec::trim(const std::string& str) const
{
	std::string ret = str;

	// trim trailing spaces
	size_t endpos = ret.find_last_not_of(" \t");
	if (std::string::npos != endpos)
	{
		ret = ret.substr(0, endpos+1);
	}

	// trim leading spaces
	size_t startpos = ret.find_first_not_of(" \t");
	if (std::string::npos != startpos)
	{
		ret = ret.substr(startpos);
	}

	return ret;
}

static void child_signal(int sig)
{
	int status;
	pid_t pid;

	if (sig == SIGCHLD)
	{
		for (;;)
		{
			pid = waitpid((pid_t)-1, &status, WNOHANG);
			if (pid <= 0)
				return;
			if (WIFEXITED(status))
			{
				D(bug("child %d exited with status %d", pid, WEXITSTATUS(status)));
#ifdef WIFSTOPPED
			} else if (WIFSTOPPED(status))
			{
				D(bug("child %d was stopped by signal %d", pid, WSTOPSIG(status)));
#endif
#ifdef WIFCONTINUED
			} else if (WIFCONTINUED(status))
			{
				D(bug("child %d was continued by signal %d", pid, WSTOPSIG(status)));
#endif
			} else if (WIFSIGNALED(status))
			{
				D(bug("child %d was killed by signal %d", pid, WTERMSIG(status)));
			}
		}
	}
}


void HostExec::exec(const std::string& path) const
{
	D(bug("HostExec::exec: %s", path.c_str()));

	// we assume that executable path doesn't contain spaces
	// (but its arguments are free to use '\ ' if needed)
	std::string extractedPath = trim(path);
	std::string extractedArgs;
	size_t i = path.find_first_of(" \t");
	if (i != std::string::npos)
	{
		extractedPath = path.substr(0, i);
		extractedArgs = path.substr(i + 1);
	}

	std::string translatedPath = translatePath(extractedPath);
	if (!translatedPath.empty())
	{
		std::vector<std::string> args;
		args.push_back("/bin/sh");
		args.push_back("-c");
		args.push_back(translatedPath + ' ' + extractedArgs);

		std::vector<char*> argv;
		for (std::vector<std::string>::const_iterator it = args.begin(); it != args.end(); ++it)
		{
			D(bug("HostExec::exec: %s", it->c_str()));
			argv.push_back(const_cast<char*>(it->c_str()));
		}
		argv.push_back(NULL);
		doexecv(&argv[0]);
	}
}

int HostExec::doexecv(char *const argv[]) const
{
#if defined(_WIN32) && !defined(__CYGWIN___)
	int ret = win32_execv(argv[0], &argv[0]);
	if (ret < 0)
		ret = errnoHost2Mint(-ret, TOS_ENSMEM);
	return ret;
#else
	int pipefds[2];
	pid_t pid;
	int ret;
	int count, err;

	if (pipe(pipefds))
	{
		return errnoHost2Mint(errno, TOS_EINVAL);
	}
	if (fcntl(pipefds[1], F_SETFD, fcntl(pipefds[1], F_GETFD) | FD_CLOEXEC))
	{
		err = errno;
		::close(pipefds[0]);
		::close(pipefds[1]);
		return errnoHost2Mint(err, TOS_EINVAL);
	}

	pid = fork();
	ret = pid;
	if (pid == -1)
	{
		D(bug("HostExec::exec: fork() failed"));
		ret = errnoHost2Mint(errno, TOS_ENSMEM);
	} else if (pid == 0)
	{
		::close(pipefds[0]);
		::execv(argv[0], &argv[0]);
		err = errno;
		while ((count = ::write(pipefds[1], &err, sizeof(err))) == -1)
			if (errno != EAGAIN && errno != EINTR)
				break;
		_exit(127);
	} else
	{
		::close(pipefds[1]);
		while ((count = ::read(pipefds[0], &err, sizeof(err))) == -1)
			if (errno != EAGAIN && errno != EINTR)
				break;
		::close(pipefds[0]);
		if (count)
		{
			if (count != sizeof(err))
				err = errno;
			bug("NF_HOSTEXEC: exec failed: %s", strerror(err));
			ret = errnoHost2Mint(err, TOS_ENSMEM);
		}
		/*
		 * we get here if we could not read the errno from the pipe,
		 * because the pipe was closed by a successful execv() in the child
		 */
	}

	{
		static int signal_handler_installed;

		if (!signal_handler_installed)
		{
			struct sigaction sa;
			sigset_t set;

			memset(&sa, 0, sizeof(sa));
			sigemptyset(&set);
#ifdef SA_NOCLDSTOP
			sa.sa_flags |= SA_NOCLDSTOP;
#endif
			sa.sa_handler = child_signal;
			sigaction(SIGCHLD, &sa, NULL);

			signal_handler_installed = 1;
		}
	}

	printf("execv: %d\n", ret);
	return ret;
#endif
}

int HostExec::execv(int argc, memptr argv) const
{
	static const int MAXPATHNAMELEN = 2048;
	int i;
	int ret;
	
	if (argc <= 0)
		return TOS_EINVAL;
	memptr argv0 = ReadNFInt32(argv);
	char fname[MAXPATHNAMELEN];
	Atari2HostUtf8Copy(fname, argv0, sizeof(fname));
	std::string translatedPath(translatePath(std::string(fname)));

	if (translatedPath.empty())
		return TOS_EINVAL;

	std::vector<char*> execargv;
	execargv.push_back(const_cast<char*>(translatedPath.c_str()));
	
	for (i = 1; i < argc; i++)
	{
		argv += 4; /* ATARI_SIZEOF_PTR */
		memptr arg = ReadNFInt32(argv);
		size_t len = Atari2HostSafeStrlen(arg);
		if (len == 0)
		{
			ret = TOS_EINVAL;
			goto fail;
		}
		char *parg = (char *)malloc(len);
		if (parg == NULL)
		{
			ret = TOS_ENSMEM;
			goto fail;
		}
		Atari2Host_memcpy(parg, arg, len);
		execargv.push_back(parg);
	}
	execargv.push_back(NULL);
	ret = doexecv(&execargv[0]);
	
fail:
	while (i > 1)
	{
		--i;
		free(execargv[i]);
	}
	
	return ret;
}

#endif /* NFEXEC_SUPPORT */
