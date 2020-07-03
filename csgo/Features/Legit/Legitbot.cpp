#include "Legitbot.h"
#include "..\Aimbot\Aimbot.h"
#include "..\Aimbot\LagComp.h"
#include "..\..\Utils\Utils.h"
#include "..\..\SDK\IVEngineClient.h"
#include "..\..\SDK\PlayerInfo.h"
#include "..\..\Utils\Math.h"
#include "..\..\SDK\Hitboxes.h"
#include "..\..\Menu\Menu.h"

Legitbot g_Legitbot;

// NOT DONE LMAO

void Legitbot::OnCreateMove()
{
	if ( !g_pEngine->IsInGame() || !g_Menu.Config.LegitBacktrack)
		return;

	if (g::pLocalEntity->IsAlive())
		return;

	if (!g::pLocalEntity->GetActiveWeapon())
		return;

	float flServerTime = g::pLocalEntity->GetTickBase() * g_pGlobalVars->intervalPerTick;
	bool canShoot = (g::pLocalEntity->GetActiveWeapon()->GetNextPrimaryAttack() <= flServerTime);

	static int bestEntTick[65];
	int bestEnt = -1;
	int allEntsBestDistance = 999999999999;

	for (int i = 1; i < g_pEngine->GetMaxClients(); ++i)
	{
		C_BaseEntity* pPlayerEntity = g_pEntityList->GetClientEntity(i);

		if (!pPlayerEntity
			|| !pPlayerEntity->IsAlive()
			|| pPlayerEntity->IsDormant()
			|| pPlayerEntity == g::pLocalEntity
			|| pPlayerEntity->GetTeam() == g::pLocalEntity->GetTeam())
			continue;

		LegitPlayerRecords Setup;
		static float StoredSimTime[65];

		if (StoredSimTime[i] != pPlayerEntity->GetSimulationTime())
		{
			Setup.SimTime = pPlayerEntity->GetSimulationTime();

			for (int hitbox = 0; hitbox < 19; hitbox++)
				Setup.HitBoxes[hitbox] = pPlayerEntity->GetHitboxPosition(hitbox, g_Aimbot.Matrix[pPlayerEntity->EntIndex()]);

			PlayerRecord[i].push_back(Setup);

			StoredSimTime[i] = pPlayerEntity->GetSimulationTime();
		}

		if (PlayerRecord[i].size() > 0)
			for (int tick = 0; tick < PlayerRecord[i].size(); tick++)
				if (!g_LagComp.ValidTick(TIME_TO_TICKS(PlayerRecord[pPlayerEntity->EntIndex()].at(tick).SimTime)))
					PlayerRecord[pPlayerEntity->EntIndex()].erase(PlayerRecord[pPlayerEntity->EntIndex()].begin() + tick);


		int Height, Width;
		g_pEngine->GetScreenSize(Width, Height);

		Vector punchAngle = g::pLocalEntity->GetAimPunchAngle();

		float x = Width / 2;
		float y = Height / 2;
		int dy = Height / 90;
		int dx = Width / 90;
		x -= (dx*(punchAngle.y));
		y += (dy*(punchAngle.x));

		Vector2D screenPunch = { x, y };

		int BestBestDistance = 999999;
		int bestTempDistance = 99999;

		for (int tick = 0; tick < PlayerRecord[i].size(); tick++)
		{
			for (int hitbox = 0; hitbox < 19; hitbox++)
			{
				Vector2D w2sHitbox;
				int distance = 999999999;
				if (Utils::WorldToScreen(PlayerRecord[i].at(tick).HitBoxes[hitbox], w2sHitbox))
					distance = abs(g_Math.Distance(screenPunch, w2sHitbox));

				if (w2sHitbox.x > Width || w2sHitbox.x < 0)
					distance = 999999999999;

				if (bestTempDistance > distance)
				{
					bestTempDistance = distance;
				}
			}

			if (BestBestDistance > bestTempDistance)
			{
				BestBestDistance = bestTempDistance;
				bestEntTick[pPlayerEntity->EntIndex()] = tick;
			}
		}

		if (allEntsBestDistance > BestBestDistance)
		{
			allEntsBestDistance = BestBestDistance;
			bestEnt = pPlayerEntity->EntIndex();
		}
	}

	if ((g::pCmd->buttons & IN_ATTACK) && canShoot && bestEnt != -1)
	{
		g::pCmd->tick_count = TIME_TO_TICKS(PlayerRecord[bestEnt].at(bestEntTick[bestEnt]).SimTime + g_LagComp.LerpTime());
	}
}