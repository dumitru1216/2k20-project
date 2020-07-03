#include "../Hooks.h"
#include "../../Features\Features.h"

void Event::FireGameEvent( IGameEvent* event )
{
	if ( !event )
		return;

	if ( !g::pLocalEntity )
		return;

	auto attacker = g_pEntityList->GetClientEntity( g_pEngine->GetPlayerForUserID( event->GetInt( "attacker" ) ) );
	if ( !attacker )
		return;

	if ( attacker != g::pLocalEntity )
		return;

	int index = g_pEngine->GetPlayerForUserID( event->GetInt( "userid" ) );

	PlayerInfo_t pInfo;
	g_pEngine->GetPlayerInfo( index, &pInfo );

	g::Hit[ index ] = true;

};
