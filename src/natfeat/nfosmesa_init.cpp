#include "nfosmesa_impl.h"

/*
 * compiled separately, because gcc blows up when generating debug info for this function
 */
#ifdef NFOSMESA_SUPPORT

void OSMesaDriver::InitPointersGL(void *handle)
{
	D(bug("nfosmesa: InitPointersGL()"));

#define GL_PROC(type, gl, name, export, upper, proto, args, first, ret) \
	fn.gl ## name = (type (APIENTRY *) proto) SDL_LoadFunction(handle, "gl" #name); \
	if (fn.gl ## name == 0 && get_procaddress) \
		fn.gl ## name = (type (APIENTRY *) proto) get_procaddress("gl" #name);
#define GLU_PROC(type, gl, name, export, upper, proto, args, first, ret)
#define OSMESA_PROC(type, gl, name, export, upper, proto, args, first, ret)
#include "../../atari/nfosmesa/glfuncs.h"
}

#endif /* NFOSMESA_SUPPORT */
