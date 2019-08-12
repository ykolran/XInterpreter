// dllmain.h : Declaration of module class.

class CXInterpreterHandlersModule : public ATL::CAtlDllModuleT<CXInterpreterHandlersModule>
{
public :
	DECLARE_LIBID(LIBID_XInterpreterHandlersLib)
	DECLARE_REGISTRY_APPID_RESOURCEID(IDR_XINTERPRETERHANDLERS, "{208aa7f8-0375-43a9-90f3-ad28623ab640}")
};

extern class CXInterpreterHandlersModule _AtlModule;
