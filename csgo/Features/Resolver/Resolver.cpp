#include "Resolver.h"
#include "..\Aimbot\Aimbot.h"
#include "..\Aimbot\Autowall.h"
#include "..\Aimbot\LagComp.h"
#include "..\..\Utils\Utils.h"
#include "..\..\SDK\IVEngineClient.h"
#include "..\..\SDK\Hitboxes.h"
#include "..\..\SDK\PlayerInfo.h"
#include "..\..\Utils\Math.h"
#include "..\..\Menu\Menu.h"

Resolver g_Resolver;

/*
my attempt at fixing desync and i was pretty successful
it can resolve static desync pretty perfectly 
and can resolve some jitter desync but 
it still gets rekt by other things
*/

void Resolver::AnimationFix(C_BaseEntity* pEnt)
{
	//who needs structs or classes not me lol
	static float oldSimtime[65];
	static float storedSimtime[65];
	static float ShotTime[65];
	static float SideTime[65][3];
	static int LastDesyncSide[65];
	static bool Delaying[65];
	static AnimationLayer StoredLayers[64][15];
	static C_AnimState * StoredAnimState[65];
	static float StoredPosParams[65][24];
	static Vector oldEyeAngles[65];
	static float oldGoalfeetYaw[65];
	float* PosParams = (float*)((uintptr_t)pEnt + 0x2774);
	bool update = false;
	bool shot = false;

	static bool jittering[65];

	auto* AnimState = pEnt->AnimState();
	if (!AnimState || !pEnt->AnimOverlays() || !PosParams)
		return;

	auto RemapVal = [](float val, float A, float B, float C, float D) -> float
	{
		if (A == B)
			return val >= B ? D : C;
		return C + (D - C) * (val - A) / (B - A);
	};

	if (storedSimtime[pEnt->EntIndex()] != pEnt->GetSimulationTime())
	{
		jittering[pEnt->EntIndex()] = false;
		pEnt->ClientAnimations(true);
		pEnt->UpdateClientAnimation();

		memcpy(StoredPosParams[pEnt->EntIndex()], PosParams, sizeof(float) * 24);
		memcpy(StoredLayers[pEnt->EntIndex()], pEnt->AnimOverlays(), (sizeof(AnimationLayer) * pEnt->NumOverlays()));

		oldGoalfeetYaw[pEnt->EntIndex()] = AnimState->m_flGoalFeetYaw;

		if (pEnt->GetActiveWeapon() && !pEnt->IsKnifeorNade())
		{
			if (ShotTime[pEnt->EntIndex()] != pEnt->GetActiveWeapon()->GetLastShotTime())
			{
				shot = true;
				ShotTime[pEnt->EntIndex()] = pEnt->GetActiveWeapon()->GetLastShotTime();
			}
			else
				shot = false;
		}
		else
		{
			shot = false;
			ShotTime[pEnt->EntIndex()] = 0.f;
		}

		float angToLocal = g_Math.NormalizeYaw(g_Math.CalcAngle(g::pLocalEntity->GetOrigin(), pEnt->GetOrigin()).y);

		float Back = g_Math.NormalizeYaw(angToLocal);
		float DesyncFix = 0;
		float Resim = g_Math.NormalizeYaw((0.24f / (pEnt->GetSimulationTime() - oldSimtime[pEnt->EntIndex()]))*(oldEyeAngles[pEnt->EntIndex()].y - pEnt->GetEyeAngles().y));

		if (Resim > 58.f)
			Resim = 58.f;
		if (Resim < -58.f)
			Resim = -58.f;

		if (pEnt->GetVelocity().Length2D() > 0.5f && !shot)
		{
			float Delta = g_Math.NormalizeYaw(g_Math.NormalizeYaw(g_Math.CalcAngle(Vector(0, 0, 0), pEnt->GetVelocity()).y) - g_Math.NormalizeYaw(g_Math.NormalizeYaw(AnimState->m_flGoalFeetYaw + RemapVal(PosParams[11], 0, 1, -60, 60)) + Resim));

			int CurrentSide = 0;

			if (Delta < 0)
			{
				CurrentSide = 1;
				SideTime[pEnt->EntIndex()][1] = g_pGlobalVars->curtime;
			}
			else if (Delta > 0)
			{
				CurrentSide = 2;
				SideTime[pEnt->EntIndex()][2] = g_pGlobalVars->curtime;
			}

			if (LastDesyncSide[pEnt->EntIndex()] == 1)
			{
				Resim += (58.f - Resim);
				DesyncFix += (58.f - Resim);
			}
			if (LastDesyncSide[pEnt->EntIndex()] == 2)
			{
				Resim += (-58.f - Resim);
				DesyncFix += (-58.f - Resim);
			}

			if (LastDesyncSide[pEnt->EntIndex()] != CurrentSide)
			{
				Delaying[pEnt->EntIndex()] = true;

				if (.5f < (g_pGlobalVars->curtime - SideTime[pEnt->EntIndex()][LastDesyncSide[pEnt->EntIndex()]]))
				{
					LastDesyncSide[pEnt->EntIndex()] = CurrentSide;
					Delaying[pEnt->EntIndex()] = false;
				}
			}

			if (!Delaying[pEnt->EntIndex()])
				LastDesyncSide[pEnt->EntIndex()] = CurrentSide;
		}
		else if (!shot)
		{
			float Brute = UseFreestandAngle[pEnt->EntIndex()] ? g_Math.NormalizeYaw(Back + FreestandAngle[pEnt->EntIndex()]) : pEnt->GetLowerBodyYaw();

			float Delta = g_Math.NormalizeYaw(g_Math.NormalizeYaw(Brute - g_Math.NormalizeYaw(g_Math.NormalizeYaw(AnimState->m_flGoalFeetYaw + RemapVal(PosParams[11], 0, 1, -60, 60))) + Resim));

			if (Delta > 58.f)
				Delta = 58.f;
			if (Delta < -58.f)
				Delta = -58.f;

			Resim += Delta;
			DesyncFix += Delta;

			if (Resim > 58.f)
				Resim = 58.f;
			if (Resim < -58.f)
				Resim = -58.f;
		}

		float Equalized = g_Math.NormalizeYaw(g_Math.NormalizeYaw(AnimState->m_flGoalFeetYaw + RemapVal(PosParams[11], 0, 1, -60, 60)) + Resim);

		float JitterDelta = fabs(g_Math.NormalizeYaw(oldEyeAngles[pEnt->EntIndex()].y - pEnt->GetEyeAngles().y));

		if (JitterDelta >= 70.f && !shot)
			jittering[pEnt->EntIndex()] = true;

		if (pEnt != g::pLocalEntity && pEnt->GetTeam() != g::pLocalEntity->GetTeam() && (pEnt->GetFlags() & FL_ONGROUND) && g_Menu.Config.Resolver)
		{
			if (jittering[pEnt->EntIndex()])
				AnimState->m_flGoalFeetYaw = g_Math.NormalizeYaw(pEnt->GetEyeAngles().y + DesyncFix);
			else
				AnimState->m_flGoalFeetYaw = Equalized;

			pEnt->SetLowerBodyYaw(AnimState->m_flGoalFeetYaw);
		}

		StoredAnimState[pEnt->EntIndex()] = AnimState;

		oldEyeAngles[pEnt->EntIndex()] = pEnt->GetEyeAngles();

		oldSimtime[pEnt->EntIndex()] = storedSimtime[pEnt->EntIndex()];

		storedSimtime[pEnt->EntIndex()] = pEnt->GetSimulationTime();

		update = true;
	}

	pEnt->ClientAnimations(false);

	if (pEnt != g::pLocalEntity && pEnt->GetTeam() != g::pLocalEntity->GetTeam() && (pEnt->GetFlags() & FL_ONGROUND) && g_Menu.Config.Resolver)
		pEnt->SetLowerBodyYaw(AnimState->m_flGoalFeetYaw);

	AnimState = StoredAnimState[pEnt->EntIndex()];

	memcpy((void*)PosParams, &StoredPosParams[pEnt->EntIndex()], (sizeof(float) * 24));
	memcpy(pEnt->AnimOverlays(), StoredLayers[pEnt->EntIndex()], (sizeof(AnimationLayer) * pEnt->NumOverlays()));

	if (pEnt != g::pLocalEntity && pEnt->GetTeam() != g::pLocalEntity->GetTeam() && (pEnt->GetFlags() & FL_ONGROUND) && g_Menu.Config.Resolver && jittering[pEnt->EntIndex()])
		pEnt->SetAbsAngles(Vector(0, pEnt->GetEyeAngles().y, 0));
	else
		pEnt->SetAbsAngles(Vector(0, oldGoalfeetYaw[pEnt->EntIndex()], 0));

	*reinterpret_cast<int*>(uintptr_t(pEnt) + 0xA30) = g_pGlobalVars->framecount;
	*reinterpret_cast<int*>(uintptr_t(pEnt) + 0xA28) = 0;
}

void HandleHits(C_BaseEntity* pEnt)
{
	auto NetChannel = g_pEngine->GetNetChannelInfo();

	if (!NetChannel)
		return;

	static float predTime[65];
	static bool init[65];

	if (g::Shot[pEnt->EntIndex()])
	{
		if (init[pEnt->EntIndex()])
		{
			g_Resolver.pitchHit[pEnt->EntIndex()] = pEnt->GetEyeAngles().x;
			predTime[pEnt->EntIndex()] = g_pGlobalVars->curtime + NetChannel->GetAvgLatency(FLOW_INCOMING) + NetChannel->GetAvgLatency(FLOW_OUTGOING) + TICKS_TO_TIME(1) + TICKS_TO_TIME(g_pEngine->GetNetChannel()->m_nChokedPackets);
			init[pEnt->EntIndex()] = false;
		}

		if (g_pGlobalVars->curtime > predTime[pEnt->EntIndex()] && !g::Hit[pEnt->EntIndex()])
		{
			g::MissedShots[pEnt->EntIndex()] += 1;
			g::Shot[pEnt->EntIndex()] = false;
		}
		else if (g_pGlobalVars->curtime <= predTime[pEnt->EntIndex()] && g::Hit[pEnt->EntIndex()])
			g::Shot[pEnt->EntIndex()] = false;

	}
	else
		init[pEnt->EntIndex()] = true;

	g::Hit[pEnt->EntIndex()] = false;
}

void Resolver::OnCreateMove() // cancer v2
{
	if (!g_Menu.Config.Resolver)
		return;

	if (!g::pLocalEntity->IsAlive())
		return;

	if (!g::pLocalEntity->GetActiveWeapon() || g::pLocalEntity->IsKnifeorNade())
		return;


	for (int i = 1; i < g_pEngine->GetMaxClients(); ++i)
	{
		C_BaseEntity* pPlayerEntity = g_pEntityList->GetClientEntity(i);

		if (!pPlayerEntity
			|| !pPlayerEntity->IsAlive()
			|| pPlayerEntity->IsDormant()
			|| pPlayerEntity == g::pLocalEntity
			|| pPlayerEntity->GetTeam() == g::pLocalEntity->GetTeam())
		{
			UseFreestandAngle[i] = false;
			continue;
		}

		if (abs(pPlayerEntity->GetVelocity().Length2D()) > 29.f)
			UseFreestandAngle[pPlayerEntity->EntIndex()] = false;

		if (abs(pPlayerEntity->GetVelocity().Length2D()) <= 29.f && !UseFreestandAngle[pPlayerEntity->EntIndex()])
		{
			bool Autowalled = false, HitSide1 = false, HitSide2 = false;

			float angToLocal = g_Math.CalcAngle(g::pLocalEntity->GetOrigin(), pPlayerEntity->GetOrigin()).y;
			Vector ViewPoint = g::pLocalEntity->GetOrigin() + Vector(0, 0, 90);

			Vector2D Side1 = { (45 * sin(g_Math.GRD_TO_BOG(angToLocal))),(45 * cos(g_Math.GRD_TO_BOG(angToLocal))) };
			Vector2D Side2 = { (45 * sin(g_Math.GRD_TO_BOG(angToLocal + 180))) ,(45 * cos(g_Math.GRD_TO_BOG(angToLocal + 180))) };

			Vector2D Side3 = { (50 * sin(g_Math.GRD_TO_BOG(angToLocal))),(50 * cos(g_Math.GRD_TO_BOG(angToLocal))) };
			Vector2D Side4 = { (50 * sin(g_Math.GRD_TO_BOG(angToLocal + 180))) ,(50 * cos(g_Math.GRD_TO_BOG(angToLocal + 180))) };

			Vector Origin = pPlayerEntity->GetOrigin();

			Vector2D OriginLeftRight[] = { Vector2D(Side1.x, Side1.y), Vector2D(Side2.x, Side2.y) };

			Vector2D OriginLeftRightLocal[] = { Vector2D(Side3.x, Side3.y), Vector2D(Side4.x, Side4.y) };

			for (int side = 0; side < 2; side++)
			{
				Vector OriginAutowall = { Origin.x + OriginLeftRight[side].x,  Origin.y - OriginLeftRight[side].y , Origin.z + 90 };
				Vector OriginAutowall2 = { ViewPoint.x + OriginLeftRightLocal[side].x,  ViewPoint.y - OriginLeftRightLocal[side].y , ViewPoint.z };

				if (g_Autowall.CanHitFloatingPoint(OriginAutowall, ViewPoint))
				{
					if (side == 0)
					{
						HitSide1 = true;
						FreestandAngle[pPlayerEntity->EntIndex()] = 90;
					}
					else if (side == 1)
					{
						HitSide2 = true;
						FreestandAngle[pPlayerEntity->EntIndex()] = -90;
					}

					Autowalled = true;
				}
				else
				{
					for (int side222 = 0; side222 < 2; side222++)
					{
						Vector OriginAutowall222 = { Origin.x + OriginLeftRight[side222].x,  Origin.y - OriginLeftRight[side222].y , Origin.z + 90 };

						if (g_Autowall.CanHitFloatingPoint(OriginAutowall222, OriginAutowall2))
						{
							if (side222 == 0)
							{
								HitSide1 = true;
								FreestandAngle[pPlayerEntity->EntIndex()] = 90;
							}
							else if (side222 == 1)
							{
								HitSide2 = true;
								FreestandAngle[pPlayerEntity->EntIndex()] = -90;
							}

							Autowalled = true;
						}
					}
				}
			}

			if (Autowalled)
			{
				if (HitSide1 && HitSide2)	
					UseFreestandAngle[pPlayerEntity->EntIndex()] = false;
				else
					UseFreestandAngle[pPlayerEntity->EntIndex()] = true;
			}
		}
	}
}

void Resolver::FrameStage(ClientFrameStage_t stage)
{
	if (!g::pLocalEntity || !g_pEngine->IsInGame())
		return;

	static bool  wasDormant[65];

	for (int i = 1; i < g_pEngine->GetMaxClients(); ++i)
	{
		C_BaseEntity* pPlayerEntity = g_pEntityList->GetClientEntity(i);

		if (!pPlayerEntity
			|| !pPlayerEntity->IsAlive())
			continue;
		if (pPlayerEntity->IsDormant())
		{
			wasDormant[i] = true;
			continue;
		}

		if (stage == FRAME_RENDER_START)
		{
			HandleHits(pPlayerEntity);
			AnimationFix(pPlayerEntity);
		}

		if (stage == FRAME_NET_UPDATE_END && pPlayerEntity != g::pLocalEntity)
		{
			auto VarMap = reinterpret_cast<uintptr_t>(pPlayerEntity) + 36;
			auto VarMapSize = *reinterpret_cast<int*>(VarMap + 20);

			for (auto index = 0; index < VarMapSize; index++)
				*reinterpret_cast<uintptr_t*>(*reinterpret_cast<uintptr_t*>(VarMap) + index * 12) = 0;
		}

		wasDormant[i] = false;
	}
}