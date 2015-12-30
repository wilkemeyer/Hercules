/**
 * This file is part of Hercules.
 * http://herc.ws - http://github.com/HerculesWS/Hercules
 *
 * Copyright (C) 2012-2015  Hercules Dev Team
 * Copyright (C)  Athena Dev Teams
 *
 * Hercules is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#define HERCULES_CORE

#include "../config/core.h"
#include "../rgcore/rgcore.h"

#include "core.h"

#include "cbasetypes.h"
#include "console.h"
#include "db.h"
#include "memmgr.h"
#include "mmo.h"
#include "random.h"
#include "showmsg.h"
#include "strlib.h"
#include "sysinfo.h"
#include "nullpo.h"

using namespace rgCore;


#ifndef MINICORE
#	include "conf.h"
#	include "ers.h"
#	include "socket.h"
#	include "sql.h"
#	include "thread.h"
#	include "timer.h"
#	include "utils.h"
#endif

#ifndef _WIN32
#	include <unistd.h>
#else
#	include "winapi.h" // Console close event handling
#endif
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>

#if defined(_MSC_VER)
#pragma comment(lib, "..\\..\\build\\common.lib")
#pragma comment(lib, "..\\..\\build\\zlib.lib")
#pragma comment(lib, "..\\..\\build\\libconfig.lib")
#pragma comment(lib, "..\\..\\build\\sfmt.lib")
#pragma comment(lib, "..\\..\\build\\rgcore.lib")
#pragma comment(lib, "..\\..\\3rdparty\\mysql\\lib\\libmysql.lib")
#pragma comment(lib, "ws2_32.lib")
#endif

/// Called when a terminate signal is received.
void (*shutdown_callback)(void) = NULL;

struct core_interface core_s;
struct core_interface *core = &core_s;

#ifndef MINICORE // minimalist Core
// Added by Gabuzomeu
//
// This is an implementation of signal() using sigaction() for portability.
// (sigaction() is POSIX; signal() is not.)  Taken from Stevens' _Advanced
// Programming in the UNIX Environment_.
//
#ifdef WIN32 // windows don't have SIGPIPE
#define SIGPIPE SIGINT
#endif

#ifndef POSIX
#define compat_signal(signo, func) signal((signo), (func))
#else
sigfunc *compat_signal(int signo, sigfunc *func) {
	struct sigaction sact, oact;

	sact.sa_handler = func;
	sigemptyset(&sact.sa_mask);
	sact.sa_flags = 0;
#ifdef SA_INTERRUPT
	sact.sa_flags |= SA_INTERRUPT; /* SunOS */
#endif

	if (sigaction(signo, &sact, &oact) < 0)
		return (SIG_ERR);

	return (oact.sa_handler);
}
#endif

/*======================================
 * CORE : Console events for Windows
 *--------------------------------------*/
#ifdef _WIN32
static BOOL WINAPI console_handler(DWORD c_event) {
	switch(c_event) {
		case CTRL_CLOSE_EVENT:
		case CTRL_LOGOFF_EVENT:
		case CTRL_SHUTDOWN_EVENT:
			if( shutdown_callback != NULL )
				shutdown_callback();
			else
				core->runflag = CORE_ST_STOP;// auto-shutdown
			break;
		default:
			return FALSE;
	}
	return TRUE;
}

static void cevents_init(void) {
	if (SetConsoleCtrlHandler(console_handler,TRUE)==FALSE)
		ShowWarning ("Unable to install the console handler!\n");
}
#endif

/*======================================
 * CORE : Signal Sub Function
 *--------------------------------------*/
static void sig_proc(int sn) {
	static int is_called = 0;

	switch (sn) {
		case SIGINT:
		case SIGTERM:
			if (++is_called > 3)
				exit(EXIT_SUCCESS);
			if( shutdown_callback != NULL )
				shutdown_callback();
			else
				core->runflag = CORE_ST_STOP;// auto-shutdown
			break;
		case SIGSEGV:
		case SIGFPE:
			do_abort();
			// Pass the signal to the system's default handler
			compat_signal(sn, SIG_DFL);
			raise(sn);
			break;
	#ifndef _WIN32
		case SIGXFSZ:
			// ignore and allow it to set errno to EFBIG
			ShowWarning ("Max file size reached!\n");
			//run_flag = 0; // should we quit?
			break;
		case SIGPIPE:
			//ShowInfo ("Broken pipe found... closing socket\n"); // set to eof in socket.c
			break; // does nothing here
	#endif
	}
}

void signals_init (void) {
	compat_signal(SIGTERM, sig_proc);
	compat_signal(SIGINT, sig_proc);
#ifndef _DEBUG // need unhandled exceptions to debug on Windows
	compat_signal(SIGSEGV, sig_proc);
	compat_signal(SIGFPE, sig_proc);
#endif
#ifndef _WIN32
	compat_signal(SIGILL, SIG_DFL);
	compat_signal(SIGXFSZ, sig_proc);
	compat_signal(SIGPIPE, sig_proc);
	compat_signal(SIGBUS, SIG_DFL);
	compat_signal(SIGTRAP, SIG_DFL);
#endif
}
#endif


void request_shutdown(){
	
	if(shutdown_callback != NULL){
		shutdown_callback();
	
	}else{
		core->runflag = CORE_ST_STOP;// auto-shutdown

	}
}//end: request_shutdown()


/**
 * Warns the user if executed as superuser (root)
 */
void usercheck(void) {
	if (sysinfo->is_superuser()) {
		ShowWarning("You are running Hercules with root privileges, it is not necessary.\n");
	}
}

void core_defaults(void) {
	
	core->request_shutdown = request_shutdown;

	nullpo_defaults();
	sysinfo_defaults();
	console_defaults();
	strlib_defaults();
	cmdline_defaults();
#ifndef MINICORE
	libconfig_defaults();
	sql_defaults();
	timer_defaults();
	db_defaults();
	socket_defaults();
#endif
}
/**
 * Returns the source (core or plugin name) for the given command-line argument
 */
const char *cmdline_arg_source(struct CmdlineArgData *arg) {

	return "core";

}
/**
 * Defines a command line argument.
 *
 * @param pluginID  the source plugin ID (use HPM_PID_CORE if loading from the core).
 * @param name      the command line argument name, including the leading '--'.
 * @param shortname an optional short form (single-character, it will be prefixed with '-'). Use '\0' to skip.
 * @param func      the triggered function.
 * @param help      the help string to be displayed by '--help', if any.
 * @param options   options associated to the command-line argument. @see enum cmdline_options.
 * @return the success status.
 */
bool cmdline_arg_add(unsigned int pluginID, const char *name, char shortname, CmdlineExecFunc func, const char *help, unsigned int options) {
	struct CmdlineArgData *data = NULL;

	VECTOR_ENSURE(cmdline->args_data, 1, 1);
	VECTOR_PUSHZEROED(cmdline->args_data);
	data = &VECTOR_LAST(cmdline->args_data);
	data->pluginID = pluginID;
	data->name = aStrdup(name);
	data->shortname = shortname;
	data->func = func;
	data->help = aStrdup(help);
	data->options = options;

	return true;
}
/**
 * Help screen to be displayed by '--help'.
 */
static CMDLINEARG(help)
{
	int i;
	ShowInfo("Usage: %s [options]\n", SERVER_NAME);
	ShowInfo("\n");
	ShowInfo("Options:\n");

	for (i = 0; i < VECTOR_LENGTH(cmdline->args_data); i++) {
		struct CmdlineArgData *data = &VECTOR_INDEX(cmdline->args_data, i);
		char altname[16], paramnames[256];
		if (data->shortname) {
			snprintf(altname, sizeof(altname), " [-%c]", data->shortname);
		} else {
			*altname = '\0';
		}
		snprintf(paramnames, sizeof(paramnames), "%s%s%s", data->name, altname, (data->options&CMDLINE_OPT_PARAM) ? " <name>" : "");
		ShowInfo("  %-30s %s [%s]\n", paramnames, data->help ? data->help : "<no description provided>", cmdline->arg_source(data));
	}
	return false;
}
/**
 * Info screen to be displayed by '--version'.
 */
static CMDLINEARG(version)
{
	ShowInfo(CL_GREEN"Website/Forum:" CL_RESET "\thttp://herc.ws/\n");
	ShowInfo(CL_GREEN"IRC Channel:" CL_RESET "\tirc://irc.rizon.net/#Hercules\n");
	ShowInfo("Open " CL_WHITE "readme.txt" CL_RESET " for more information.\n");
	return false;
}
/**
 * Checks if there is a value available for the current argument
 *
 * @param name        the current argument's name.
 * @param current_arg the current argument's position.
 * @param argc        the program's argc.
 * @return true if a value for the current argument is available on the command line.
 */
bool cmdline_arg_next_value(const char *name, int current_arg, int argc)
{
	if (current_arg >= argc-1) {
		ShowError("Missing value for option '%s'.\n", name);
		return false;
	}

	return true;
}
/**
 * Executes the command line arguments handlers.
 *
 * @param argc    the program's argc
 * @param argv    the program's argv
 * @param options Execution options. Allowed values:
 * - CMDLINE_OPT_SILENT: Scans the argv for a command line argument that
 *   requires the server's silent mode, and triggers it. Invalid command line
 *   arguments don't cause it to abort. No command handlers are executed.
 * - CMDLINE_OPT_PREINIT: Scans the argv for command line arguments with the
 *   CMDLINE_OPT_PREINIT flag set and executes their handlers. Invalid command
 *   line arguments don't cause it to abort. Handler's failure causes the
 *   program to abort.
 * - CMDLINE_OPT_NORMAL: Scans the argv for normal command line arguments,
 *   skipping the pre-init ones, and executes their handlers. Invalid command
 *   line arguments or handler's failure cause the program to abort.
 * @return the amount of command line handlers successfully executed.
 */
int cmdline_exec(int argc, char **argv, unsigned int options)
{
	int count = 0, i;
	for (i = 1; i < argc; i++) {
		int j;
		struct CmdlineArgData *data = NULL;
		const char *arg = argv[i];
		if (arg[0] != '-') { // All arguments must begin with '-'
			ShowError("Invalid option '%s'.\n", argv[i]);
			exit(EXIT_FAILURE);
		}
		if (arg[1] != '-' && strlen(arg) == 2) {
			ARR_FIND(0, VECTOR_LENGTH(cmdline->args_data), j, VECTOR_INDEX(cmdline->args_data, j).shortname == arg[1]);
		} else {
			ARR_FIND(0, VECTOR_LENGTH(cmdline->args_data), j, strcmpi(VECTOR_INDEX(cmdline->args_data, j).name, arg) == 0);
		}
		if (j == VECTOR_LENGTH(cmdline->args_data)) {
			if (options&(CMDLINE_OPT_SILENT|CMDLINE_OPT_PREINIT))
				continue;
			ShowError("Unknown option '%s'.\n", arg);
			exit(EXIT_FAILURE);
		}
		data = &VECTOR_INDEX(cmdline->args_data, j);
		if (data->options&CMDLINE_OPT_PARAM) {
			if (!cmdline->arg_next_value(arg, i, argc))
				exit(EXIT_FAILURE);
			i++;
		}
		
		if ((data->options&CMDLINE_OPT_PREINIT) == (options&CMDLINE_OPT_PREINIT)) {
			const char *param = NULL;
			if (data->options&CMDLINE_OPT_PARAM) {
				param = argv[i]; // Already incremented above
			}
			if (!data->func(arg, param))
				exit(EXIT_SUCCESS);
			count++;
		}
	}
	return count;
}
/**
 * Defines the global command-line arguments.
 */
void cmdline_init(void)
{
	CMDLINEARG_DEF(help, 'h', "Displays this help screen", CMDLINE_OPT_NORMAL);
	CMDLINEARG_DEF(version, 'v', "Displays the server's version.", CMDLINE_OPT_NORMAL);

	cmdline_args_init_local();
}

void cmdline_final(void)
{
	while (VECTOR_LENGTH(cmdline->args_data) > 0) {
		struct CmdlineArgData *data = &VECTOR_POP(cmdline->args_data);
		aFree(data->name);
		aFree(data->help);
	}
	VECTOR_CLEAR(cmdline->args_data);
}

struct cmdline_interface cmdline_s;
struct cmdline_interface *cmdline;

void cmdline_defaults(void)
{
	cmdline = &cmdline_s;

	VECTOR_INIT(cmdline->args_data);

	cmdline->init = cmdline_init;
	cmdline->final = cmdline_final;
	cmdline->arg_add = cmdline_arg_add;
	cmdline->exec = cmdline_exec;
	cmdline->arg_next_value = cmdline_arg_next_value;
	cmdline->arg_source = cmdline_arg_source;
}



/****
 * Main Thread
 * 
 */
DWORD __stdcall serverMain(util::thread *_self){
	DWORD retval = EXIT_SUCCESS;

	putDbg("serverMain: Begin Initialization\n");

	cmdline->init();

	cmdline->exec(core->arg_c, core->arg_v, CMDLINE_OPT_SILENT);

	sysinfo->init();

	console->display_title();

	usercheck();

#ifdef MINICORE // minimalist Core
	do_init(core->arg_c, core->arg_v);
	do_final();
#else// not MINICORE

	Sql_Init();
	rathread_init();
	DB->init();
	signals_init();

#ifdef _WIN32
	cevents_init();
#endif

	timer->init();

	/* timer first */
	rnd_init();
	srand((unsigned int)timer->gettick());

	console->init();

	sockt->init();

	do_init(core->arg_c, core->arg_v);


	putDbg("serverMain: Initializaiton Finished, Entering MainLoop\n");

	// Main runtime cycle
	while(core->runflag != CORE_ST_STOP) {
		int next = timer->perform(timer->gettick_nocache());
		sockt->perform(next);
	}

	putDbg("serverMain: Begin Finalization\n");

	console->final();

	retval = (DWORD)do_final();
	timer->final();
	sockt->final();
	DB->final();
	rathread_final();
	ers_final();
#endif
	cmdline->final();
	sysinfo->final();

	putDbg("serverMain: Finished Finailization\n");

	rgCore_releaseIdleLoop();

	return retval;
}//end: serverMain()







/*======================================
 * CORE : MAINROUTINE 
 * Tasks Performed:
 *  - Pre Init
 *  - rgCore Init
 *  - UI handling
 *  - rgCore Final
 *  - post Final
 *--------------------------------------*/
int main (int argc, char **argv) {
	{// initialize program arguments
		char *p1 = SERVER_NAME = argv[0];
		char *p2 = p1;
		while ((p1 = strchr(p2, '/')) != NULL || (p1 = strchr(p2, '\\')) != NULL) {
			SERVER_NAME = ++p1;
			p2 = p1;
		}
		core->arg_c = argc;
		core->arg_v = argv;
		core->runflag = CORE_ST_RUN;
	}
	core_defaults();
	
	set_server_type();
	
	{
		//
		// Initialize rgCore
		//
		char *appName;
		if(SERVER_TYPE == SERVER_TYPE_MAP){
			appName = "MapServer";
		}else if(SERVER_TYPE == SERVER_TYPE_CHAR){
			appName = "CharServer";
		}else if(SERVER_TYPE == SERVER_TYPE_LOGIN){
			appName = "LoginServer";
		}else{
			appName = "Unknown";
		}

		rgCore_init(appName);
	}


	// 
	// Create serverMain Thread (old athena main)
	//
	util::thread *hServerMain = new util::thread("Athena Server Main", serverMain);



	//
	// Process rgCore MainLoop:
	//
	rgCore_idleLoop();


	putLog("Finalization -> Waiting for ServerMain Thread Finalization!\n");
	hServerMain->wait();

	// The ServerMain Thread sets athena-exit-code as exitParam
	int retval = (int)hServerMain->getExitParam();

	// Free up thread ressources
	delete hServerMain;
	hServerMain = NULL;



	//
	// rgCore Final:
	//
	rgCore_final();
	

	return retval;
}//end: main()
