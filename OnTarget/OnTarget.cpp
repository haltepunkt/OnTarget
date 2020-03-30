#include "OnTarget.h"

BAKKESMOD_PLUGIN(OnTarget, "On Target", "1.0", PLUGINTYPE_CUSTOM_TRAINING)

void OnTarget::onLoad()
{
	gameWrapper->SetTimeout([this](GameWrapper* gameWrapper) {
		cvarManager->executeCommand("togglemenu " + GetMenuName());
	}, 1);

	cvarManager->registerCvar("onTargetShotHistory", "128").addOnValueChanged([this](string old, CVarWrapper now) {
		shotHistory = cvarManager->getCvar("onTargetShotHistory").getIntValue();

		writeCfg();
	});

	cvarManager->registerCvar("onTargetTitleBar", "1").addOnValueChanged([this](string old, CVarWrapper now) {
		titleBar = (cvarManager->getCvar("onTargetTitleBar").getStringValue() == "1");

		writeCfg();
	});

	cvarManager->registerCvar("onTargetTransparency", "0.5").addOnValueChanged([this](string old, CVarWrapper now) {
		transparency = cvarManager->getCvar("onTargetTransparency").getFloatValue();

		writeCfg();
	});

	if (ifstream(configurationFilePath)) {
		cvarManager->loadCfg(configurationFilePath);
	}

	else {
		cvarManager->getCvar("onTargetShotHistory").notify();
	}

	gameWrapper->HookEvent("Function TAGame.GameEvent_TrainingEditor_TA.IncrementRound", bind(&OnTarget::onIncrementRound, this, placeholders::_1));
	gameWrapper->HookEvent("Function TAGame.GameEvent_TrainingEditor_TA.Load", bind(&OnTarget::onTrainingLoad, this, placeholders::_1));
	gameWrapper->HookEvent("Function TAGame.GameEvent_TrainingEditor_TA.OnResetRoundConfirm", bind(&OnTarget::onResetRoundConfirm, this, placeholders::_1));
	gameWrapper->HookEvent("Function TAGame.GameEvent_TrainingEditor_TA.StartNewRound", bind(&OnTarget::onStartNewRound, this, placeholders::_1));

	gameWrapper->HookEventWithCaller<BallWrapper>("Function TAGame.Ball_TA.EventHitWorld", std::bind(&OnTarget::onHitWorld, this, placeholders::_1, placeholders::_2, placeholders::_3));
	gameWrapper->HookEventWithCaller<BallWrapper>("Function TAGame.Ball_TA.EventHitGoal", std::bind(&OnTarget::onHitGoal, this, placeholders::_1, placeholders::_2, placeholders::_3));
}

void OnTarget::onUnload()
{
	if (renderImgui) {
		cvarManager->executeCommand("togglemenu " + GetMenuName());
	}
}

void OnTarget::writeCfg()
{
	ofstream configurationFile;

	configurationFile.open(configurationFilePath);

	configurationFile << "onTargetShotHistory \"" + cvarManager->getCvar("onTargetShotHistory").getStringValue() + "\"";
	configurationFile << "\n";
	configurationFile << "onTargetTitleBar \"" + cvarManager->getCvar("onTargetTitleBar").getStringValue() + "\"";
	configurationFile << "\n";
	configurationFile << "onTargetTransparency \"" + cvarManager->getCvar("onTargetTransparency").getStringValue() + "\"";

	configurationFile.close();
}

void OnTarget::onIncrementRound(string eventName)
{
	ballHitsRound = 0;
	shots.clear();
}

void OnTarget::onTrainingLoad(string eventName)
{
	ballHitsRound = 0;
	shots.clear();
}

void OnTarget::onStartNewRound(string eventName)
{
	ballHitsRound = 0;
}

void OnTarget::onResetRoundConfirm(string eventName)
{
	ballHitsRound = 0;
	shots.clear();
}

void OnTarget::onHitGoal(BallWrapper ball, void* params, string eventName)
{
	if (!ball.IsNull()) {
		Vector hitGoalBallLocation = ball.GetLocation();

		if ((hitGoalBallLocation.Z > 93.f && hitGoalBallLocation.Z < 2044.f) && (hitGoalBallLocation.X > -4096.f && hitGoalBallLocation.X < 4096.f)) {
			ImVec2 location = flattenToPlane(hitGoalBallLocation);

			if (shots.size() == shotHistory) {
				shots.erase(shots.begin());
			}

			ballHitsRound++;

			if (ballHitsRound > 1) {
				Shot shot = shots.back();
				shot.multiTouch = true;

				shots.back() = shot;
			}

			shots.push_back(Shot{ location, true , false });
		}
	}
}

void OnTarget::onHitWorld(BallWrapper ball, void* params, string eventName)
{
	if (!ball.IsNull()) {
		Vector hitWorldBallLocation = ball.GetLocation();

		if ((hitWorldBallLocation.Z > 93.f && hitWorldBallLocation.Z < 2044.f) && (hitWorldBallLocation.X > -4096.f && hitWorldBallLocation.X < 4096.f) && (hitWorldBallLocation.Y < -4992.f || hitWorldBallLocation.Y > 4992.f)) {
			ImVec2 location = flattenToPlane(hitWorldBallLocation);

			if (shots.size() == shotHistory) {
				shots.erase(shots.begin());
			}

			ballHitsRound++;

			if (ballHitsRound > 1) {
				Shot shot = shots.back();
				shot.multiTouch = true;

				shots.back() = shot;
			}

			shots.push_back(Shot{ location, false, false });
		}
	}
}

ImVec2 OnTarget::flattenToPlane(Vector vector)
{
	ImVec2 flat;

	if (vector.X == 0) {
		flat.x = 0;
	}

	else {
		flat.x = (vector.X / 4096.f) * -1.f;
	}

	if (vector.Z == 0) {
		flat.y = 0;
	}

	else {
		flat.y = vector.Z / 2044.f;
	}

	return flat;
}

void OnTarget::Render()
{
	if (!renderImgui) {
		cvarManager->executeCommand("togglemenu " + GetMenuName());

		return;
	}

	if (gameWrapper->IsInCustomTraining()) {
		OnTarget::RenderImGui();
	}
}

void OnTarget::RenderImGui()
{
	float scale = 0.5f;

	float goalWidth = 178.f * scale, goalHeight = 64.f * scale;
	float wallWidth = 818.f * scale, wallHeight = 204.f * scale;
	float corner = 28.f * scale;

	float windowHeight = wallHeight + ImGui::GetFrameHeight() + 16;

	ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize;

	if (!titleBar) {
		windowHeight -= ImGui::GetFrameHeight();

		windowFlags = windowFlags | ImGuiWindowFlags_NoTitleBar;
	}

	ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0);

	ImGui::SetNextWindowBgAlpha(transparency);
	ImGui::SetNextWindowPos(ImVec2(128, 128), ImGuiCond_FirstUseEver);
	ImGui::SetNextWindowSize(ImVec2(wallWidth + 16, windowHeight));
	
	ImGui::Begin(GetMenuTitle().c_str(), &renderImgui, windowFlags);

	ImDrawList* drawList = ImGui::GetWindowDrawList();

	ImVec2 p = ImGui::GetCursorScreenPos();

	drawList->AddLine(ImVec2(p.x, p.y + corner), ImVec2(p.x + wallWidth, p.y + corner), DARKGREY, 1);

	drawList->AddLine(ImVec2(p.x, p.y + wallHeight - corner), ImVec2(p.x + wallWidth / 2 - goalWidth / 2, p.y + wallHeight - corner), DARKGREY, 1);
	drawList->AddLine(ImVec2(p.x + wallWidth / 2 - goalWidth / 2 + goalWidth, p.y + wallHeight - corner), ImVec2(p.x + wallWidth, p.y + wallHeight - corner), DARKGREY, 1);

	drawList->AddLine(ImVec2(p.x + corner, p.y), ImVec2(p.x + corner, p.y + wallHeight), DARKGREY, 1);
	drawList->AddLine(ImVec2(p.x + wallWidth - corner, p.y), ImVec2(p.x + wallWidth - corner, p.y + wallHeight), DARKGREY, 1);

	drawList->AddRect(p, ImVec2(p.x + wallWidth, p.y + wallHeight), GREY, 12, ImDrawCornerFlags_All, 3);

	p.x += wallWidth / 2 - goalWidth / 2;
	p.y += wallHeight - goalHeight;

	drawList->AddRect(p, ImVec2(p.x + goalWidth, p.y + goalHeight), GREY, 12, ImDrawCornerFlags_Top, 3);

	p = ImGui::GetCursorScreenPos();

	if (shots.size() > 0) {
		float s = scale;

		ImColor wallHitColor = LIGHTRED;
		ImColor goalHitColor = LIGHTGREEN;

		uint64_t attempts = 0, goals = 0;

		for (uint64_t idx = 0; idx < shots.size(); idx++) {
			if (idx + 1 == shots.size()) {
				s = scale + 0.25f;

				wallHitColor = RED;
				goalHitColor = GREEN;
			}

			if (shots.at(idx).goal) {
				drawList->AddCircle(ImVec2(p.x + (wallWidth / 2) + shots.at(idx).location.x * (wallWidth / 2), p.y + wallHeight - shots.at(idx).location.y * wallHeight), 10.f * s, goalHitColor, 16, 3);

				attempts++;
				goals++;
			}

			else if (shots.at(idx).multiTouch) {
				float line = 8 * s;

				drawList->AddLine(ImVec2((p.x + (wallWidth / 2) + shots.at(idx).location.x * (wallWidth / 2)) - line, (p.y + wallHeight - shots.at(idx).location.y * wallHeight) - line),
					ImVec2((p.x + (wallWidth / 2) + shots.at(idx).location.x * (wallWidth / 2)) + line, (p.y + wallHeight - shots.at(idx).location.y * wallHeight) + line),
					DARKGREY, 3);
				drawList->AddLine(ImVec2((p.x + (wallWidth / 2) + shots.at(idx).location.x * (wallWidth / 2)) - line, (p.y + wallHeight - shots.at(idx).location.y * wallHeight) + line),
					ImVec2((p.x + (wallWidth / 2) + shots.at(idx).location.x * (wallWidth / 2)) + line, (p.y + wallHeight - shots.at(idx).location.y * wallHeight) - line),
					DARKGREY, 3);
			}

			else if (!shots.at(idx).goal && !shots.at(idx).multiTouch) {
				float line = 8 * s;

				drawList->AddLine(ImVec2((p.x + (wallWidth / 2) + shots.at(idx).location.x * (wallWidth / 2)) - line, (p.y + wallHeight - shots.at(idx).location.y * wallHeight) - line),
					ImVec2((p.x + (wallWidth / 2) + shots.at(idx).location.x * (wallWidth / 2)) + line, (p.y + wallHeight - shots.at(idx).location.y * wallHeight) + line),
					wallHitColor, 3);
				drawList->AddLine(ImVec2((p.x + (wallWidth / 2) + shots.at(idx).location.x * (wallWidth / 2)) - line, (p.y + wallHeight - shots.at(idx).location.y * wallHeight) + line),
					ImVec2((p.x + (wallWidth / 2) + shots.at(idx).location.x * (wallWidth / 2)) + line, (p.y + wallHeight - shots.at(idx).location.y * wallHeight) - line),
					wallHitColor, 3);

				attempts++;
			}
		}

		float shootingPercentage = ((float)goals / (float)attempts * 100);

		ImGui::SetCursorPos(ImVec2(ImGui::GetCursorPosX() + 16 * scale, ImGui::GetCursorPosY() + 12 * scale));

		ImGui::Text("%i/%i (%.0f%%)", goals, attempts, shootingPercentage);
	}

	ImGui::PopStyleVar();

	ImGui::End();
}

string OnTarget::GetMenuName()
{
	return "ontarget";
}

string OnTarget::GetMenuTitle()
{
	return "On Target";
}

void OnTarget::SetImGuiContext(uintptr_t ctx)
{
	ImGui::SetCurrentContext(reinterpret_cast<ImGuiContext*>(ctx));
}

bool OnTarget::ShouldBlockInput()
{
	return false;
}

bool OnTarget::IsActiveOverlay()
{
	return false;
}

void OnTarget::OnOpen()
{
	renderImgui = true;
}

void OnTarget::OnClose()
{
	renderImgui = false;
}
