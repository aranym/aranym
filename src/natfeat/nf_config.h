#ifndef _ARANYM_NFCONFIG_H
#define _ARANYM_NFCONFIG_H

#include "nf_base.h"

#include <string>
#include <map>

class NF_Config : public NF_Base
{
private:
	bool changed;

	typedef std::map<uint32_t,uint32_t> UserValues;
	UserValues values;

	bool ValidName(uint32_t name);

	static NF_Config *nf_config;

public:

	NF_Config();
	~NF_Config();
	const char *name() { return "NF_CONFIG"; }
	bool isSuperOnly() { return false; }
	int32 dispatch(uint32 fncode);
	void reset();
	
	bool IsChanged(void) { return changed; }
	void clear(void);
	bool SetValue(uint32_t name, uint32_t value);
	bool ListValue(uint32_t &name, uint32_t &value);
	
	static NF_Config *GetNFConfig();
};

#endif /* _ARANYM_NFCONFIG_H */
