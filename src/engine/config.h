
#ifndef ENGINE_CONFIG_H
#define ENGINE_CONFIG_H

#include "kernel.h"

typedef void (*ConfigVariableFuncInt)(char *pName, char *pScriptName, int &Value, char *pDefault, char *pMin, char *pMax, char *pFlags, char *pDesc, void *pData);
typedef void (*ConfigVariableFuncStr)(char *pName, char *pScriptName, char *pValue, char *pLength, char *pDefault, char *pFlags, char *pDesc, void *pData);

class IConfig : public IInterface
{
	MACRO_INTERFACE("config", 0)
public:
	typedef void (*SAVECALLBACKFUNC)(IConfig *pConfig, void *pUserData);

	virtual void Init() = 0;
	virtual void Reset() = 0;
	virtual void RestoreStrings() = 0;
	virtual void Save() = 0;
	virtual void GetAllConfigVariables(ConfigVariableFuncInt IntFunc, ConfigVariableFuncStr StrFunc, void *pData) = 0;

	virtual void RegisterCallback(SAVECALLBACKFUNC pfnFunc, void *pUserData) = 0;

	virtual void WriteLine(const char *pLine) = 0;
};

extern IConfig *CreateConfig();

#endif
