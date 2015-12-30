/*
vgui_parser.cpp - implementation of VGUI *.res parser
Copyright (C) 2015 Uncle Mike, a1batross
This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.
This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.
*/

#include <string.h>
#include "wrect.h" // need for cl_dll.h
#include "cl_dll.h"
#include "vgui_parser.h"

#define MAX_LOCALIZED_TITLES 512

struct locString
{
	char toLocalize[256];
	char localizedString[512];
};

locString gTitlesTXT[MAX_LOCALIZED_TITLES]; // for localized titles.txt strings
														  //locString gCstrikeMsgs[1024]; // for another
int giLastTitlesTXT;
const char* Localize (const char* string)
{
	StripEndNewlineFromString ((char*)string);

	for (int i = 0; i < giLastTitlesTXT; ++i)
	{
		if (!stricmp (gTitlesTXT[i].toLocalize, string))
			return gTitlesTXT[i].localizedString;
	}
	// nothing was found
	return string;
}

void Localize_Init ()
{
	char *filename = "resource/cstrike_english.txt";
	char *pfile;
	char token[1024];
	giLastTitlesTXT = 0;

	char *afile = (char *)gEngfuncs.COM_LoadFile (filename, 5, NULL);

	pfile = afile;

	if (!pfile)
	{
		gEngfuncs.Con_Printf ("Couldn't open file %s. Strings will not be localized!.\n", filename);
		return;
	}

	while (true)
	{
		pfile = gEngfuncs.COM_ParseFile (pfile, token);

		if (!pfile) break;

		if (strstr (token, "TitlesTXT"))
		{
			if (giLastTitlesTXT > MAX_LOCALIZED_TITLES)
			{
				gEngfuncs.Con_Printf ("Too many localized titles.txt strings\n");
				break;
			}

			strcpy (gTitlesTXT[giLastTitlesTXT].toLocalize, token);
			pfile = gEngfuncs.COM_ParseFile (pfile, gTitlesTXT[giLastTitlesTXT].localizedString);

			if (!pfile) break;

			giLastTitlesTXT++;
		}
	}

	gEngfuncs.COM_FreeFile (afile);
}

void Localize_Free ()
{
	return;
}
Status API Training Shop Blog About Pricing
© 2015 GitHub, Inc.Terms Privacy Security Contact Help