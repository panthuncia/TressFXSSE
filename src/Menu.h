#pragma once

#include "imgui.h"
#include "imgui_impl_dx11.h"
#include "imgui_impl_win32.h"
#include <nlohmann/json.hpp>
#include <vector>
#include <string>
#include <glm/glm.hpp>
using json = nlohmann::json;
class TressFXRenderingSettings;
class TressFXSimulationSettings;
class Menu : public RE::BSTEventSink<RE::InputEvent*>
{
public:
	~Menu();

	static Menu* GetSingleton()
	{
		static Menu menu;
		return &menu;
	}

	void Load(json& o_json);
	void Save(json& o_json);

	RE::BSEventNotifyControl ProcessEvent(RE::InputEvent* const* a_event,
		RE::BSTEventSource<RE::InputEvent*>*                     a_eventSource) override;

	void Init(IDXGISwapChain* swapchain, ID3D11Device* device, ID3D11DeviceContext* context);
	void DrawSettings();
	void DrawOverlay();
	void SetActiveActors(std::vector<std::string>);
	std::string GetSelectedHair();
	void DrawMatrix(glm::mat4 mat, std::string name);
	void DrawMatrix(RE::NiMatrix3 mat, std::string name);
	void DrawMatrix(DirectX::XMMATRIX mat, std::string name);
	void DrawVector3(glm::vec3 mat, std::string name);
	void DrawVector3(RE::NiPoint3 mat, std::string name);
	void DrawFloat(float mat, std::string name);
	void DrawInt(uint16_t integer, std::string name);
	void DrawNiTransform(RE::NiTransform transform, std::string name);
	void UpdateActiveHairs(std::vector<std::string> actors);
	TressFXRenderingSettings GetSelectedRenderingSettings(TressFXRenderingSettings& previousSettings);
	TressFXSimulationSettings GetSelectedSimulationSettings(TressFXSimulationSettings& previousSettings);
	void SetCurrentSliders(TressFXRenderingSettings renderSettings, TressFXSimulationSettings simSettings, float offsets[4]);
	
	//hair offsets
	float xSliderValue = 0.0f;
	float ySliderValue = 0.0f;
	float zSliderValue = 0.0f;
	float sSliderValue = 1.0f;
	float lastXSliderValue = xSliderValue;
	float lastYSliderValue = ySliderValue;
	float lastZSliderValue = zSliderValue;
	float lastSSliderValue = sSliderValue;
	bool                     offsetSlidersUpdated = false;

	//hair render and sim parameters
	float                    fiberRadiusSliderValue = 0.02f;
	float                    tipSeparationSliderValue = 0.1f;
	float                    fiberRatioSliderValue = 0.5f;
	float                    kdSliderValue = 0.07f;
	float                    ks1SliderValue = 0.17f;
	float                    ex1SliderValue = 14.4f;
	float                    ks2SliderValue = 0.72f;
	float                    ex2SliderValue = 11.8f;
	int                      localConstraintsIterationsSlider = 3;
	int                      lengthConstraintsIterationsSlider = 3;
	float                    localConstraintsStiffnessSlider = 0.9f;
	float                    globalConstraintsStiffnessSlider = 0.4f;
	float                    globalConstraintsRangeSlider = 0.3f;
	float                    dampingSlider = 0.06f;
	float                    vspAmountSlider = 0.75f;
	float                    vspAccelThresholdSlider = 1.2f;
	float                    hairOpacitySlider = 0.63f;
	float                    hairShadowAlphaSlider = 0.35f;
	bool                     thinTipCheckbox = true;
	bool                     drawShadowsCheckbox = true;
	bool                     communityShadersScreenSpaceShadowsCheckbox = true;
	bool                     drawHairCheckbox = true;
	float                    gravityMagnitudeSlider = 0.09f;
	bool                     drawDebugMarkersCheckbox = true;
	float                    sunlightScaleSlider = 1.0;
	float                    directionalAmbientLightScaleSlider = 1.0;
	float                    ambientFlatShadingScaleSlider = 1.0;

	//needed because IMGUI sliders are low-res.
	uint8_t                  fiberRadiusScale = 100;
	std::vector<std::string> activeActors = { "PLAYER" };
	uint32_t                 selectedActor = 0;
	std::vector<std::string> activeHairs = { "NONE" };
	int                 lastSelectedHair = -1;
	int                 selectedHair = 0;


private:
	uint32_t toggleKey = VK_DELETE;
	bool     settingToggleKey = false;
	Menu() {}
	void DrawQueues();
	void DrawOffsetSliders();
	void           DrawHairSelector();
	void           DrawHairParams();
	const char*    KeyIdToString(uint32_t key);
	const ImGuiKey VirtualKeyToImGuiKey(WPARAM vkKey);

	std::vector<glm::mat4> matrices;
	std::vector<std::string> matrixNames;

	std::vector<glm::vec3>   vector3s;
	std::vector<std::string> vector3Names;

	std::vector<float> floats;
	std::vector<std::string> floatNames;

	std::vector<uint16_t>       ints;
	std::vector<std::string> intNames;

	std::vector<RE::NiTransform>    transforms;
	std::vector<std::string> transformNames;
};
