#include "Menu.h"

#include <dinput.h>
#include <magic_enum.hpp>
#include "SkyrimTressFX.h"
#define SETTING_MENU_TOGGLEKEY "Toggle Key"

void SetupImGuiStyle()
{
	constexpr auto bg_alpha = 0.68f;
	constexpr auto disabled_alpha = 0.30f;

	constexpr auto background = ImVec4(0.0f, 0.0f, 0.0f, bg_alpha);
	constexpr auto border = ImVec4(0.396f, 0.404f, 0.412f, bg_alpha);

	auto& style = ImGui::GetStyle();
	auto& colors = style.Colors;

	colors[ImGuiCol_Text] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
	colors[ImGuiCol_TextDisabled] = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
	colors[ImGuiCol_WindowBg] = ImVec4(0.13f, 0.14f, 0.15f, 1.00f);
	colors[ImGuiCol_ChildBg] = ImVec4(0.13f, 0.14f, 0.15f, 1.00f);
	colors[ImGuiCol_PopupBg] = ImVec4(0.13f, 0.14f, 0.15f, 1.00f);
	colors[ImGuiCol_Border] = ImVec4(0.43f, 0.43f, 0.50f, 0.50f);
	colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
	colors[ImGuiCol_FrameBg] = ImVec4(0.25f, 0.25f, 0.25f, 1.00f);
	colors[ImGuiCol_FrameBgHovered] = ImVec4(0.38f, 0.38f, 0.38f, 1.00f);
	colors[ImGuiCol_FrameBgActive] = ImVec4(0.67f, 0.67f, 0.67f, 0.39f);
	colors[ImGuiCol_TitleBg] = ImVec4(0.08f, 0.08f, 0.09f, 1.00f);
	colors[ImGuiCol_TitleBgActive] = ImVec4(0.08f, 0.08f, 0.09f, 1.00f);
	colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.00f, 0.00f, 0.00f, 0.51f);
	colors[ImGuiCol_MenuBarBg] = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
	colors[ImGuiCol_ScrollbarBg] = ImVec4(0.02f, 0.02f, 0.02f, 0.53f);
	colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.31f, 0.31f, 0.31f, 1.00f);
	colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.41f, 0.41f, 0.41f, 1.00f);
	colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.51f, 0.51f, 0.51f, 1.00f);
	colors[ImGuiCol_CheckMark] = ImVec4(0.11f, 0.64f, 0.92f, 1.00f);
	colors[ImGuiCol_SliderGrab] = ImVec4(0.11f, 0.64f, 0.92f, 1.00f);
	colors[ImGuiCol_SliderGrabActive] = ImVec4(0.08f, 0.50f, 0.72f, 1.00f);
	colors[ImGuiCol_Button] = ImVec4(0.25f, 0.25f, 0.25f, 1.00f);
	colors[ImGuiCol_ButtonHovered] = ImVec4(0.38f, 0.38f, 0.38f, 1.00f);
	colors[ImGuiCol_ButtonActive] = ImVec4(0.67f, 0.67f, 0.67f, 0.39f);
	colors[ImGuiCol_Header] = ImVec4(0.22f, 0.22f, 0.22f, 1.00f);
	colors[ImGuiCol_HeaderHovered] = ImVec4(0.25f, 0.25f, 0.25f, 1.00f);
	colors[ImGuiCol_HeaderActive] = ImVec4(0.67f, 0.67f, 0.67f, 0.39f);
	colors[ImGuiCol_Separator] = colors[ImGuiCol_Border];
	colors[ImGuiCol_SeparatorHovered] = ImVec4(0.41f, 0.42f, 0.44f, 1.00f);
	colors[ImGuiCol_SeparatorActive] = ImVec4(0.26f, 0.59f, 0.98f, 0.95f);
	colors[ImGuiCol_ResizeGrip] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
	colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.29f, 0.30f, 0.31f, 0.67f);
	colors[ImGuiCol_ResizeGripActive] = ImVec4(0.26f, 0.59f, 0.98f, 0.95f);
	colors[ImGuiCol_Tab] = ImVec4(0.23f, 0.23f, 0.24f, 0.25f);
	colors[ImGuiCol_TabHovered] = ImVec4(0.23f, 0.23f, 0.24f, 1.0f);
	colors[ImGuiCol_TabActive] = ImVec4(0.23f, 0.23f, 0.24f, 0.75f);
	colors[ImGuiCol_TabUnfocused] = ImVec4(0.08f, 0.08f, 0.09f, 1.00f);
	colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.13f, 0.14f, 0.15f, 1.00f);
	colors[ImGuiCol_PlotLines] = ImVec4(0.61f, 0.61f, 0.61f, 1.00f);
	colors[ImGuiCol_PlotLinesHovered] = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
	colors[ImGuiCol_PlotHistogram] = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
	colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
	colors[ImGuiCol_TextSelectedBg] = ImVec4(0.26f, 0.59f, 0.98f, 0.35f);
	colors[ImGuiCol_DragDropTarget] = ImVec4(0.11f, 0.64f, 0.92f, 1.00f);
	colors[ImGuiCol_NavHighlight] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
	colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
	colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
	colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.35f);

	colors[ImGuiCol_WindowBg] = background;
	colors[ImGuiCol_ChildBg] = background;

	colors[ImGuiCol_Border] = border;
	colors[ImGuiCol_Separator] = border;

	colors[ImGuiCol_TextDisabled] = ImVec4(1.0f, 1.0f, 1.0f, disabled_alpha);

	colors[ImGuiCol_FrameBg] = ImVec4(0.2f, 0.2f, 0.2f, bg_alpha);
	colors[ImGuiCol_FrameBgHovered] = colors[ImGuiCol_FrameBg];
	colors[ImGuiCol_FrameBgActive] = colors[ImGuiCol_FrameBg];

	colors[ImGuiCol_SliderGrab] = ImVec4(1.0f, 1.0f, 1.0f, 0.245f);
	colors[ImGuiCol_SliderGrabActive] = ImVec4(1.0f, 1.0f, 1.0f, 0.531f);

	colors[ImGuiCol_CheckMark] = ImVec4(1.0f, 1.0f, 1.0f, 0.9f);

	style.WindowBorderSize = 3.0f;
	style.TabRounding = 0.0f;
}

bool IsEnabled = false;

Menu::~Menu()
{
	ImGui_ImplDX11_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();
}

void Menu::Load(json& o_json)
{
	if (o_json[SETTING_MENU_TOGGLEKEY].is_number_unsigned()) {
		toggleKey = o_json[SETTING_MENU_TOGGLEKEY];
	}
}

void Menu::Save(json& o_json)
{
	json menu;
	menu[SETTING_MENU_TOGGLEKEY] = toggleKey;

	o_json["Menu"] = menu;
}

#define IM_VK_KEYPAD_ENTER (VK_RETURN + 256)
RE::BSEventNotifyControl Menu::ProcessEvent(RE::InputEvent* const* a_event, RE::BSTEventSource<RE::InputEvent*>* a_eventSource)
{
	if (!a_event || !a_eventSource)
		return RE::BSEventNotifyControl::kContinue;

	auto& io = ImGui::GetIO();

	for (auto event = *a_event; event; event = event->next) {
		if (event->eventType == RE::INPUT_EVENT_TYPE::kChar) {
			io.AddInputCharacter(event->AsCharEvent()->keycode);
		} else if (event->eventType == RE::INPUT_EVENT_TYPE::kButton) {
			const auto button = static_cast<RE::ButtonEvent*>(event);
			if (!button || (button->IsPressed() && !button->IsDown()))
				continue;

			auto     scan_code = button->GetIDCode();
			uint32_t key = MapVirtualKeyEx(scan_code, MAPVK_VSC_TO_VK_EX, GetKeyboardLayout(0));

			switch (scan_code) {
			case DIK_LEFTARROW:
				key = VK_LEFT;
				break;
			case DIK_RIGHTARROW:
				key = VK_RIGHT;
				break;
			case DIK_UPARROW:
				key = VK_UP;
				break;
			case DIK_DOWNARROW:
				key = VK_DOWN;
				break;
			case DIK_DELETE:
				key = VK_DELETE;
				break;
			case DIK_END:
				key = VK_END;
				break;
			case DIK_HOME:
				key = VK_HOME;
				break;  // pos1
			case DIK_PRIOR:
				key = VK_PRIOR;
				break;  // page up
			case DIK_NEXT:
				key = VK_NEXT;
				break;  // page down
			case DIK_INSERT:
				key = VK_INSERT;
				break;
			case DIK_NUMPAD0:
				key = VK_NUMPAD0;
				break;
			case DIK_NUMPAD1:
				key = VK_NUMPAD1;
				break;
			case DIK_NUMPAD2:
				key = VK_NUMPAD2;
				break;
			case DIK_NUMPAD3:
				key = VK_NUMPAD3;
				break;
			case DIK_NUMPAD4:
				key = VK_NUMPAD4;
				break;
			case DIK_NUMPAD5:
				key = VK_NUMPAD5;
				break;
			case DIK_NUMPAD6:
				key = VK_NUMPAD6;
				break;
			case DIK_NUMPAD7:
				key = VK_NUMPAD7;
				break;
			case DIK_NUMPAD8:
				key = VK_NUMPAD8;
				break;
			case DIK_NUMPAD9:
				key = VK_NUMPAD9;
				break;
			case DIK_DECIMAL:
				key = VK_DECIMAL;
				break;
			case DIK_NUMPADENTER:
				key = IM_VK_KEYPAD_ENTER;
				break;
			case DIK_LMENU:
				key = VK_LMENU;
				break;  // left alt
			case DIK_LCONTROL:
				key = VK_LCONTROL;
				break;  // left control
			case DIK_LSHIFT:
				key = VK_LSHIFT;
				break;  // left shift
			case DIK_RMENU:
				key = VK_RMENU;
				break;  // right alt
			case DIK_RCONTROL:
				key = VK_RCONTROL;
				break;  // right control
			case DIK_RSHIFT:
				key = VK_RSHIFT;
				break;  // right shift
			case DIK_LWIN:
				key = VK_LWIN;
				break;  // left win
			case DIK_RWIN:
				key = VK_RWIN;
				break;  // right win
			case DIK_APPS:
				key = VK_APPS;
				break;
			default:
				break;
			}

			switch (button->device.get()) {
			case RE::INPUT_DEVICE::kKeyboard:
				if (!button->IsPressed()) {
					if (settingToggleKey) {
						toggleKey = key;
						settingToggleKey = false;
					} else if (key == toggleKey) {
						IsEnabled = !IsEnabled;
					}
				}

				io.AddKeyEvent(VirtualKeyToImGuiKey(key), button->IsPressed());

				if (key == VK_LCONTROL || key == VK_RCONTROL)
					io.AddKeyEvent(ImGuiKey_ModCtrl, button->IsPressed());
				else if (key == VK_LSHIFT || key == VK_RSHIFT)
					io.AddKeyEvent(ImGuiKey_ModShift, button->IsPressed());
				else if (key == VK_LMENU || key == VK_RMENU)
					io.AddKeyEvent(ImGuiKey_ModAlt, button->IsPressed());
				break;
			case RE::INPUT_DEVICE::kMouse:
				logger::trace("Detect mouse scan code {} value {} pressed: {}", scan_code, button->Value(), button->IsPressed());
				if (scan_code > 7)  // middle scroll
					io.AddMouseWheelEvent(0, button->Value() * (scan_code == 8 ? 1 : -1));
				else {
					if (scan_code > 5)
						scan_code = 5;
					io.AddMouseButtonEvent(scan_code, button->IsPressed());
				}
				break;
			default:
				continue;
			}
			if (const auto controlMap = RE::ControlMap::GetSingleton()) {
				controlMap->GetRuntimeData().ignoreKeyboardMouse = IsEnabled;
			}
		}
	}

	return RE::BSEventNotifyControl::kContinue;

	return RE::BSEventNotifyControl::kContinue;
}

void Menu::Init(IDXGISwapChain* swapchain, ID3D11Device* device, ID3D11DeviceContext* context)
{
	// Setup Dear ImGui context
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	auto& imgui_io = ImGui::GetIO();

	imgui_io.ConfigFlags = ImGuiConfigFlags_NavEnableKeyboard;
	imgui_io.BackendFlags = ImGuiBackendFlags_HasMouseCursors | ImGuiBackendFlags_RendererHasVtxOffset;

	imgui_io.Fonts->AddFontFromFileTTF("Data\\Interface\\CommunityShaders\\Fonts\\Jost-Regular.ttf", 24);

	DXGI_SWAP_CHAIN_DESC desc;
	swapchain->GetDesc(&desc);

	// Setup Platform/Renderer backends
	ImGui_ImplWin32_Init(desc.OutputWindow);
	ImGui_ImplDX11_Init(device, context);
}

void Menu::DrawSettings()
{
	ImGuiStyle oldStyle = ImGui::GetStyle();
	SetupImGuiStyle();
	static bool visible = false;

	ImGui::SetNextWindowSize({ 1000, 1000 }, ImGuiCond_Once);
	ImGui::SetNextWindowPos(ImVec2(0, 0));
	ImGui::Begin(std::format("TressFXSSE {}", Plugin::VERSION.string(".")).c_str(), &IsEnabled);
	if (ImGui::Button("Reload hairs")) {
		SkyrimTressFX::GetSingleton()->m_doReload = true;
	}
	const char* drawingControl[] = { "PPLL", "ShortCut" };
	static int  drawingControlSelected = 0;
	int         oldDrawingControlSelected = drawingControlSelected;
	ImGui::Combo("Drawing Method", &drawingControlSelected, drawingControl, _countof(drawingControl));
	if (drawingControlSelected != oldDrawingControlSelected) {
		SkyrimTressFX::GetSingleton()->ToggleShortCut();
	}
	DrawHairSelector();
	if (ImGui::CollapsingHeader("Offsets")) {
		DrawOffsetSliders();
	}
	if (ImGui::CollapsingHeader("Hair params")) {
		DrawHairParams();
	}
	ImGui::SliderFloat("Sunlight scale", &sunlightScaleSlider, 0.0f, 20.0f);
	ImGui::SliderFloat("Ambient flat shading scale", &ambientFlatShadingScaleSlider, 0.0f, 20.0f);
	ImGui::SliderFloat("Directional ambient light scale", &directionalAmbientLightScaleSlider, 0.0f, 20.0f);
	ImGui::Checkbox("Draw hair", &drawHairCheckbox);
	ImGui::Checkbox("Draw shadows", &drawShadowsCheckbox);
	ImGui::Checkbox("Community shaders shadows", &communityShadersScreenSpaceShadowsCheckbox);
	ImGui::Checkbox("Draw debug markers", &drawDebugMarkersCheckbox);
	DrawQueues();


	ImGui::End();
	ImGuiStyle& style = ImGui::GetStyle();
	style = oldStyle;

	ImGui::Render();
	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
}
void Menu::DrawHairParams() {
	ImGui::SliderFloat("Fiber Radius", &fiberRadiusSliderValue, 0.0f, 1.0f);
	ImGui::SliderFloat("Fiber Spacing", &fiberSpacingSliderValue, 0.0f, 1.0f);
	ImGui::SliderFloat("Fiber Ratio", &fiberRatioSliderValue, 0.0f, 1.0f);
	ImGui::SliderFloat("Kd", &kdSliderValue, 0.0f, 1.0f);
	ImGui::SliderFloat("Ks1", &ks1SliderValue, 0.0f, 1.0f);
	ImGui::SliderFloat("Kx1", &ex1SliderValue, 0.0f, 20.0f);
	ImGui::SliderFloat("Ks2", &ks2SliderValue, 0.0f, 1.0f);
	ImGui::SliderFloat("Ex2", &ex2SliderValue, 0.0f, 20.0f);
	ImGui::SliderInt("Local constraints iter.", &localConstraintsIterationsSlider, 0, 20);
	ImGui::SliderInt("Length constraints iter.", &lengthConstraintsIterationsSlider, 1, 20);
	ImGui::SliderFloat("Local constraints stiffness", &localConstraintsStiffnessSlider, 0.0f, 5.0f);
	ImGui::SliderFloat("Global constraints stiffness", &globalConstraintsStiffnessSlider, 0.0f, 1.0f);
	ImGui::SliderFloat("Global constraints range", &globalConstraintsRangeSlider, 0.0f, 1.0f);
	ImGui::SliderFloat("Damping", &dampingSlider, 0.0f, 1.0f);
	ImGui::SliderFloat("VSP amount", &vspAmountSlider, 0.0f, 1.0f);
	ImGui::SliderFloat("VSP accel. threshold", &vspAccelThresholdSlider, 0.0f, 10.0f);
	ImGui::SliderFloat("Gravity magnitude", &gravityMagnitudeSlider, 0.0f, 1.0f);
	ImGui::SliderFloat("Hair opacity", &hairOpacitySlider, 0.0f, 1.0f);
	ImGui::SliderFloat("Hair shadow alpha", &hairShadowAlphaSlider, 0.0f, 1.0f);
	ImGui::Checkbox("Thin tip", &thinTipCheckbox);
	if (ImGui::Button("Export parameters")) {
		SkyrimTressFX::GetSingleton()->GetHairByName(activeHairs[selectedHair])->ExportParameters();
	}
}

void Menu::DrawHairSelector()
{
	if (ImGui::BeginCombo("Select actor", activeActors[selectedActor].c_str())) {
		for (uint32_t i = 0; i < activeActors.size(); i++) {
			bool isSelected = (selectedActor == i);
			if (ImGui::Selectable(activeActors[i].c_str(), isSelected)) {
				selectedActor = i;
			}

			// Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
			if (isSelected) {
				ImGui::SetItemDefaultFocus();
			}
		}

		ImGui::EndCombo();
	}

	if (ImGui::BeginCombo("Select hair", activeHairs[selectedHair].c_str())) {
		for (uint32_t i = 0; i < activeHairs.size(); i++) {
			bool isSelected = (selectedHair == i);
			if (ImGui::Selectable(activeHairs[i].c_str(), isSelected)) {
				selectedHair = i;
			}

			// Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
			if (isSelected) {
				ImGui::SetItemDefaultFocus();
			}
		}

		ImGui::EndCombo();
	}
}
void Menu::DrawOffsetSliders()
{
	offsetSlidersUpdated = false;
	ImGui::SliderFloat("X", &xSliderValue, -20.0f, 20.0f);
	if (!(fabs(xSliderValue - lastXSliderValue) < std::numeric_limits<float>::epsilon())) {
		lastXSliderValue = xSliderValue;
		offsetSlidersUpdated = true;
	}
	ImGui::SliderFloat("Y", &ySliderValue, -20.0f, 20.0f);
	if (!(fabs(ySliderValue - lastYSliderValue) < std::numeric_limits<float>::epsilon())) {
		lastYSliderValue = ySliderValue;
		offsetSlidersUpdated = true;
	}
	ImGui::SliderFloat("Z", &zSliderValue, -20.0f, 20.0f);
	if (!(fabs(zSliderValue - lastZSliderValue) < std::numeric_limits<float>::epsilon())) {
		lastZSliderValue = zSliderValue;
		offsetSlidersUpdated = true;
	}
	ImGui::SliderFloat("SCALE", &sSliderValue, 0.2f, 10.0f);
	if (!(fabs(sSliderValue - lastSSliderValue) < std::numeric_limits<float>::epsilon())) {
		lastSSliderValue = sSliderValue;
		offsetSlidersUpdated = true;
	}
	if (ImGui::Button("Export offsets")) {
		for (auto& hair : SkyrimTressFX::GetSingleton()->m_activeScene.objects) {
			if (hair.name == activeHairs[selectedHair]) {
				hair.hairStrands.get()->ExportOffsets(xSliderValue, ySliderValue, zSliderValue, sSliderValue);
			}
		}
	}
}
void Menu::DrawQueues()
{
	int i = 0;
	for (glm::mat4 mat : matrices) {
		ImGui::BeginTable("matrix", 4);
		// Display the table headers
		for (int j = 0; j < 4; ++j) {
			ImGui::TableSetupColumn((matrixNames[i]).c_str());
		}
		ImGui::TableHeadersRow();

		//Display the table data
		for (int row = 0; row < 4; ++row) {
			ImGui::TableNextRow();
			for (int col = 0; col < 4; ++col) {
				ImGui::TableSetColumnIndex(col);
				ImGui::Text(std::to_string(mat[row][col]).c_str());
			}
		}
		ImGui::EndTable();
		i += 1;
	}
	matrices.clear();
	matrixNames.clear();

	int k = 0;
	for (glm::vec3 vec : vector3s) {
		ImGui::BeginTable(vector3Names[k].c_str(), 4);
		// Display the table headers
		for (int j = 0; j < 3; ++j) {
			ImGui::TableSetupColumn(vector3Names[k].c_str());
		}
		ImGui::TableHeadersRow();
		ImGui::TableNextRow();
		for (int col = 0; col < 3; ++col) {
			ImGui::TableSetColumnIndex(col);
			ImGui::Text(std::to_string(vec[col]).c_str());
		}
		ImGui::EndTable();
		k += 1;
	}
	vector3s.clear();
	vector3Names.clear();

	ImGui::Spacing();
	int j = 0;
	for (float val : floats) {
		ImGui::Text(floatNames[j].c_str());
		ImGui::Text(std::to_string(val).c_str());
		ImGui::Spacing();
		j += 1;
	}
	floats.clear();
	floatNames.clear();

	int l = 0;
	for (float val : ints) {
		ImGui::Text(intNames[j].c_str());
		ImGui::Text(std::to_string(val).c_str());
		ImGui::Spacing();
		l += 1;
	}
	ints.clear();
	intNames.clear();

	int m = 0;
	for (RE::NiTransform transform: transforms){
		ImGui::BeginTable(vector3Names[k].c_str(), 4);
		// Display the table headers
		for (int n = 0; n < 3; ++n) {
			ImGui::TableSetupColumn(transformNames[m].c_str());
		}
		ImGui::TableHeadersRow();
		ImGui::TableNextRow();
		for (int col = 0; col < 3; ++col) {
			ImGui::TableSetColumnIndex(col);
			ImGui::Text(std::to_string(transform.translate[col]).c_str());
		}
		ImGui::EndTable();

		ImGui::BeginTable("rotation", 4);
		// Display the table headers
		for (int n = 0; n < 4; ++n) {
			ImGui::TableSetupColumn((transformNames[m]).c_str());
		}
		ImGui::TableHeadersRow();

		//Display the table data
		for (int row = 0; row < 4; ++row) {
			ImGui::TableNextRow();
			for (int col = 0; col < 4; ++col) {
				ImGui::TableSetColumnIndex(col);
				ImGui::Text(std::to_string(transform.rotate.entry[row][col]).c_str());
			}
		}
		ImGui::EndTable();

		m += 1;
	}
	transforms.clear();
	transformNames.clear();
}

void Menu::SetActiveActors(std::vector<std::string> actors) {
	activeActors = actors;
}

void Menu::UpdateActiveHairs(std::vector<std::string> hairs){
	activeHairs = hairs;
}

std::string Menu::GetSelectedHair() {
	return activeHairs[selectedHair];
}

void Menu::DrawOverlay()
{
	// Start the Dear ImGui frame
	ImGui_ImplDX11_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

	uint64_t totalShaders = 0;
	uint64_t compiledShaders = 0;


	if (compiledShaders != totalShaders) {
		ImGui::SetNextWindowBgAlpha(1);
		ImGui::SetNextWindowPos(ImVec2(10, 10));
		if (!ImGui::Begin("ShaderCompilationInfo", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings)) {
			ImGui::End();
			return;
		}

		ImGui::Text("Compiling Shaders: %d / %d", compiledShaders, totalShaders);

		ImGui::End();
	}

	if (IsEnabled) {
		ImGui::GetIO().MouseDrawCursor = true;
		DrawSettings();
	} else {
		ImGui::GetIO().MouseDrawCursor = false;
	}

	ImGui::Render();
	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
}
void Menu::DrawVector3(glm::vec3 vec, std::string name)
{
	vector3s.push_back(vec);
	vector3Names.push_back(name);
}
void Menu::DrawVector3(RE::NiPoint3 vec, std::string name)
{
	glm::vec3 glmvec = glm::vec3(vec.x, vec.y, vec.z);
	vector3s.push_back(glmvec);
	vector3Names.push_back(name);
}

void Menu::DrawFloat(float val, std::string name) {
	floats.push_back(val);
	floatNames.push_back(name);
}
void Menu::DrawInt(uint16_t integer, std::string name) {
	ints.push_back(integer);
	intNames.push_back(name);
}
void Menu::DrawMatrix(glm::mat4 mat, std::string name)
{
	matrices.push_back(mat);
	matrixNames.push_back(name);
}
void Menu::DrawNiTransform(RE::NiTransform transform, std::string name) {
	transforms.push_back(transform);
	transformNames.push_back(name);
}
void Menu::DrawMatrix(RE::NiMatrix3 mat, std::string name)
{
	glm::mat4 glmmat = glm::mat4({ mat.entry[0][0], mat.entry[0][1], mat.entry[0][2], 0 }, 
								 { mat.entry[1][0], mat.entry[1][1], mat.entry[1][2], 0 }, 
								 { mat.entry[2][0], mat.entry[2][1], mat.entry[2][2], 0 }, 
								 { 0, 0, 0, 1 });
	DrawMatrix(glmmat, name);
}
void Menu::DrawMatrix(DirectX::XMMATRIX mat, std::string name)
{
	DirectX::XMFLOAT4X4 temp;
	DirectX::XMStoreFloat4x4(&temp, mat);
	glm::mat4 glmmat = glm::mat4({ temp._11, temp._12, temp._13, temp._14 },
		{ temp._21, temp._22, temp._23, temp._24 },
		{ temp._31, temp._32, temp._33, temp._34 },
		{ temp._41, temp._42, temp._43, temp._44 });
	DrawMatrix(glmmat, name);
}
TressFXRenderingSettings Menu::GetSelectedRenderingSettings(TressFXRenderingSettings& previousSettings)
{
	TressFXRenderingSettings settings;
	settings.m_BaseAlbedoName = previousSettings.m_BaseAlbedoName;
	settings.m_EnableHairLOD = previousSettings.m_EnableHairLOD;
	settings.m_EnableShadowLOD = previousSettings.m_EnableShadowLOD;
	settings.m_EnableStrandTangent = previousSettings.m_EnableStrandTangent;
	settings.m_EnableStrandUV = previousSettings.m_EnableStrandUV;
	settings.m_EnableThinTip = thinTipCheckbox;
	settings.m_FiberRadius = fiberRadiusSliderValue;
	settings.m_FiberRatio = fiberRatioSliderValue;
	settings.m_HairFiberSpacing = previousSettings.m_HairFiberSpacing;
	settings.m_HairKDiffuse = kdSliderValue;
	settings.m_HairKSpec1 = ks1SliderValue;
	settings.m_HairKSpec2 = ks2SliderValue;
	settings.m_HairMatBaseColor = previousSettings.m_HairMatBaseColor;
	settings.m_HairMatTipColor = previousSettings.m_HairMatTipColor;
	settings.m_HairMaxShadowFibers = previousSettings.m_HairMaxShadowFibers;
	settings.m_HairShadowAlpha = hairShadowAlphaSlider;
	settings.m_HairSpecExp1 = ex1SliderValue;
	settings.m_HairSpecExp2 = ex2SliderValue;
	settings.m_LODEndDistance = previousSettings.m_LODEndDistance;
	settings.m_LODPercent = previousSettings.m_LODPercent;
	settings.m_LODStartDistance = previousSettings.m_LODStartDistance;
	settings.m_LODWidthMultiplier = previousSettings.m_LODWidthMultiplier;
	settings.m_ShadowLODEndDistance = previousSettings.m_ShadowLODEndDistance;
	settings.m_ShadowLODPercent = previousSettings.m_ShadowLODPercent;
	settings.m_ShadowLODStartDistance = previousSettings.m_ShadowLODStartDistance;
	settings.m_ShadowLODWidthMultiplier = previousSettings.m_ShadowLODWidthMultiplier;
	settings.m_StrandAlbedoName = previousSettings.m_StrandAlbedoName;
	settings.m_StrandUVTilingFactor = previousSettings.m_StrandUVTilingFactor;
	settings.m_TipPercentage = previousSettings.m_TipPercentage;
	return settings;
}

TressFXSimulationSettings Menu::GetSelectedSimulationSettings(TressFXSimulationSettings& previousSettings) {
	TressFXSimulationSettings settings;
	settings.m_clampPositionDelta = previousSettings.m_clampPositionDelta;
	settings.m_damping = dampingSlider;
	settings.m_globalConstraintsRange = globalConstraintsRangeSlider;
	settings.m_globalConstraintStiffness = globalConstraintsStiffnessSlider;
	settings.m_gravityMagnitude = gravityMagnitudeSlider;
	settings.m_lengthConstraintsIterations = lengthConstraintsIterationsSlider;
	settings.m_localConstraintsIterations = localConstraintsIterationsSlider;
	settings.m_localConstraintStiffness = localConstraintsStiffnessSlider;
	settings.m_tipSeparation = previousSettings.m_tipSeparation;
	settings.m_vspAccelThreshold = vspAccelThresholdSlider;
	settings.m_vspCoeff = vspAmountSlider;
	settings.m_windAngleRadians = previousSettings.m_windAngleRadians;
	settings.m_windDirection[0] = previousSettings.m_windDirection[0];
	settings.m_windDirection[1] = previousSettings.m_windDirection[1];
	settings.m_windDirection[2] = previousSettings.m_windDirection[2];
	settings.m_windMagnitude = previousSettings.m_windMagnitude;
	return settings;
}
	const char* Menu::KeyIdToString(uint32_t key)
{
	if (key >= 256)
		return "";

	static const char* keyboard_keys_international[256] = {
		"", "Left Mouse", "Right Mouse", "Cancel", "Middle Mouse", "X1 Mouse", "X2 Mouse", "", "Backspace", "Tab", "", "", "Clear", "Enter", "", "",
		"Shift", "Control", "Alt", "Pause", "Caps Lock", "", "", "", "", "", "", "Escape", "", "", "", "",
		"Space", "Page Up", "Page Down", "End", "Home", "Left Arrow", "Up Arrow", "Right Arrow", "Down Arrow", "Select", "", "", "Print Screen", "Insert", "Delete", "Help",
		"0", "1", "2", "3", "4", "5", "6", "7", "8", "9", "", "", "", "", "", "",
		"", "A", "B", "C", "D", "E", "F", "G", "H", "I", "J", "K", "L", "M", "N", "O",
		"P", "Q", "R", "S", "T", "U", "V", "W", "X", "Y", "Z", "Left Windows", "Right Windows", "Apps", "", "Sleep",
		"Numpad 0", "Numpad 1", "Numpad 2", "Numpad 3", "Numpad 4", "Numpad 5", "Numpad 6", "Numpad 7", "Numpad 8", "Numpad 9", "Numpad *", "Numpad +", "", "Numpad -", "Numpad Decimal", "Numpad /",
		"F1", "F2", "F3", "F4", "F5", "F6", "F7", "F8", "F9", "F10", "F11", "F12", "F13", "F14", "F15", "F16",
		"F17", "F18", "F19", "F20", "F21", "F22", "F23", "F24", "", "", "", "", "", "", "", "",
		"Num Lock", "Scroll Lock", "", "", "", "", "", "", "", "", "", "", "", "", "", "",
		"Left Shift", "Right Shift", "Left Control", "Right Control", "Left Menu", "Right Menu", "Browser Back", "Browser Forward", "Browser Refresh", "Browser Stop", "Browser Search", "Browser Favorites", "Browser Home", "Volume Mute", "Volume Down", "Volume Up",
		"Next Track", "Previous Track", "Media Stop", "Media Play/Pause", "Mail", "Media Select", "Launch App 1", "Launch App 2", "", "", "OEM ;", "OEM +", "OEM ,", "OEM -", "OEM .", "OEM /",
		"OEM ~", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "",
		"", "", "", "", "", "", "", "", "", "", "", "OEM [", "OEM \\", "OEM ]", "OEM '", "OEM 8",
		"", "", "OEM <", "", "", "", "", "", "", "", "", "", "", "", "", "",
		"", "", "", "", "", "", "Attn", "CrSel", "ExSel", "Erase EOF", "Play", "Zoom", "", "PA1", "OEM Clear", ""
	};

	return keyboard_keys_international[key];
}

#define IM_VK_KEYPAD_ENTER (VK_RETURN + 256)
const ImGuiKey Menu::VirtualKeyToImGuiKey(WPARAM vkKey)
{
	switch (vkKey) {
	case VK_TAB:
		return ImGuiKey_Tab;
	case VK_LEFT:
		return ImGuiKey_LeftArrow;
	case VK_RIGHT:
		return ImGuiKey_RightArrow;
	case VK_UP:
		return ImGuiKey_UpArrow;
	case VK_DOWN:
		return ImGuiKey_DownArrow;
	case VK_PRIOR:
		return ImGuiKey_PageUp;
	case VK_NEXT:
		return ImGuiKey_PageDown;
	case VK_HOME:
		return ImGuiKey_Home;
	case VK_END:
		return ImGuiKey_End;
	case VK_INSERT:
		return ImGuiKey_Insert;
	case VK_DELETE:
		return ImGuiKey_Delete;
	case VK_BACK:
		return ImGuiKey_Backspace;
	case VK_SPACE:
		return ImGuiKey_Space;
	case VK_RETURN:
		return ImGuiKey_Enter;
	case VK_ESCAPE:
		return ImGuiKey_Escape;
	case VK_OEM_7:
		return ImGuiKey_Apostrophe;
	case VK_OEM_COMMA:
		return ImGuiKey_Comma;
	case VK_OEM_MINUS:
		return ImGuiKey_Minus;
	case VK_OEM_PERIOD:
		return ImGuiKey_Period;
	case VK_OEM_2:
		return ImGuiKey_Slash;
	case VK_OEM_1:
		return ImGuiKey_Semicolon;
	case VK_OEM_PLUS:
		return ImGuiKey_Equal;
	case VK_OEM_4:
		return ImGuiKey_LeftBracket;
	case VK_OEM_5:
		return ImGuiKey_Backslash;
	case VK_OEM_6:
		return ImGuiKey_RightBracket;
	case VK_OEM_3:
		return ImGuiKey_GraveAccent;
	case VK_CAPITAL:
		return ImGuiKey_CapsLock;
	case VK_SCROLL:
		return ImGuiKey_ScrollLock;
	case VK_NUMLOCK:
		return ImGuiKey_NumLock;
	case VK_SNAPSHOT:
		return ImGuiKey_PrintScreen;
	case VK_PAUSE:
		return ImGuiKey_Pause;
	case VK_NUMPAD0:
		return ImGuiKey_Keypad0;
	case VK_NUMPAD1:
		return ImGuiKey_Keypad1;
	case VK_NUMPAD2:
		return ImGuiKey_Keypad2;
	case VK_NUMPAD3:
		return ImGuiKey_Keypad3;
	case VK_NUMPAD4:
		return ImGuiKey_Keypad4;
	case VK_NUMPAD5:
		return ImGuiKey_Keypad5;
	case VK_NUMPAD6:
		return ImGuiKey_Keypad6;
	case VK_NUMPAD7:
		return ImGuiKey_Keypad7;
	case VK_NUMPAD8:
		return ImGuiKey_Keypad8;
	case VK_NUMPAD9:
		return ImGuiKey_Keypad9;
	case VK_DECIMAL:
		return ImGuiKey_KeypadDecimal;
	case VK_DIVIDE:
		return ImGuiKey_KeypadDivide;
	case VK_MULTIPLY:
		return ImGuiKey_KeypadMultiply;
	case VK_SUBTRACT:
		return ImGuiKey_KeypadSubtract;
	case VK_ADD:
		return ImGuiKey_KeypadAdd;
	case IM_VK_KEYPAD_ENTER:
		return ImGuiKey_KeypadEnter;
	case VK_LSHIFT:
		return ImGuiKey_LeftShift;
	case VK_LCONTROL:
		return ImGuiKey_LeftCtrl;
	case VK_LMENU:
		return ImGuiKey_LeftAlt;
	case VK_LWIN:
		return ImGuiKey_LeftSuper;
	case VK_RSHIFT:
		return ImGuiKey_RightShift;
	case VK_RCONTROL:
		return ImGuiKey_RightCtrl;
	case VK_RMENU:
		return ImGuiKey_RightAlt;
	case VK_RWIN:
		return ImGuiKey_RightSuper;
	case VK_APPS:
		return ImGuiKey_Menu;
	case '0':
		return ImGuiKey_0;
	case '1':
		return ImGuiKey_1;
	case '2':
		return ImGuiKey_2;
	case '3':
		return ImGuiKey_3;
	case '4':
		return ImGuiKey_4;
	case '5':
		return ImGuiKey_5;
	case '6':
		return ImGuiKey_6;
	case '7':
		return ImGuiKey_7;
	case '8':
		return ImGuiKey_8;
	case '9':
		return ImGuiKey_9;
	case 'A':
		return ImGuiKey_A;
	case 'B':
		return ImGuiKey_B;
	case 'C':
		return ImGuiKey_C;
	case 'D':
		return ImGuiKey_D;
	case 'E':
		return ImGuiKey_E;
	case 'F':
		return ImGuiKey_F;
	case 'G':
		return ImGuiKey_G;
	case 'H':
		return ImGuiKey_H;
	case 'I':
		return ImGuiKey_I;
	case 'J':
		return ImGuiKey_J;
	case 'K':
		return ImGuiKey_K;
	case 'L':
		return ImGuiKey_L;
	case 'M':
		return ImGuiKey_M;
	case 'N':
		return ImGuiKey_N;
	case 'O':
		return ImGuiKey_O;
	case 'P':
		return ImGuiKey_P;
	case 'Q':
		return ImGuiKey_Q;
	case 'R':
		return ImGuiKey_R;
	case 'S':
		return ImGuiKey_S;
	case 'T':
		return ImGuiKey_T;
	case 'U':
		return ImGuiKey_U;
	case 'V':
		return ImGuiKey_V;
	case 'W':
		return ImGuiKey_W;
	case 'X':
		return ImGuiKey_X;
	case 'Y':
		return ImGuiKey_Y;
	case 'Z':
		return ImGuiKey_Z;
	case VK_F1:
		return ImGuiKey_F1;
	case VK_F2:
		return ImGuiKey_F2;
	case VK_F3:
		return ImGuiKey_F3;
	case VK_F4:
		return ImGuiKey_F4;
	case VK_F5:
		return ImGuiKey_F5;
	case VK_F6:
		return ImGuiKey_F6;
	case VK_F7:
		return ImGuiKey_F7;
	case VK_F8:
		return ImGuiKey_F8;
	case VK_F9:
		return ImGuiKey_F9;
	case VK_F10:
		return ImGuiKey_F10;
	case VK_F11:
		return ImGuiKey_F11;
	case VK_F12:
		return ImGuiKey_F12;
	default:
		return ImGuiKey_None;
	};
}
