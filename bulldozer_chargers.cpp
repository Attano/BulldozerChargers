/**
 * vim: set ts=4 sw=4 tw=99 noet :
 * =============================================================================
 * Bulldozer Chargers
 * Copyright (C) 2016 Ilya "Visor" Komarov
 * All rights reserved.
 * =============================================================================
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License, version 3.0, as published by the
 * Free Software Foundation.
 * 
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * As a special exception, the authors give you permission to link the
 * code of this program (as well as its derivative works) to "Half-Life 2," the
 * "Source Engine," the "SourcePawn JIT," and any Game MODs that run on software
 * by the Valve Corporation.  You must obey the GNU General Public License in
 * all respects for all other code used.  Additionally, AlliedModders LLC grants
 * this exception to all derivative works.  AlliedModders LLC defines further
 * exceptions, found in LICENSE.txt (as of this writing, version JULY-31-2007),
 * or <http://www.sourcemod.net/license.php>.
 */

#include <stdio.h>
#include "bulldozer_chargers.h"
#include "memutils.h"
#include "sourcehook.h"
#include "icvar.h"
#include "tier1/iconvar.h"
#include "tier1/convar.h"

BulldozerChargers g_BulldozerChargers;
IServerGameDLL *server = NULL;
ICvar* g_pCVar = NULL;

bool Patch();
void Unpatch();
void extension_enabled_changed(IConVar *var, const char *pOldValue, float flOldValue);
ConVar g_CvarEnabled("chestbump_patch_enabled", "0", FCVAR_CHEAT, "Enable or disable charger chestbump fix", true, 0.0, true, 1.0, extension_enabled_changed);

// We're looking into CTerrorGameMovement::HandleCustomCollision(Vector  const&, Vector  const&, CGameTrace *)
// Specifically the part where a cross product is compared against 14400f
// .text:004CB51A	movss   xmm1, ds:dword_BB7504	---> this is where our value is stored
// .text:004CB522	comiss  xmm1, xmm0
// .text:004CB525       jb      loc_4CB808
// Jump to dword_BB7504...
// 61 46 AC C5 27 B7 AC
const char c_sPattern[] = "\x61\x46\xAC\xC5\x27\xB7\xAC";

char *pPatchBaseAddr = NULL;

bool patched = false;

PLUGIN_EXPOSE(PDUncap, g_BulldozerChargers);

bool BulldozerChargers::Load(PluginId id, ISmmAPI *ismm, char *error, size_t maxlen, bool late)
{
	PLUGIN_SAVEVARS();

	GET_V_IFACE_ANY(GetServerFactory, server, IServerGameDLL, INTERFACEVERSION_SERVERGAMEDLL);
	GET_V_IFACE_ANY(GetEngineFactory, g_pCVar, ICvar, CVAR_INTERFACE_VERSION);

	if (!Patch())
	{
		Warning("Failed to patch charger chestbump section. Was there a game update?\n");
		return false;
	}

	ConVar_Register(0, this);

	return true;
}

bool Patch()
{
	if (patched)
		return true;
	
	// Find the address of the start of our signature
	char *pAddr = pPatchBaseAddr = (char*)g_MemUtils.FindLibPattern(server, c_sPattern, sizeof(c_sPattern)-1);
	if (pAddr == NULL)
	{
		return false;
	}
	DevMsg("Found Pattern at %08x\n", pAddr);

	if (!g_MemUtils.SetMemPatchable(pAddr, 2))
	{
		Warning("Failed to set mem patchable\n");
		return false;
	}

	if (g_CvarEnabled.GetBool())
	{
		memset((unsigned char *)pAddr, 0xC8, sizeof(uint8_t));
		memset((unsigned char *)(pAddr + 1), 0x42, sizeof(uint8_t));
		
		patched = true;
	}
	return true;
}

void Unpatch()
{
	if (!patched)
		return;
	
	char *pAddr = pPatchBaseAddr;

	memset((unsigned char *)pAddr, 0x61, sizeof(uint8_t));
	memset((unsigned char *)(pAddr + 1), 0x46, sizeof(uint8_t));
	
	patched = false;
}

bool BulldozerChargers::Unload(char *error, size_t maxlen)
{
	Unpatch();
	return true;
}

void extension_enabled_changed(IConVar *var, const char *pOldValue, float flOldValue)
{
	if (g_CvarEnabled.GetBool())
	{
		Patch();
	}
	else
	{
		Unpatch();
	}
}

const char *BulldozerChargers::GetLicense()
{
	return "GPLv3";
}

const char *BulldozerChargers::GetVersion()
{
	return "1.0";
}

const char *BulldozerChargers::GetDate()
{
	return __DATE__;
}

const char *BulldozerChargers::GetLogTag()
{
	return "CHESTBUMPFIX";
}

const char *BulldozerChargers::GetAuthor()
{
	return "Ilya \"Visor\" Komarov";
}

const char *BulldozerChargers::GetDescription()
{
	return "Gets rid of one of the most annoying bugs in the history of L4D2.";
}

const char *BulldozerChargers::GetName()
{
	return "Bulldozer Chargers";
}

const char *BulldozerChargers::GetURL()
{
	return "https://github.com/Attano/BulldozerChargers";
}

bool BulldozerChargers::RegisterConCommandBase(ConCommandBase *pVar)
{
	return META_REGCVAR(pVar);
}
