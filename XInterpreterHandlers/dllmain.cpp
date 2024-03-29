// dllmain.cpp : Implementation of DllMain.

#include "stdafx.h"
#include "resource.h"
#include "XInterpreterHandlers_i.h"
#include "dllmain.h"
#include "xdlldata.h"

CXInterpreterHandlersModule _AtlModule;

class CXInterpreterHandlersApp : public CWinApp
{
public:

// Overrides
	virtual BOOL InitInstance();
	virtual int ExitInstance();

	DECLARE_MESSAGE_MAP()
};

BEGIN_MESSAGE_MAP(CXInterpreterHandlersApp, CWinApp)
END_MESSAGE_MAP()

CXInterpreterHandlersApp theApp;

BOOL CXInterpreterHandlersApp::InitInstance()
{
	if (!PrxDllMain(m_hInstance, DLL_PROCESS_ATTACH, nullptr))
		return FALSE;
	return CWinApp::InitInstance();
}

int CXInterpreterHandlersApp::ExitInstance()
{
	return CWinApp::ExitInstance();
}
