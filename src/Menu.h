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
	void DrawMatrix(glm::mat4 mat, std::string name);
	void DrawMatrix(RE::NiMatrix3 mat, std::string name);
	void DrawMatrix(DirectX::XMMATRIX mat, std::string name);
	void DrawVector3(glm::vec3 mat, std::string name);
	void DrawVector3(RE::NiPoint3 mat, std::string name);
	void DrawFloat(float mat, std::string name);

private:
	uint32_t toggleKey = VK_DELETE;
	bool     settingToggleKey = false;

	Menu() {}
	const char*    KeyIdToString(uint32_t key);
	const ImGuiKey VirtualKeyToImGuiKey(WPARAM vkKey);

	std::vector<glm::mat4> matrices;
	std::vector<std::string> matrixNames;

	std::vector<glm::vec3>   vector3s;
	std::vector<std::string> vector3Names;

	std::vector<float> floats;
	std::vector<std::string> floatNames;
};
