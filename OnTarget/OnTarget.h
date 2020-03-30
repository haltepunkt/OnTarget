#pragma comment(lib, "BakkesMod.lib")

#include <fstream>

#include "bakkesmod/plugin/bakkesmodplugin.h"
#include "bakkesmod/plugin/pluginwindow.h"

#include "imgui/imgui.h"

#define RED ImColor(255, 0, 0, 255)
#define LIGHTRED ImColor(170, 0, 0, 255)
#define GREEN ImColor(0, 255, 0, 255)
#define LIGHTGREEN ImColor(0, 170, 0, 255)
#define GREY ImColor(170, 170, 170, 255)
#define DARKGREY ImColor(85, 85, 85, 255)

using namespace placeholders;

struct Shot {
	ImVec2 location;
	bool goal;
	bool multiTouch;
};

class OnTarget : public BakkesMod::Plugin::BakkesModPlugin, public BakkesMod::Plugin::PluginWindow
{
public:
	void onLoad();
	void onUnload();

	void writeCfg();

	void onIncrementRound(string eventName);
	void onTrainingLoad(string eventName);
	void onStartNewRound(string eventName);
	void onResetRoundConfirm(string eventName);

	void onHitGoal(BallWrapper ball, void* params, string eventName);
	void onHitWorld(BallWrapper ball, void* params, string eventName);
	ImVec2 flattenToPlane(Vector vector);

	void Render();
	void RenderImGui();
	string GetMenuName();
	string GetMenuTitle();
	void SetImGuiContext(uintptr_t ctx);
	bool ShouldBlockInput();
	bool IsActiveOverlay();
	void OnOpen();
	void OnClose();

	bool renderImgui = false;

	string configurationFilePath = "./bakkesmod/cfg/ontarget.cfg";

	int shotHistory = 128;
	bool titleBar = true;
	float transparency = 1;

	uint64_t ballHitsRound = 0;
	vector<Shot> shots;
};
