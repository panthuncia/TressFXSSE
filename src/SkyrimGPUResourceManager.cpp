#include "SkyrimGPUResourceManager.h"
#include <comdef.h>
#include <atlstr.h>
#include "Util.h"
SkyrimGPUResourceManager::SkyrimGPUResourceManager(ID3D11Device* pDevice, IDXGISwapChain* pSwapChain)
{
	this->m_pDevice = pDevice;
	this->m_pSwapChain = pSwapChain;
	}
SkyrimGPUResourceManager::~SkyrimGPUResourceManager() {
	}
SkyrimGPUResourceManager* SkyrimGPUResourceManager::GetInstance(ID3D11Device* pDevice, IDXGISwapChain* pSwapChain) {
	if (singleton_ == nullptr) {
		singleton_ = new SkyrimGPUResourceManager(pDevice, pSwapChain);
	}
	return singleton_;
}
SkyrimGPUResourceManager* SkyrimGPUResourceManager::GetInstance() {
	if (singleton_ == nullptr) {
		throw std::invalid_argument("Use other GetSingleton() to initialize device");
	}
	return singleton_;
}
	ID3D11Buffer* SkyrimGPUResourceManager::CreateStructuredBuffer(uint32_t nStructSize,
		uint32_t nNumStructs,
		uint32_t nBindFlags,
		D3D11_SUBRESOURCE_DATA* pInitialData,
		const std::string& rName,
		uint32_t nMiscHintFlags,
		uint32_t nNumMaxStagingBuffers) 
	{
		//TODO ignore these for now
		nMiscHintFlags = nMiscHintFlags;
		nNumMaxStagingBuffers = nNumMaxStagingBuffers;
		nStructSize = nStructSize;

		ID3D11Buffer* buffer = NULL;
		unsigned int stride = nStructSize;
		unsigned int byteWidth = nStructSize*nNumStructs;
		D3D11_BUFFER_DESC desc;
		desc.ByteWidth = byteWidth;
		desc.Usage = nBindFlags & D3D11_BIND_UNORDERED_ACCESS? D3D11_USAGE_DEFAULT:D3D11_USAGE_DYNAMIC;
		desc.BindFlags = nBindFlags;
		desc.CPUAccessFlags = nBindFlags & D3D11_BIND_UNORDERED_ACCESS? D3D11_CPU_ACCESS_WRITE|D3D11_CPU_ACCESS_READ:D3D11_CPU_ACCESS_READ;
		desc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED; // D3D11_RESOURCE_MISC_DRAWINDIRECT_ARGS ?
		desc.StructureByteStride = stride;
		D3D11_SUBRESOURCE_DATA data;
		data.pSysMem = pInitialData;
		data.SysMemPitch = 0;
		data.SysMemSlicePitch = 0;
		HRESULT hr = m_pDevice->CreateBuffer(&desc, pInitialData, &buffer);
		_com_error err(hr);
		LPCTSTR errMsg = err.ErrorMessage();
		CStringA sB(errMsg);
		const char* errConv = sB;
		if (hr != S_OK) {
			logger::error("D3D11Device::CreateBuffer() failed: {}", errConv);
			Util::printHResult(hr);
		}
		logger::info("Created buffer");
		buffers[rName] = buffer;
		return buffer;
	}
