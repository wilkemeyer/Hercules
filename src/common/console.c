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

#include "../config/core.h" // CONSOLE_INPUT, MAX_CONSOLE_INPUT
#include "console.h"

#include "cbasetypes.h"
#include "core.h"
#include "nullpo.h"
#include "showmsg.h"
#include "sysinfo.h"



/*======================================
 * CORE : Display title
 *--------------------------------------*/
void console_display_title(void) {
	const char *vcstype = sysinfo->vcstype();

	ShowMessage("\n");
	ShowMessage(""CL_BG_RED""CL_BT_WHITE"                                                                      "CL_CLL""CL_NORMAL"\n");
	ShowMessage(""CL_BG_RED""CL_BT_WHITE"                 Hercules Development Team presents                   "CL_CLL""CL_NORMAL"\n");
	ShowMessage(""CL_BG_RED""CL_BT_WHITE"                _   _                     _                           "CL_CLL""CL_NORMAL"\n");
	ShowMessage(""CL_BG_RED""CL_BT_WHITE"               | | | |                   | |                          "CL_CLL""CL_NORMAL"\n");
	ShowMessage(""CL_BG_RED""CL_BT_WHITE"               | |_| | ___ _ __ ___ _   _| | ___  ___                 "CL_CLL""CL_NORMAL"\n");
	ShowMessage(""CL_BG_RED""CL_BT_WHITE"               |  _  |/ _ \\ '__/ __| | | | |/ _ \\/ __|                "CL_CLL""CL_NORMAL"\n");
	ShowMessage(""CL_BG_RED""CL_BT_WHITE"               | | | |  __/ | | (__| |_| | |  __/\\__ \\                "CL_CLL""CL_NORMAL"\n");
	ShowMessage(""CL_BG_RED""CL_BT_WHITE"               \\_| |_/\\___|_|  \\___|\\__,_|_|\\___||___/                "CL_CLL""CL_NORMAL"\n");
	ShowMessage(""CL_BG_RED""CL_BT_WHITE"                                                                      "CL_CLL""CL_NORMAL"\n");
	ShowMessage(""CL_BG_RED""CL_BT_WHITE"                      http://herc.ws/board/                           "CL_CLL""CL_NORMAL"\n");
	ShowMessage(""CL_BG_RED""CL_BT_WHITE"                                                                      "CL_CLL""CL_NORMAL"\n");

	ShowInfo("Hercules %d-bit for %s\n", sysinfo->is64bit() ? 64 : 32, sysinfo->platform());
	ShowInfo("%s revision (src): '"CL_WHITE"%s"CL_RESET"'\n", vcstype, sysinfo->vcsrevision_src());
	ShowInfo("%s revision (scripts): '"CL_WHITE"%s"CL_RESET"'\n", vcstype, sysinfo->vcsrevision_scripts());
	ShowInfo("OS version: '"CL_WHITE"%s"CL_RESET" [%s]'\n", sysinfo->osversion(), sysinfo->arch());
	ShowInfo("CPU: '"CL_WHITE"%s [%d]"CL_RESET"'\n", sysinfo->cpu(), sysinfo->cpucores());
	ShowInfo("Compiled with %s\n", sysinfo->compiler());
	ShowInfo("Compile Flags: %s\n", sysinfo->cflags());
}

/**
 * Shows a license notice as per GNU GPL recommendation.
 */
void console_display_gplnotice(void)
{
	ShowInfo("Hercules, Copyright (C) 2012-2015, Hercules Dev Team and others.\n");
	ShowInfo("Licensed under the GNU General Public License, version 3 or later.\n");
}
