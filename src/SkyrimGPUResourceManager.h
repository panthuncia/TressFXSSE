#pragma once
#include "d3d11.h"
class SkyrimGPUResourceManager {
protected:
	//=================================================================================================   
	/// Constructor
	//=================================================================================================   
	SkyrimGPUResourceManager(ID3D11Device* pDevice, IDXGISwapChain* pSwapChain);
	static inline SkyrimGPUResourceManager* singleton_;
public:
	//=================================================================================================   
	/// Destructor
	//=================================================================================================   
	virtual ~SkyrimGPUResourceManager();

	/**
	 * Singletons should not be cloneable.
	 */
	SkyrimGPUResourceManager(SkyrimGPUResourceManager& other) = delete;
	/**
	 * Singletons should not be assignable.
	 */
	void operator=(const SkyrimGPUResourceManager&) = delete;
	/**
	 * This is the static method that controls the access to the singleton
	 * instance. On the first run, it creates a singleton object and places it
	 * into the static field. On subsequent runs, it returns the client existing
	 * object stored in the static field.
	 */
	static SkyrimGPUResourceManager* GetInstance(ID3D11Device* pDevice, IDXGISwapChain* pSwapChain);
	static SkyrimGPUResourceManager* GetInstance();

	inline static ID3D11Device* m_pDevice;
	inline static IDXGISwapChain* m_pSwapChain;
	ID3D11DeviceContext* m_pContext;
	/// Creates a structured buffer
	ID3D11Buffer* CreateStructuredBuffer(uint32_t nStructSize,
		uint32_t nNumStructs,
		uint32_t nBindFlags,
		D3D11_SUBRESOURCE_DATA* pInitialData,
		const std::string& rName,
		uint32_t nMiscHintFlags = 0,
		uint32_t nNumMaxStagingBuffers = 1);
private:
	std::unordered_map<std::string, ID3D11Buffer*> buffers;
};
