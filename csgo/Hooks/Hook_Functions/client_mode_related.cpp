#include "../Hooks.h"
#include "../../Features\Features.h"

bool __fastcall hooking::create_move( IClientMode* thisptr, void* edx, float sample_frametime, CUserCmd* pCmd ) {
	// Call original createmove before we start screwing with it
	static auto oCreateMove = hooks.pClientModeHook.get_original<create_move_t>( hook_index::create_move );
	oCreateMove( thisptr, edx, sample_frametime, pCmd );

	if ( !pCmd || !pCmd->command_number )
		return oCreateMove;

	// Get globals
	g::pCmd = pCmd;
	g::pLocalEntity = g_pEntityList->GetClientEntity( g_pEngine->GetLocalPlayer( ) );
	g::bSendPacket = true;
	if ( !g::pLocalEntity )
		return oCreateMove;

	uintptr_t* framePtr;
	__asm mov framePtr, ebp;

	g::OriginalView = g::pCmd->viewangles;

	//	HookPlayers();

	g_Misc.OnCreateMove( );
	g_Resolver.OnCreateMove( );

	engine_prediction::RunEnginePred( );
	g_AntiAim.OnCreateMove( );
	g_Aimbot.OnCreateMove( );
	g_Legitbot.OnCreateMove( );
	engine_prediction::EndEnginePred( );

	g_Misc.MovementFix( g::OriginalView );
	g_Math.Clamp( g::pCmd->viewangles );

	if ( g::bSendPacket )
		g::RealAngle = g::pCmd->viewangles;

	*( bool* )( *framePtr - 0x1C ) = g::bSendPacket;

	g::pCmd->buttons |= IN_BULLRUSH; // hehe

	return false;
}

void __fastcall hooking::overide_view( void* ecx, void* edx, CViewSetup* pSetup ) {
	static auto oOverrideView = hooks.pClientModeHook.get_original<overide_view_t>( hook_index::overide_view );

	if ( g_pEngine->IsConnected( ) && g_pEngine->IsInGame( ) ) {
		if ( !g::pLocalEntity )
			return;

		if ( !g::pLocalEntity->IsAlive( ) )
			return;

		if ( g_Menu.Config.NoRecoil ) {
			Vector viewPunch = g::pLocalEntity->GetViewPunchAngle( );
			Vector aimPunch = g::pLocalEntity->GetAimPunchAngle( );

			pSetup->angles[ 0 ] -= ( viewPunch[ 0 ] + ( aimPunch[ 0 ] * 2 * 0.4499999f ) );
			pSetup->angles[ 1 ] -= ( viewPunch[ 1 ] + ( aimPunch[ 1 ] * 2 * 0.4499999f ) );
			pSetup->angles[ 2 ] -= ( viewPunch[ 2 ] + ( aimPunch[ 2 ] * 2 * 0.4499999f ) );
		}

		if ( g_Menu.Config.Fov != 0 && !g::pLocalEntity->IsScoped( ) )
			pSetup->fov = g_Menu.Config.Fov;

		if ( g_Menu.Config.NoZoom && g::pLocalEntity->IsScoped( ) )
			pSetup->fov = ( g_Menu.Config.Fov == 0 ) ? 90 : g_Menu.Config.Fov;
	}

	oOverrideView( ecx, edx, pSetup );
}
