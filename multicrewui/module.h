/* Microsoft Flight Simulator 2004 Module Example.
 *
 * Copyright (c) 2004, Cyril Hruscak.
 * You may freely redistribute and/or modify this code provided this copyright
 * notice remains unchanged. Usage of the code is at your own risk. I accept
 * no liability for any possible damage using this code.
 *
 * It shows how to create a module dll ("Modules" directory of the main
 * FS directory) that can be loaded by the FS2004. It also shows how to add
 * a new menu entry to the main flight simulator menu.
 */

#ifndef	__FS_MODULE_H__
#define	__FS_MODULE_H__

#define	UI_DLLEXPORT	__declspec(dllexport)
#define	FSAPI	__stdcall

/**
 * This is the module's import table definition.
 */
typedef struct _MODULE_IMPORT {
	struct {
		int fnID;
		PVOID fnptr;
	} IMPORTSentry;
	struct {
		int fnID;
		PVOID fnptr;
	} nullentry;
} MODULE_IMPORT;

/**
 * This is the module's export table definition
 */
typedef struct _MODULE_LINKAGE {
	int ModuleID;
	void (FSAPI *ModuleInit)(void);
	void (FSAPI *ModuleDeinit)(void);
	UINT32 ModuleFlags;
	UINT32 ModulePriority;
	UINT32 ModuleVersion;
	PVOID ModuleTable;
} MODULE_LINKAGE;

#endif	/* __FS_MODULE_H__ */
