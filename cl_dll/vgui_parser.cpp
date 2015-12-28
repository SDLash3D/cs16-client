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

#include "unicode_strtools.h"

#define MAX_LOCALIZED_TITLES 512


struct locString
{
	char toLocalize[256];
	char localizedString[512];
};

locString gTitlesTXT[MAX_LOCALIZED_TITLES]; // for localized titles.txt strings
//locString gCstrikeMsgs[1024]; // for another
int giLastTitlesTXT;
const char* Localize( const char* string )
{
	StripEndNewlineFromString( (char*)string );

	for( int i = 0; i < giLastTitlesTXT; ++i )
	{
		if( !stricmp(gTitlesTXT[i].toLocalize, string))
			return gTitlesTXT[i].localizedString;
	}
	// nothing was found
	return string;
}

void Localize_Init(  )
{
	wchar_t *filename = L"cstrike/resource/cstrike_english.txt";

   FILE *wf = _wfopen (filename, L"rb");

   if (!wf)
   {
      gEngfuncs.Con_Printf ("Couldn't open file %s. Strings will not be localized!.\n", filename);
      return;
   }

   fseek (wf, 0L, SEEK_END);
   int length = ftell (wf);
   fseek (wf, 0L, SEEK_SET);

   wchar_t *unicode_buffer = new wchar_t[length];
   fread (unicode_buffer, 1, length, wf);

   fclose (wf);
   unicode_buffer++;

   int ansi_length = length / 2;

   char *ansi_buffer = new char[ansi_length];
   Q_UTF16ToUTF8 (unicode_buffer, ansi_buffer, ansi_length, STRINGCONVERT_ASSERT_REPLACE);

	char token[1024];
	giLastTitlesTXT = 0;

	while( true )
	{
      ansi_buffer = gEngfuncs.COM_ParseFile(ansi_buffer, token );

		if( !ansi_buffer) break;

		if( strstr(token, "TitlesTXT") )
		{
			if( giLastTitlesTXT > MAX_LOCALIZED_TITLES )
			{
				gEngfuncs.Con_Printf( "Too many localized titles.txt strings\n");
				break;
			}
			strcpy(gTitlesTXT[giLastTitlesTXT].toLocalize, &token[18]);
         ansi_buffer = gEngfuncs.COM_ParseFile(ansi_buffer, gTitlesTXT[giLastTitlesTXT].localizedString );

			if( !ansi_buffer) break;

			giLastTitlesTXT++;
		}
	}

   free (ansi_buffer);
 //  free (unicode_buffer);
  
}

void Localize_Free( )
{
	return;
}
