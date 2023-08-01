#pragma once

#include "imgui.h"
#include "imgui_impl_dx11.h"
#include "imgui_impl_win32.h"
#include <nlohmann/json.hpp>
#include <vector>
#include <string>
#include <glm/glm.hpp>
using json = nlohmann::json;
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
	float                    fiberRadiusSliderValue = 0.002;
	float                    fiberSpacingSliderValue = 0.1;
	float                    fiberRatioSliderValue = 0.5;
	float                    kdSliderValue = 0.07;
	float                    ks1SliderValue = 0.17;
	float                    ex1SliderValue = 14.4;
	float                    ks2SliderValue = 0.72;
	float                    ex2SliderValue = 11.8;
	int                      localConstraintsIterationsSlider = 3;
	int                      lengthConstraintsIterationsSlider = 3;
	float                    localConstraintsStiffnessSlider = 0.9;
	float                    globalConstraintsStiffnessSlider = 0.4;
	float                    globalConstraintsRangeSlider = 0.3;
	float                    dampingSlider = 0.06;
	float                    vspAmountSlider = 0.75;
	float                    vspAccelThresholdSlider = 1.2;
	float                    hairOpacitySlider = 0.63;
	float                    hairShadowAlphaSlider = 0.35;
	bool                     thinTipCheckbox = true;
	bool                     drawShadowsCheckbox = true;
	float                    gravityMagnitudeSlider = 0.09;
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
