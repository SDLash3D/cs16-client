#include "events.h"

enum ak47_e
{
	AK47_IDLE1 = 0,
	AK47_RELOAD,
	AK47_DRAW,
	AK47_SHOOT1,
	AK47_SHOOT2,
	AK47_SHOOT3
};

void EV_FireAK47( event_args_t *args )
{
	vec3_t origin, angles, velocity;

	vec3_t ShellVelocity, ShellOrigin;
	vec3_t vecSrc, vecAiming;
	vec3_t up, right, forward;
	float flSpread = 0.01;
	int shell, idx;

	idx = args->entindex;
	VectorCopy( args->origin, origin );
	VectorCopy( args->angles, angles );
	VectorCopy( args->velocity, velocity );

	AngleVectors( angles, forward, right, up );

	if ( EV_IsLocal( args->entindex ) )
	{
		++g_iShotsFired;
		gEngfuncs.pEventAPI->EV_WeaponAnimation(gEngfuncs.pfnRandomLong(AK47_SHOOT1, AK47_SHOOT3), 2);
		EV_MuzzleFlash();
	}

	shell = gEngfuncs.pEventAPI->EV_FindModelIndex ("models/rshell.mdl");
	EV_GetDefaultShellInfo( args, origin, velocity, ShellVelocity, ShellOrigin, forward, -right, up, 12, -10, -7 );
	EV_EjectBrass(ShellOrigin, ShellVelocity, angles[ YAW ], shell, TE_BOUNCE_SHELL);

	gEngfuncs.pEventAPI->EV_PlaySound( idx, origin, CHAN_WEAPON,
		gEngfuncs.pfnRandomLong(0, 1) ? "weapons/ak47-1.wav" : "weapons/ak47-2.wav",
		1, ATTN_NORM, 0,
		94 + gEngfuncs.pfnRandomLong( 0, 0xf ) );


	EV_GetGunPosition( args, vecSrc, origin );
	VectorCopy( forward, vecAiming );
	EV_HLDM_FireBullets( idx,
		forward, right,	up,
		1, vecSrc, vecAiming,
		8192, 0, 0, 0,
		args->fparam1, args->fparam2 );
}
