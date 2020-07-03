#include "GlobalVars.h"

namespace g
{
    CUserCmd*      pCmd         = nullptr;
    C_BaseEntity*  pLocalEntity = nullptr;
    std::uintptr_t uRandomSeed  = NULL;
	Vector         OriginalView;
	bool           bSendPacket  = true;
	bool		   LagPeek      = false;
	int            TargetIndex  = -1;
	Vector         EnemyEyeAngs[65];
	Vector         AimbotHitbox[65][28];
	Vector         RealAngle;
	Vector         FakeAngle;
	bool           Shot[65];
	bool           Hit[65];
	int            MissedShots[65];

	DWORD CourierNew;
	DWORD Tahoma;
}
