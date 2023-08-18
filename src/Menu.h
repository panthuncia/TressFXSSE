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
	
	float ambientLightingAmount = 1.0f;
	float pointLightDiffuseAmount = 1.0f;
	float sunLightDiffuseAmount = 1.0f;
	float pointLightSpecularAmount = 1.0f;
	float sunLightSpecularAmount = 1.0f;

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
	float                    fiberRadiusSliderValue = 0.002f;
	float                    fiberSpacingSliderValue = 0.1f;
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
	bool                     HBAOCheckbox = true;
	bool                     clearBeforeHBAOCheckbox = false;
	float                    gravityMagnitudeSlider = 0.09f;
	bool                     drawDebugMarkersCheckbox = true;
	//ao
	float aoLargeScaleAOSlider = 0.0f;
	float aoSmallScaleAOSlider = 1.0f;
	float aoBiasSlider = 0.266f;
	bool aoBlurEnableCheckbox = true;
	float aoBlurSharpnessSlider = 100.0f;
	float aoPowerExponentSlider = 2.0f;
	float aoRadiusSlider = 0.038f;
	bool aoDepthThresholdEnableCheckbox = false;
	float aoDepthThresholdMaxViewDepthSlider = 0.0f;
	float aoDepthThresholdSharpnessSlider = 100.0f;
	std::vector<std::string> activeActors = { "PLAYER" };
	uint32_t                 selectedActor = 0;
	std::vector<std::string> activeHairs = { "NONE" };
	uint32_t                 selectedHair = 0;


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
