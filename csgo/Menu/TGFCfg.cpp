#include "Menu.h"
#include "TGFCfg.h"

// credits to team gamer food

void CConfig::Setup()
{
	SetupValue(g_Menu.Config.Aimbot, false, "aimbot");
	SetupValue(g_Menu.Config.Hitchance, 0, "hitchance");
	SetupValue(g_Menu.Config.HitchanceValue, 0, "hitchanceval");
	SetupValue(g_Menu.Config.Mindmg, 0, "mindmg");
	SetupValue(g_Menu.Config.Resolver, false, "resolver");
	SetupValue(g_Menu.Config.BodyScale, 0, "bodyscale");
	SetupValue(g_Menu.Config.HeadScale, 0, "headscale");
	SetupValue(g_Menu.Config.MultiPoint, false, "multipoint");
	SetupValue(g_Menu.Config.DelayShot, false, "delayshot");
	SetupValue(g_Menu.Config.IgnoreLimbs, false, "ignorelimbs");
	SetupValue(g_Menu.Config.Autostop, false, "autostop");
	SetupValue(g_Menu.Config.FixShotPitch, false, "fixshotpitch");
	SetupValue(g_Menu.Config.ShotBacktrack, false, "shotbacktrk");
	SetupValue(g_Menu.Config.PosBacktrack, false, "posbacktrk");
	SetupValue(g_Menu.Config.BaimLethal, false, "baimlethal");
	SetupValue(g_Menu.Config.BaimLethal, false, "baimlethal");
	SetupValue(g_Menu.Config.BaimPitch, false, "bamepitch");
	SetupValue(g_Menu.Config.BaimInAir, false, "baiminair");

	SetupValue(g_Menu.Config.Antiaim, false, "antiaimd");
	SetupValue(g_Menu.Config.DesyncAngle, false, "desyncang");
	SetupValue(g_Menu.Config.RandJitterInRange, false, "randjitterrng");
	SetupValue(g_Menu.Config.JitterRange, 0, "jitterrange");
	SetupValue(g_Menu.Config.Fakelag, 0, "fakelag");
	SetupValue(g_Menu.Config.FakeLagOnPeek, false, "onpeekflag");
	SetupValue(g_Menu.Config.ChokeShotOnPeek, false, "chokeshotonpeek");

	SetupValue(g_Menu.Config.Esp, false, "esp");
	SetupValue(g_Menu.Config.Font, 0, "font");
	SetupValue(g_Menu.Config.FontColor, 255.f, "fontclr");
	SetupValue(g_Menu.Config.Name, 0, "name");
	SetupValue(g_Menu.Config.HealthVal, 0, "healthval");
	SetupValue(g_Menu.Config.Weapon, 0, "weapon");
	SetupValue(g_Menu.Config.Box, false, "box");
	SetupValue(g_Menu.Config.BoxColor, 255.f, "boxclr");
	SetupValue(g_Menu.Config.HealthBar, false, "healthbar");
	SetupValue(g_Menu.Config.Skeleton, false, 2, "skeleton");
	SetupValue(g_Menu.Config.SkeletonColor, 255.f, "skeletonclr");
	SetupValue(g_Menu.Config.HitboxPoints, false, "hitboxpoints");
	SetupValue(g_Menu.Config.Chams, false, "chams");
	SetupValue(g_Menu.Config.ChamsColor, 255.f, "chamsclr");
	SetupValue(g_Menu.Config.NoZoom, false, "nozoom");
	SetupValue(g_Menu.Config.NoScope, false, "noscope");
	SetupValue(g_Menu.Config.NoRecoil, false, "norecoil");
	SetupValue(g_Menu.Config.Fov, 0, "fov");
	SetupValue(g_Menu.Config.Crosshair, false, "crosshair");

	SetupValue(g_Menu.Config.Bhop, false, "bhop");
	SetupValue(g_Menu.Config.AutoStrafe, false, "autostrafe");
//	SetupValue(g_Menu.Config.LegitBacktrack, false, "legitbacktrack");
	SetupValue(g_Menu.Config.Ak47meme, false, "ak47meme");
}

void CConfig::SetupValue(int& value, int def, std::string name)
{
	value = def;
	ints.push_back(new ConfigValue< int >(name, &value));
}

void CConfig::SetupValue(float& value, float def, std::string name)
{
	value = def;
	floats.push_back(new ConfigValue< float >(name, &value));
}

void CConfig::SetupValue(bool& value, bool def, std::string name)
{
	value = def;
	bools.push_back(new ConfigValue< bool >(name, &value));
}

void CConfig::SetupValue(ColorV2& value, float def, std::string name)
{
	value.red = def;
	value.green = def;
	value.blue = def;
	value.alpha = def;

	floats.push_back(new ConfigValue< float >(name + "red", &value.red));
	floats.push_back(new ConfigValue< float >(name + "green", &value.green));
	floats.push_back(new ConfigValue< float >(name + "blue", &value.blue));
	floats.push_back(new ConfigValue< float >(name + "alpha", &value.alpha));
}

void CConfig::SetupValue(bool* value, bool def, int size, std::string name) // for multiboxes
{
	for (int c = 0; c < size; c++)
	{
		value[c] = def;

		name += std::to_string(c);

		bools.push_back(new ConfigValue< bool >(name, &value[c]));
	}
}

static char name[] = "csgohvh";

void CConfig::Save()
{
	static TCHAR path[MAX_PATH];
	std::string folder, file;

	if (SUCCEEDED(SHGetFolderPath(NULL, CSIDL_APPDATA, NULL, 0, path)))
	{
		folder = std::string(path) + "\\csgohvh\\";
		file = std::string(path) + "\\csgohvh\\cfg.ini";
	}

	CreateDirectory(folder.c_str(), NULL);

	for (auto value : ints)
		WritePrivateProfileString(name, value->name.c_str(), std::to_string(*value->value).c_str(), file.c_str());

	for (auto value : floats)
		WritePrivateProfileString(name, value->name.c_str(), std::to_string(*value->value).c_str(), file.c_str());

	for (auto value : bools)
		WritePrivateProfileString(name, value->name.c_str(), *value->value ? "true" : "false", file.c_str());
}

void CConfig::Load()
{
	static TCHAR path[MAX_PATH];
	std::string folder, file;

	if (SUCCEEDED(SHGetFolderPath(NULL, CSIDL_APPDATA, NULL, 0, path)))
	{
		folder = std::string(path) + "\\csgohvh\\";
		file = std::string(path) + "\\csgohvh\\cfg.ini";
	}

	CreateDirectory(folder.c_str(), NULL);

	char value_l[32] = { '\0' };

	for (auto value : ints)
	{
		GetPrivateProfileString(name, value->name.c_str(), "", value_l, 32, file.c_str());
		*value->value = atoi(value_l);
	}

	for (auto value : floats)
	{
		GetPrivateProfileString(name, value->name.c_str(), "", value_l, 32, file.c_str());
		*value->value = atof(value_l);
	}

	for (auto value : bools)
	{
		GetPrivateProfileString(name, value->name.c_str(), "", value_l, 32, file.c_str());
		*value->value = !strcmp(value_l, "true");
	}
}

CConfig* g_Config = new CConfig();