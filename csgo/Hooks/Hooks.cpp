#include <thread>
#include "Hooks.h"
#include "../Utils\Utils.h"
#include "../Features\Features.h"
#include "../Menu\Menu.h"
#include "../SDK\Hitboxes.h"
#include "libloaderapi.h"


Misc     g_Misc;
hooking hooks;
Event g_Event;

void hooking::initialize( ) {
	// Get window handle
	while ( !( hooks.hCSGOWindow = FindWindowA( "Valve001", nullptr ) ) ) {
		using namespace std::literals::chrono_literals;
		std::this_thread::sleep_for( 50ms );
	}

	interfaces::Init( );                         // Get interfaces
	g_pNetvars = std::make_unique<NetvarTree>( );// Get netvars after getting interfaces as we use them

	if ( hooks.hCSGOWindow )        // Hook WNDProc to capture mouse / keyboard input
		hooks.pOriginalWNDProc = reinterpret_cast< WNDPROC >( SetWindowLongPtr( hooks.hCSGOWindow, GWLP_WNDPROC,
			reinterpret_cast< LONG_PTR >( hooks.WndProc ) ) );

	hooks.pClientHook.setup( g_pClientDll );
	hooks.pClientModeHook.setup( g_pClientMode );
	hooks.pSurfaceHook.setup( g_pSurface );
	hooks.pPanelHook.setup( g_pPanel );
	hooks.pModelHook.setup( g_pModelRender );
	hooks.pRenderViewHook.setup( g_pRenderView );

	hooks.pClientHook.hook_index( hook_index::frame_stage, hooking::frame_stage_notify  );
	hooks.pClientModeHook.hook_index( hook_index::create_move, hooking::create_move );
	hooks.pClientModeHook.hook_index( hook_index::overide_view, hooking::overide_view );
	hooks.pSurfaceHook.hook_index( hook_index::lock_cursor, hooking::lock_cursor );
	hooks.pPanelHook.hook_index( hook_index::paint_traverse, hooking::PaintTraverse );
	hooks.pModelHook.hook_index( hook_index::draw_model_execute, hooking::DrawModelExecute );
	hooks.pRenderViewHook.hook_index( hook_index::scene_end, hooking::SceneEnd );

	g_Event.Init( );

	g::CourierNew = g_pSurface->FontCreate( );
	g_pSurface->SetFontGlyphSet( g::CourierNew, "Courier New", 14, 300, 0, 0, FONTFLAG_OUTLINE );
	g::Tahoma = g_pSurface->FontCreate( );
	g_pSurface->SetFontGlyphSet( g::Tahoma, "Tahoma", 12, 700, 0, 0, FONTFLAG_DROPSHADOW );


	Utils::Log( "Hooking completed!" );
}

void hooking::restore( ) {
	
	/* unhook indexes */
	hooks.pClientHook.unhook_index( hook_index::frame_stage );
	hooks.pClientModeHook.unhook_index( hook_index::create_move );
	hooks.pClientModeHook.unhook_index( hook_index::overide_view );
	hooks.pSurfaceHook.unhook_index( hook_index::lock_cursor );
	hooks.pPanelHook.unhook_index( hook_index::paint_traverse );
	hooks.pModelHook.unhook_index( hook_index::draw_model_execute );
	hooks.pRenderViewHook.unhook_index( hook_index::scene_end );

	/* reset csgo window */
	SetWindowLongPtr( hooks.hCSGOWindow, GWLP_WNDPROC, reinterpret_cast< LONG_PTR >( hooks.pOriginalWNDProc ) );

	/* reset netvars */
	g_pNetvars.reset( );  
	
	/* unhook functions end */
	Utils::Log( "Unhooking succeded!" );
}

void hooking::hook_players( ) {
	static bool Init[ 65 ];
	static bool Hooked[ 65 ];

	for ( int i = 1; i < g_pEngine->GetMaxClients( ); ++i ) {
		C_BaseEntity* pPlayerEntity = g_pEntityList->GetClientEntity( i );

		if ( !pPlayerEntity || !pPlayerEntity->IsAlive( ) || pPlayerEntity->IsDormant( ) ) {
			if ( Hooked[ i ] )
				hooks.pPlayerHook[ i ]->Unhook( hook_index::bone_processing );

			Hooked[ i ] = false;
			continue;
		}

		if ( !Init[ i ] ) {
			hooks.pPlayerHook[ i ] = std::make_unique<ShadowVTManager>( );
			Init[ i ] = true;
		}

		if ( Hooked[ i ] )
			hooks.pPlayerHook[ i ]->Unhook( hook_index::bone_processing );

		if ( !Hooked[ i ] ) {
			hooks.pPlayerHook[ i ]->Setup( pPlayerEntity );
			hooks.pPlayerHook[ i ]->Hook( hook_index::bone_processing, hooking::DoExtraBonesProcessing );

			Hooked[ i ] = true;
		}
	}
}

void __fastcall hooking::SceneEnd( void* ecx, void* edx )
{
	static auto oSceneEnd = hooks.pRenderViewHook.get_original<SceneEnd_t>( hook_index::scene_end );
	oSceneEnd( ecx, edx );

	
}

void __fastcall hooking::DoExtraBonesProcessing( void* ECX, void* EDX, void* unkn1, void* unkn2, void* unkn3, void* unkn4, CBoneBitList& unkn5, void* unkn6 )
{
	C_BaseEntity* pPlayerEntity = ( C_BaseEntity* )ECX;

	if ( !pPlayerEntity || pPlayerEntity == nullptr )
		return;

	if ( !pPlayerEntity->IsAlive( ) || pPlayerEntity->IsDormant( ) )
		return;

	if ( !pPlayerEntity->AnimState( ) )
		return;

	auto oDoExtraBonesProcessing = hooks.pPlayerHook[ pPlayerEntity->EntIndex( ) ]->GetOriginal<ExtraBoneProcess_t>( hook_index::bone_processing );

	float Backup = pPlayerEntity->AnimState( )->m_flUnknownFraction;
	pPlayerEntity->AnimState( )->m_flUnknownFraction = 0;

	oDoExtraBonesProcessing( ECX, unkn1, unkn2, unkn3, unkn4, unkn5, unkn6 );

	pPlayerEntity->AnimState( )->m_flUnknownFraction = Backup;
}

void __fastcall hooking::DrawModelExecute( void* ecx, void* edx, IMatRenderContext* context, const DrawModelState_t& state, const ModelRenderInfo_t& info, matrix3x4_t* matrix )
{
	static auto oDrawModelExecute = hooks.pModelHook.get_original<DrawModelExecute_t>( hook_index::draw_model_execute );

	const char* ModelName = g_pModelInfo->GetModelName( ( model_t* )info.pModel );

	C_BaseEntity* pPlayerEntity = g_pEntityList->GetClientEntity( info.index );



	oDrawModelExecute( ecx, context, state, info, matrix );
}

C_BaseEntity* UTIL_PlayerByIndex( int index )
{
	typedef C_BaseEntity* ( __fastcall* PlayerByIndex )( int );
	static PlayerByIndex UTIL_PlayerByIndex = ( PlayerByIndex )Utils::FindSignature( "server.dll", "85 C9 7E 2A A1" );

	if ( !UTIL_PlayerByIndex )
		return false;

	return UTIL_PlayerByIndex( index );
}

void __fastcall hooking::PaintTraverse( PVOID pPanels, int edx, unsigned int vguiPanel, bool forceRepaint, bool allowForce )
{
	static auto oPaintTraverse = hooks.pPanelHook.get_original<PaintTraverse_t>( hook_index::paint_traverse );
	static unsigned int panelID, panelHudID;

	if ( !panelHudID )
		if ( !strcmp( "HudZoom", g_pPanel->GetName( vguiPanel ) ) )
		{
			panelHudID = vguiPanel;
		}

	if ( panelHudID == vguiPanel && g::pLocalEntity && g::pLocalEntity->IsAlive( ) && g_Menu.Config.NoScope )
	{
		if ( g::pLocalEntity->IsScoped( ) )
			return;
	}

	oPaintTraverse( pPanels, vguiPanel, forceRepaint, allowForce );

	if ( !panelID )
		if ( !strcmp( "MatSystemTopPanel", g_pPanel->GetName( vguiPanel ) ) )
		{
			panelID = vguiPanel;
			hooks.bInitializedDrawManager = true;
		}

	if ( panelID == vguiPanel )
	{
		g_ESP.Render( );
		g_Misc.Crosshair( );
		g_Menu.Render( );

		if ( false ) // server hitboxes
		{
			static uintptr_t pCall = ( uintptr_t )Utils::FindSignature( "server.dll", "55 8B EC 81 EC ? ? ? ? 53 56 8B 35 ? ? ? ? 8B D9 57 8B CE" );

			float fDuration = -1.f;

			PVOID pEntity = nullptr;
			pEntity = UTIL_PlayerByIndex( g::pLocalEntity->EntIndex( ) );

			if ( pEntity )
			{
				__asm
				{
					pushad
					movss xmm1, fDuration
					push 0 //bool monoColor
					mov ecx, pEntity
					call pCall
					popad
				}
			}
		}
	}
}

LRESULT hooking::WndProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
	// for now as a lambda, to be transfered somewhere
	// Thanks uc/WasserEsser for pointing out my mistake!
	// Working when you HOLD th button, not when you press it.
	const auto getButtonHeld = [ uMsg, wParam ]( bool& bButton, int vKey )
	{
		if ( wParam != vKey ) return;

		if ( uMsg == WM_KEYDOWN )
			bButton = true;
		else if ( uMsg == WM_KEYUP )
			bButton = false;
	};

	const auto getButtonToggle = [ uMsg, wParam ]( bool& bButton, int vKey )
	{
		if ( wParam != vKey ) return;

		if ( uMsg == WM_KEYUP )
			bButton = !bButton;
	};

	if ( hooks.bInitializedDrawManager )
	{
		// our wndproc capture fn
		if ( g_Menu.menuOpened )
		{
			return true;
		}
	}


	return CallWindowProcA( hooks.pOriginalWNDProc, hWnd, uMsg, wParam, lParam );
}
