#include "hud.h"
#include "usercmd.h"
#include "cvardef.h"
#include "kbutton.h"
cvar_t		*sensitivity;
cvar_t		*in_joystick;
void IN_MouseEvent( int mstate )
{
}

// Stubs

extern "C" void IN_ClearStates ( void )
{
	//gEngfuncs.Con_Printf("IN_ClearStates\n");
}

extern "C" void IN_ActivateMouse ( void )
{
	//gEngfuncs.Con_Printf("IN_ActivateMouse\n");
}

extern "C" void IN_DeactivateMouse ( void )
{
	//gEngfuncs.Con_Printf("IN_DeactivateMouse\n");
}

extern "C" void IN_MouseEvent ( void )
{
	//gEngfuncs.Con_Printf("IN_DeactivateMouse\n");
}

extern "C" void IN_Accumulate ( void )
{
	//gEngfuncs.Con_Printf("IN_Accumulate\n");
}

void IN_Commands ( void )
{
	//gEngfuncs.Con_Printf("IN_Commands\n");
}

void IN_Shutdown ( void )
{
}

// Register cvars and reset data
void IN_Init( void )
{
	sensitivity = gEngfuncs.pfnRegisterVariable ( "sensitivity", "3", FCVAR_ARCHIVE );
	in_joystick = gEngfuncs.pfnRegisterVariable ( "joystick", "0", FCVAR_ARCHIVE );
}

void IN_Move( float frametime, usercmd_t *cmd )
{
}
