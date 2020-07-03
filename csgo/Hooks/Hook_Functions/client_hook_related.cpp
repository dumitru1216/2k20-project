#include "../Hooks.h"
#include "../../Features\Features.h"

void __stdcall hooking::frame_stage_notify( ClientFrameStage_t curStage )
{
	static auto oFrameStage = hooks.pClientHook.get_original<frame_stage_notify_t>( hook_index::frame_stage );

	g_Misc.ThirdPerson( curStage );

	g_Resolver.FrameStage( curStage );

	oFrameStage( curStage );
}
