#pragma once
#include "..\SDK\CInput.h"
#include "..\SDK\CEntity.h"

namespace g
{
    extern CUserCmd*      pCmd;
    extern C_BaseEntity*  pLocalEntity;
    extern std::uintptr_t uRandomSeed;
	extern Vector         OriginalView;
	extern bool           bSendPacket;
	extern bool			  LagPeek;
	extern int            TargetIndex;
	extern Vector         EnemyEyeAngs[65];
	extern Vector         AimbotHitbox[65][28];
	extern Vector         RealAngle;
	extern Vector         FakeAngle;
	extern bool           Shot[65];
	extern bool           Hit[65];
	extern int            MissedShots[65];

	extern DWORD CourierNew;
	extern DWORD Tahoma;
}