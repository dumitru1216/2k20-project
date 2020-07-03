#pragma once

#include "../Utils\Interfaces.h"
#include "../SDK\IBaseClientDll.h"
#include "../SDK\IClientMode.h"
#include "../SDK\ISurface.h"
#include "../SDK\IGameEvent.h"
#include "../SDK\CInput.h"
#include "../SDK\CModelRender.h"
#include "../SDK\IVModelInfo.h"
#include "../SDK\IMaterial.h"
#include "../SDK\IVRenderView.h"
#include <assert.h>

#include "Hook_Method/vfunc_hook.hpp"

namespace hook_index {
	constexpr auto overide_view = 18;
	constexpr auto paint_traverse = 41; /* im dont think i need that but ok */
	constexpr auto create_move = 24;
	constexpr auto lock_cursor = 67;
	constexpr auto frame_stage = 37;
	constexpr auto draw_model_execute = 21;
	constexpr auto bone_processing = 197;
	constexpr auto scene_end = 9;
}

class Event : public IGameEventListener {
public:
	void FireGameEvent(IGameEvent *event);
	int  GetEventDebugID = 42;
	void Init()
	{
		g_pEventManager->AddListener(this, "player_hurt", false);
	}
};

extern Event g_Event;
class IMatRenderContext;
class CBoneBitList;
class ShadowVTManager;
class hooking {
public:
    // Initialization setup, called on injection
    static void initialize();
    static void restore();
	static void hook_players();

	static void     __stdcall   frame_stage_notify(ClientFrameStage_t curStage);
    static bool     __fastcall  create_move(IClientMode*, void*, float, CUserCmd*);
    static void     __fastcall  lock_cursor(ISurface*, void*);
	static void		__fastcall  PaintTraverse(PVOID pPanels, int edx, unsigned int vguiPanel, bool forceRepaint, bool allowForce);
	static void     __fastcall  overide_view(void* ecx, void* edx, CViewSetup* pSetup);
	static void     __fastcall  DrawModelExecute(void* ecx, void* edx, IMatRenderContext* context, const DrawModelState_t& state, const ModelRenderInfo_t& render_info, matrix3x4_t* matrix);
	static void     __fastcall  DoExtraBonesProcessing(void * ECX, void * EDX, void * unkn1, void * unkn2, void * unkn3, void * unkn4, CBoneBitList & unkn5, void * unkn6);
	static void     __fastcall  SceneEnd(void *ecx, void *edx);

    static LRESULT  __stdcall   WndProc   (HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

private:

	vfunc_hook pClientHook;
	vfunc_hook pClientModeHook;
	vfunc_hook pSurfaceHook;
	vfunc_hook pPanelHook;
	vfunc_hook pModelHook;
	vfunc_hook pRenderViewHook;
	std::unique_ptr<ShadowVTManager> pPlayerHook[65];

    /*---------------------------------------------*/
    /*-------------Hook prototypes-----------------*/
    /*---------------------------------------------*/

	typedef void (__stdcall*  frame_stage_notify_t ) (ClientFrameStage_t);
    typedef bool (__fastcall* create_move_t ) (IClientMode*, void*, float, CUserCmd*);
    typedef void (__fastcall* lock_cursor_t ) (ISurface*, void*);
	typedef void (__thiscall* PaintTraverse_t) (PVOID, unsigned int, bool, bool);
	typedef void (__fastcall* overide_view_t ) (void*, void*, CViewSetup*);
	typedef void (__thiscall* DrawModelExecute_t) (void*, IMatRenderContext*, const DrawModelState_t&, const ModelRenderInfo_t&, matrix3x4_t*);
	typedef void (__thiscall* ExtraBoneProcess_t) (void*, void*, void*, void*, void*, CBoneBitList&, void*);
	typedef void (__fastcall* SceneEnd_t) (void*, void*);

private:
    HWND                           hCSGOWindow             = nullptr; // CSGO window handle
    bool                           bInitializedDrawManager = false;   // Check if we initialized our draw manager
    WNDPROC                        pOriginalWNDProc        = nullptr; // Original CSGO window proc

	template<class Type> // GLAD
	Type HookManual(uintptr_t *instance, int offset, Type hook)
{
		DWORD Dummy;
		Type fnOld = (Type)instance[offset];
		VirtualProtect((void*)(instance + offset * 0x4), 0x4, PAGE_EXECUTE_READWRITE, &Dummy);
		instance[offset] = (uintptr_t)hook;
		VirtualProtect((void*)(instance + offset * 0x4), 0x4, Dummy, &Dummy);
		return fnOld;
	}
};
extern hooking hooks;

class ProtectGuard
{
public:

	ProtectGuard(void *base, uint32_t len, uint32_t protect)
	{
		this->base = base;
		this->len = len;

		if (!VirtualProtect(base, len, protect, (PDWORD)&old_protect))
			throw std::runtime_error("Failed to protect region!");
	}

	~ProtectGuard()
	{
		VirtualProtect(base, len, old_protect, (PDWORD)&old_protect);
	}

private:

	void *base;
	uint32_t len;
	uint32_t old_protect;
};

class ShadowVTManager // GLAD
{

public:

	ShadowVTManager() : class_base(nullptr), method_count(0), shadow_vtable(nullptr), original_vtable(nullptr) {}
	ShadowVTManager(void *base) : class_base(base), method_count(0), shadow_vtable(nullptr), original_vtable(nullptr) {}
	~ShadowVTManager()
	{
		RestoreTable();

		delete[] shadow_vtable;
	}

	inline void Setup(void *base = nullptr)
	{
		if (base != nullptr)
			class_base = base;

		if (class_base == nullptr)
			return;

		original_vtable = *(uintptr_t**)class_base;
		method_count = GetMethodCount(original_vtable);

		if (method_count == 0)
			return;

		shadow_vtable = new uintptr_t[method_count + 1]();

		shadow_vtable[0] = original_vtable[-1];
		std::memcpy(&shadow_vtable[1], original_vtable, method_count * sizeof(uintptr_t));

		try
		{
			auto guard = ProtectGuard{ class_base, sizeof(uintptr_t), PAGE_READWRITE };
			*(uintptr_t**)class_base = &shadow_vtable[1];
		}
		catch (...)
		{
			delete[] shadow_vtable;
		}
	}

	template<typename T>
	inline void Hook(uint32_t index, T method)
	{
		assert(index < method_count);
		shadow_vtable[index + 1] = reinterpret_cast<uintptr_t>(method);
	}

	inline void Unhook(uint32_t index)
	{
		assert(index < method_count);
		shadow_vtable[index + 1] = original_vtable[index];
	}

	template<typename T>
	inline T GetOriginal(uint32_t index)
	{
		return (T)original_vtable[index];
	}

	inline void RestoreTable()
	{
		try
		{
			if (original_vtable != nullptr)
			{
				auto guard = ProtectGuard{ class_base, sizeof(uintptr_t), PAGE_READWRITE };
				*(uintptr_t**)class_base = original_vtable;
				original_vtable = nullptr;
			}
		}
		catch (...) {}
	}

private:

	inline uint32_t GetMethodCount(uintptr_t *vtable_start)
	{
		uint32_t len = -1;

		do ++len; while (vtable_start[len]);

		return len;
	}

	void *class_base;
	uint32_t method_count;
	uintptr_t *shadow_vtable;
	uintptr_t *original_vtable;
};