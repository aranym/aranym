#include "nf_objs.h"

NF_Name nf_name;
NF_Version nf_version;
XHDIDriver Xhdi;

pNatFeat nf_objects[] = {&nf_name, &nf_version, &Xhdi};
unsigned int nf_objs_cnt = sizeof(nf_objects) / sizeof(nf_objects[0]);
