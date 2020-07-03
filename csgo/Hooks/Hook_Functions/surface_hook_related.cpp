#include "../Hooks.h"
#include "../../Features\Features.h"

void __fastcall hooking::lock_cursor( ISurface* thisptr, void* edx ) {
	static auto oLockCursor = hooks.pSurfaceHook.get_original<lock_cursor_t>( hook_index::lock_cursor );

	if ( !g_Menu.menuOpened )
		return oLockCursor( thisptr, edx );

	g_pSurface->UnLockCursor( );
}