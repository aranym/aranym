#ifndef _ARANYM_HOSTEXEC_H
#define _ARANYM_HOSTEXEC_H

#include "nf_base.h"

#include <string>

class HostExec : public NF_Base
{
public:
	const char *name() { return "HOSTEXEC"; }
	bool isSuperOnly() { return false; }
	int32 dispatch(uint32 fncode);
	void reset();

private:
	std::string str;

	std::string getc(uint8 c) const;
	void exec(const std::string& path) const;
	int execv(int argc, memptr argv) const;
	int doexecv(char *const argv[]) const;
	std::string trim(const std::string& str) const;
	std::string translatePath(const std::string& path) const;
};

#endif /* _ARANYM_HOSTEXEC_H */
