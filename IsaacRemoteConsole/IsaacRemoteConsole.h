// The following ifdef block is the standard way of creating macros which make exporting 
// from a DLL simpler. All files within this DLL are compiled with the ISAACREMOTECONSOLE_EXPORTS
// symbol defined on the command line. This symbol should not be defined on any project
// that uses this DLL. This way any other project whose source files include this file see 
// ISAACREMOTECONSOLE_API functions as being imported from a DLL, whereas this DLL sees symbols
// defined with this macro as being exported.
#ifdef ISAACREMOTECONSOLE_EXPORTS
#define ISAACREMOTECONSOLE_API __declspec(dllexport)
#else
#define ISAACREMOTECONSOLE_API __declspec(dllimport)
#endif

// This class is exported from the IsaacRemoteConsole.dll
class ISAACREMOTECONSOLE_API CIsaacRemoteConsole {
public:
	CIsaacRemoteConsole(void);
	// TODO: add your methods here.
};

extern ISAACREMOTECONSOLE_API int nIsaacRemoteConsole;

ISAACREMOTECONSOLE_API int fnIsaacRemoteConsole(void);
