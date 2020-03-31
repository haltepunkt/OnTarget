#include "OnTarget.h"

BAKKESMOD_PLUGIN(OnTarget, "On Target", "1.0", PLUGINTYPE_CUSTOM_TRAINING)

void OnTarget::onLoad()
{
	gameWrapper->SetTimeout([this](GameWrapper* gameWrapper) {
		cvarManager->executeCommand("togglemenu " + GetMenuName());
	}, 1);

	cvarManager->registerCvar(shotHistorySetting, "128").addOnValueChanged([this](string old, CVarWrapper now) {
		shotHistory = now.getIntValue();

		writeCfg();
	});

	cvarManager->registerCvar(titleBarSetting, "1").addOnValueChanged([this](string old, CVarWrapper now) {
		titleBar = (now.getStringValue() == "1");

		writeCfg();
	});

	cvarManager->registerCvar("onTargetTransparency", "0.5").addOnValueChanged([this](string old, CVarWrapper now) {
		transparency = now.getFloatValue();

		writeCfg();
	});

	cvarManager->registerCvar(goalHitColorSetting, "0,255,0").addOnValueChanged([this](string old, CVarWrapper now) {
		ImColor color = stringToImColor(now.getStringValue());

		if (color != NULL) {
			goalHitColor = color;

			writeCfg();
		}
	});

	cvarManager->registerCvar(multiTouchColorSetting, "170,170,170").addOnValueChanged([this](string old, CVarWrapper now) {
		ImColor color = stringToImColor(now.getStringValue());

		if (color != NULL) {
			multiTouchColor = color;

			writeCfg();
		}
	});

	cvarManager->registerCvar(wallHitColorSetting, "255,0,0").addOnValueChanged([this](string old, CVarWrapper now) {
		ImColor color = stringToImColor(now.getStringValue());

		if (color != NULL) {
			wallHitColor = color;

			writeCfg();
		}
	});

	if (ifstream(configurationFilePath)) {
		cvarManager->loadCfg(configurationFilePath);
	}

	else {
		vector<string> settings = { "onTargetShotHistory", "onTargetTitleBar", "onTargetTransparency", goalHitColorSetting, multiTouchColorSetting, wallHitColorSetting };

		for (string& setting : settings) {
			cvarManager->getCvar(setting).notify();
		}
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
	writeCfg();

	if (renderShotChart) {
		cvarManager->executeCommand("togglemenu " + GetMenuName());
	}
}

void OnTarget::writeCfg()
{
	ofstream configurationFile;

	configurationFile.open(configurationFilePath);

	configurationFile << shotHistorySetting + " \"" + to_string(shotHistory) + "\"";
	configurationFile << "\n";
	configurationFile << titleBarSetting + " \"" + to_string(titleBar) + "\"";
	configurationFile << "\n";
	configurationFile << transparencySetting + " \"" + to_string(transparency) + "\"";
	configurationFile << "\n";
	configurationFile << goalHitColorSetting + " \"" + ImColorToString(goalHitColor) + "\"";
	configurationFile << "\n";
	configurationFile << multiTouchColorSetting + " \"" + ImColorToString(multiTouchColor) + "\"";
	configurationFile << "\n";
	configurationFile << wallHitColorSetting + " \"" + ImColorToString(wallHitColor) + "\"";

	configurationFile.close();
}

ImColor OnTarget::stringToImColor(string colorString)
{
	if (colorString.length() >= 5) {
		stringstream ss(colorString);

		vector<string> rgb;

		while (ss.good()) {
			string value;

			getline(ss, value, ',');

			rgb.push_back(value);
		}

		if (rgb.size() == 3) {
			return ImColor(stoi(rgb.at(0)), stoi(rgb.at(1)), stoi(rgb.at(2)));
		}
	}

	return NULL;
}

string OnTarget::ImColorToString(ImColor color)
{
	return to_string((int)(color.Value.x * 255)) + "," + to_string((int)(color.Value.y * 255)) + "," + to_string((int)(color.Value.z * 255));
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
	if (!renderShotChart) {
		cvarManager->executeCommand("togglemenu " + GetMenuName());

		return;
	}

	if (gameWrapper->IsInCustomTraining()) {
		OnTarget::RenderImGui();
	}
}

void OnTarget::RenderImGui()
{
	if (renderSettings) {
		ImGui::SetNextWindowPos(ImVec2(128, 256), ImGuiCond_FirstUseEver);
		ImGui::SetNextWindowSize(ImVec2(0, 0));

		ImGui::Begin((GetMenuTitle() + " - Settings").c_str(), &renderSettings, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize);

		ImGui::Checkbox("Titlebar", &titleBar);
		ImGui::InputInt("Shot History", &shotHistory, 1, 16, ImGuiInputTextFlags_CharsNoBlank | ImGuiInputTextFlags_AutoSelectAll);
		ImGui::SliderFloat("Transparency", &transparency, 0.f, 1.f, "%.2f");

		ImGui::Separator();

		if (ImGui::ColorEdit3("Goal Hit", (float*)&goalHitColor, ImGuiColorEditFlags_NoAlpha | ImGuiColorEditFlags_NoOptions | ImGuiColorEditFlags_NoInputs)) {
			cvarManager->getCvar(goalHitColorSetting).setValue(ImColorToString(goalHitColor));
		}

		if (ImGui::ColorEdit3("Wall Hit", (float*)&wallHitColor, ImGuiColorEditFlags_NoAlpha | ImGuiColorEditFlags_NoOptions | ImGuiColorEditFlags_NoInputs)) {
			cvarManager->getCvar(wallHitColorSetting).setValue(ImColorToString(wallHitColor));
		}

		if (ImGui::ColorEdit3("Multi Touch", (float*)&multiTouchColor, ImGuiColorEditFlags_NoAlpha | ImGuiColorEditFlags_NoOptions | ImGuiColorEditFlags_NoInputs)) {
			cvarManager->getCvar(multiTouchColorSetting).setValue(ImColorToString(multiTouchColor));
		}

		ImGui::End();
	}

	float scale = .5f;

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
	
	ImGui::Begin(GetMenuTitle().c_str(), &renderShotChart, windowFlags);

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

		uint64_t attempts = 0, goals = 0;

		for (uint64_t idx = 0; idx < shots.size(); idx++) {
			if (idx + 1 == shots.size()) {
				s = scale + 0.25f;
			}

			if (shots.at(idx).goal) {
				if (idx + 1 == shots.size()) {
					drawList->AddCircle(ImVec2(p.x + (wallWidth / 2) + shots.at(idx).location.x * (wallWidth / 2), p.y + wallHeight - shots.at(idx).location.y * wallHeight), 10.f * s, WHITE, 16, 7);
				}

				drawList->AddCircle(ImVec2(p.x + (wallWidth / 2) + shots.at(idx).location.x * (wallWidth / 2), p.y + wallHeight - shots.at(idx).location.y * wallHeight), 10.f * s, goalHitColor, 16, 3);

				attempts++;
				goals++;
			}

			else if (shots.at(idx).multiTouch) {
				float line = 8 * s;

				drawList->AddLine(ImVec2((p.x + (wallWidth / 2) + shots.at(idx).location.x * (wallWidth / 2)) - line, (p.y + wallHeight - shots.at(idx).location.y * wallHeight) - line),
					ImVec2((p.x + (wallWidth / 2) + shots.at(idx).location.x * (wallWidth / 2)) + line, (p.y + wallHeight - shots.at(idx).location.y * wallHeight) + line),
					multiTouchColor, 3);
				drawList->AddLine(ImVec2((p.x + (wallWidth / 2) + shots.at(idx).location.x * (wallWidth / 2)) - line, (p.y + wallHeight - shots.at(idx).location.y * wallHeight) + line),
					ImVec2((p.x + (wallWidth / 2) + shots.at(idx).location.x * (wallWidth / 2)) + line, (p.y + wallHeight - shots.at(idx).location.y * wallHeight) - line),
					multiTouchColor, 3);
			}

			else if (!shots.at(idx).goal && !shots.at(idx).multiTouch) {
				if (idx + 1 == shots.size()) {
					float line = 10 * s;

					drawList->AddLine(ImVec2((p.x + (wallWidth / 2) + shots.at(idx).location.x * (wallWidth / 2)) - line, (p.y + wallHeight - shots.at(idx).location.y * wallHeight) - line),
						ImVec2((p.x + (wallWidth / 2) + shots.at(idx).location.x * (wallWidth / 2)) + line, (p.y + wallHeight - shots.at(idx).location.y * wallHeight) + line),
						WHITE, 7);
					drawList->AddLine(ImVec2((p.x + (wallWidth / 2) + shots.at(idx).location.x * (wallWidth / 2)) - line, (p.y + wallHeight - shots.at(idx).location.y * wallHeight) + line),
						ImVec2((p.x + (wallWidth / 2) + shots.at(idx).location.x * (wallWidth / 2)) + line, (p.y + wallHeight - shots.at(idx).location.y * wallHeight) - line),
						WHITE, 7);
				}

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

	ImGui::SetCursorPos(ImVec2(ImGui::GetCursorPosX() + ImGui::GetWindowWidth() - 76, ImGui::GetCursorPosY() + 6));

	if (ImGui::IsMouseHoveringRect(ImVec2(ImGui::GetWindowPos().x, ImGui::GetWindowPos().y), ImVec2(ImGui::GetWindowPos().x + ImGui::GetWindowWidth(), ImGui::GetWindowPos().y + ImGui::GetWindowHeight()))) {
		if (ImGui::Button("Settings")) {
			renderSettings = !renderSettings;
		}
	}

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
	renderShotChart = true;
}

void OnTarget::OnClose()
{
	renderShotChart = false;
}
