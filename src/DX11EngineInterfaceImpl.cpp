#include "DX11EngineInterfaceImpl.h"
#include "Buffer.h"
#include "SkyrimGPUResourceManager.h"
#include "Util.h"
#include "DDSTextureLoader.h"
EI_Device::EI_Device()
{
}

EI_Device::~EI_Device()
{
}
//public
std::unique_ptr<EI_Resource> EI_Device::CreateBufferResource(int structSize, const int structCount, const unsigned int flags, EI_StringHash name)
{
	if (flags == EI_BF_INDEXBUFFER) {
		return CreateIndexBufferResource(structSize, structCount, name);
	} else if (flags == EI_BF_UNIFORMBUFFER) {
		return CreateConstantBufferResource(structSize, structCount, name);
	} else if (flags == EI_BF_NEEDSUAV) {
		return CreateStructuredBufferResource(structSize, structCount, name);
	} else {
		logger::error("Requested unimplemented EI_BufferFlags combination: {}", flags);
		return nullptr;
	}
}
std::unique_ptr<EI_BindLayout> EI_Device::CreateLayout(const EI_LayoutDescription& description)
{
	EI_BindLayout* pLayout = new EI_BindLayout;
	pLayout->description = description;
	return std::unique_ptr<EI_BindLayout>(pLayout);
}
std::unique_ptr<EI_BindSet> EI_Device::CreateBindSet(EI_BindLayout* layout, EI_BindSetDescription& bindSet)
{
	EI_BindSet set;
	int        i = 0;
	for (EI_ResourceDescription resourceDescription : layout->description.resources) {
		EI_Resource* resource = bindSet.resources[i];
		switch (resourceDescription.type) {
		case EI_RESOURCETYPE_BUFFER_RO:
			set.srvs.push_back(resource->SRView);
			break;
		case EI_RESOURCETYPE_IMAGE_RO:
			set.srvs.push_back(resource->SRView);
			break;
		case EI_RESOURCETYPE_BUFFER_RW:
			set.uavs.push_back(resource->UAView);
			break;
		case EI_RESOURCETYPE_IMAGE_RW:
			set.uavs.push_back(resource->UAView);
			break;
		case EI_RESOURCETYPE_SAMPLER:
			set.samplers.push_back(resource->m_pSampler);
			break;
		case EI_RESOURCETYPE_UNIFORM:
			set.cbuffers.push_back(resource->m_pBuffer);
			break;
		}
		i += 1;
	}
}

std::unique_ptr<EI_Resource> EI_Device::CreateResourceFromFile(const char* szFilename, bool useSRGB /*= false*/)
{
	EI_Resource* res = new EI_Resource;
	res->m_ResourceType = EI_ResourceType::Texture;
	auto pDevice = SkyrimGPUResourceManager::GetInstance()->m_pDevice;
	ID3D11DeviceContext* pContext;
	pDevice->GetImmediateContext(&pContext);
	std::filesystem::path name = szFilename;
	logger::info("Creating texture from file");
	DirectX::CreateDDSTextureFromFile(pDevice, pContext, name.generic_wstring().c_str(), (ID3D11Resource**) & res->m_pTexture, &res->SRView);
	return std::unique_ptr<EI_Resource>(res);
}

std::unique_ptr<EI_Resource> EI_Device::CreateUint32Resource(const int width, const int height, const int arraySize, const char* name, uint32_t ClearValue /*= 0*/)
{
	EI_Resource* res = new EI_Resource;
	res->m_ResourceType = EI_ResourceType::Buffer;

	DXGI_SAMPLE_DESC sampleDesc;
	sampleDesc.Count = 1;
	sampleDesc.Quality = 0;

	D3D11_TEXTURE2D_DESC desc;
	desc.Width = width;
	desc.Height = height;
	desc.Format = DXGI_FORMAT_R32_UINT;
	desc.ArraySize = arraySize;
	desc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS;
	desc.MipLevels = 1;
	desc.Usage = D3D11_USAGE_DEFAULT;
	desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	desc.SampleDesc = sampleDesc;
	desc.MiscFlags = 0;

	SkyrimGPUResourceManager::GetInstance()->m_pDevice->CreateTexture2D(&desc, nullptr, &res->m_pTexture);
	return std::unique_ptr<EI_Resource>(res);
}

std::unique_ptr<EI_RenderTargetSet> EI_Device::CreateRenderTargetSet(const EI_ResourceFormat* pResourceFormats, const uint32_t numResources, const EI_AttachmentParams* AttachmentParams, float* clearValues)
{
	assert(numResources < MaxRenderAttachments && "Number of resources exceeds maximum allowable. Please grow MaxRenderAttachments value.");

	// Create the render pass set
	EI_RenderTargetSet* pNewRenderTargetSet = new EI_RenderTargetSet;

	int currentClearValueRef = 0;
	for (uint32_t i = 0; i < numResources; i++) {
		// Check size consistency
		assert(!(AttachmentParams[i].Flags & EI_RenderPassFlags::Depth && (i != (numResources - 1))) && "Only the last attachment can be specified as depth target");

		// Setup a clear value if needed
		if (AttachmentParams[i].Flags & EI_RenderPassFlags::Clear) {
			if (AttachmentParams[i].Flags & EI_RenderPassFlags::Depth) {
				pNewRenderTargetSet->m_ClearValues[i].DepthStencil.Depth = clearValues[currentClearValueRef++];
				pNewRenderTargetSet->m_ClearValues[i].DepthStencil.Stencil = (uint32_t)clearValues[currentClearValueRef++];
				pNewRenderTargetSet->m_ClearValues[i].Format = DXGI_FORMAT_D32_FLOAT;
				pNewRenderTargetSet->m_HasDepth = true;
				pNewRenderTargetSet->m_ClearDepth = true;
			} else {
				pNewRenderTargetSet->m_ClearValues[i].Color[0] = clearValues[currentClearValueRef++];
				pNewRenderTargetSet->m_ClearValues[i].Color[1] = clearValues[currentClearValueRef++];
				pNewRenderTargetSet->m_ClearValues[i].Color[2] = clearValues[currentClearValueRef++];
				pNewRenderTargetSet->m_ClearValues[i].Color[3] = clearValues[currentClearValueRef++];
				pNewRenderTargetSet->m_ClearValues[i].Format = pResourceFormats[i];
				pNewRenderTargetSet->m_ClearColor[i] = true;
			}
		} else if (AttachmentParams[i].Flags & EI_RenderPassFlags::Depth)
			pNewRenderTargetSet->m_HasDepth = true;

		pNewRenderTargetSet->m_RenderResourceFormats[i] = pResourceFormats[i];
	}

	// Tag the number of resources this render pass set is setting/clearing
	pNewRenderTargetSet->m_NumResources = numResources;

	return std::unique_ptr<EI_RenderTargetSet>(pNewRenderTargetSet);
}

std::unique_ptr<EI_Resource> EI_Device::CreateRenderTargetResource(const int width, const int height, const size_t channels, const size_t channelSize, const char* name, AMD::float4* ClearValues /*= nullptr*/)
{
	EI_Resource* res = new EI_Resource;

	res->m_ResourceType = EI_ResourceType::Texture;
	//res->m_pTexture = new CAULDRON_DX12::Texture();

	D3D11_TEXTURE2D_DESC resourceDesc = {};
	switch (channels) {
	case 1:
		resourceDesc.Format = DXGI_FORMAT_R16_FLOAT;
		break;
	case 2:
		resourceDesc.Format = DXGI_FORMAT_R16G16_FLOAT;
		break;
	case 4:
		if (channelSize == 1)
			resourceDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		else
			resourceDesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
		break;
	}
	resourceDesc.ArraySize = 1;
	resourceDesc.Width = width;
	resourceDesc.Height = height;
	resourceDesc.MipLevels = 1;
	resourceDesc.SampleDesc.Count = 1;
	resourceDesc.SampleDesc.Quality = 0;
	resourceDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
	resourceDesc.CPUAccessFlags = DXGI_CPU_ACCESS_NONE;
	resourceDesc.MiscFlags = 0;

	if (channels == 1) {
		resourceDesc.Format = DXGI_FORMAT_R16_FLOAT;
	} else if (channels == 2) {
		resourceDesc.Format = DXGI_FORMAT_R16G16_FLOAT;
	} else if (channels == 4) {
		if (channelSize == 1)
			resourceDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		else
			resourceDesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
	}

	if (ClearValues) {
		D3D11_CLEAR_VALUE ClearParams;
		ClearParams.Format = resourceDesc.Format;
		ClearParams.Color[0] = ClearValues->x;
		ClearParams.Color[1] = ClearValues->y;
		ClearParams.Color[2] = ClearValues->z;
		ClearParams.Color[3] = ClearValues->w;

		// Makes initial barriers easier to deal with
		//res->m_pTexture->Init(&m_device, name, &resourceDesc, D3D12_RESOURCE_STATE_COMMON, &ClearParams);
	} //else
		//res->m_pTexture->InitRenderTarget(&m_device, name, &resourceDesc, D3D12_RESOURCE_STATE_RENDER_TARGET);

	auto pDevice = SkyrimGPUResourceManager::GetInstance()->m_pDevice;
	logger::info("Creating texture2D");
	pDevice->CreateTexture2D(&resourceDesc, nullptr, &res->m_pTexture);
	//res->RTView = new CAULDRON_DX12::RTV();
	//m_resourceViewHeaps.AllocRTVDescriptor(1, res->RTView);
	//res->m_pTexture->CreateRTV(0, res->RTView);
	D3D11_RENDER_TARGET_VIEW_DESC rtviewDesc;
	rtviewDesc.Format = resourceDesc.Format;
	rtviewDesc.Texture2D.MipSlice = 0;
	rtviewDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
	logger::info("Creating RTV");
	pDevice->CreateRenderTargetView(res->m_pTexture, &rtviewDesc, &res->RTView);

	//res->SRView = new CAULDRON_DX12::CBV_SRV_UAV();
	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Format = resourceDesc.Format;
	srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Texture2D.MipLevels = 1;
	//m_resourceViewHeaps.AllocCBV_SRV_UAVDescriptor(1, res->SRView);
	//res->m_pTexture->CreateSRV(0, res->SRView, &srvDesc);
	logger::info("Creating SRV");
	pDevice->CreateShaderResourceView(res->m_pTexture, &srvDesc, &res->SRView);
	return std::unique_ptr<EI_Resource>(res);
}

//private

std::unique_ptr<EI_Resource> EI_Device::CreateIndexBufferResource(int structSize, const int structCount, EI_StringHash name)
{
	ID3D11Buffer*     g_pIndexBuffer = NULL;
	D3D11_BUFFER_DESC bufferDesc;
	bufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	bufferDesc.ByteWidth = structSize * structCount;
	bufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
	bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	bufferDesc.MiscFlags = 0;
	D3D11_SUBRESOURCE_DATA data;
	data.pSysMem = nullptr;
	HRESULT hr = SkyrimGPUResourceManager::GetInstance()->m_pDevice->CreateBuffer(&bufferDesc, &data, &g_pIndexBuffer);
	if (FAILED(hr))
		logger::error("index buffer creation failed");
	EI_Resource* res = new EI_Resource;
	res->m_ResourceType = EI_ResourceType::IndexBuffer;
	res->m_pBuffer = g_pIndexBuffer;
	return std::unique_ptr<EI_Resource>(res);
}
std::unique_ptr<EI_Resource> EI_Device::CreateConstantBufferResource(int structSize, const int structCount, EI_StringHash name)
{
	ID3D11Buffer*     g_pConstantBuffer;
	D3D11_BUFFER_DESC cbDesc;
	cbDesc.ByteWidth = structSize * structCount;
	cbDesc.Usage = D3D11_USAGE_DYNAMIC;
	cbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	cbDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	cbDesc.MiscFlags = 0;
	cbDesc.StructureByteStride = 0;

	// Fill in the subresource data.
	D3D11_SUBRESOURCE_DATA InitData;
	InitData.pSysMem = nullptr;
	InitData.SysMemPitch = 0;
	InitData.SysMemSlicePitch = 0;
	HRESULT hr = SkyrimGPUResourceManager::GetInstance()->m_pDevice->CreateBuffer(&cbDesc, &InitData,
		&g_pConstantBuffer);

	if (FAILED(hr))
		logger::info("Constant buffer creation failed");
	EI_Resource* res = new EI_Resource;
	res->m_ResourceType = EI_ResourceType::ConstantBuffer;
	res->m_pBuffer = g_pConstantBuffer;
	return std::unique_ptr<EI_Resource>(res);
}

std::unique_ptr<EI_Resource> EI_Device::CreateStructuredBufferResource(int structSize, const int structCount, EI_StringHash name)
{
	bool              counted = true;
	bool              uav = true;
	auto              pManager = SkyrimGPUResourceManager::GetInstance();
	D3D11_BUFFER_DESC nextDesc = StructuredBufferDesc(structCount, structSize, true, false);
	//r.desc = nextDesc;
	//logger::info("creating buffer");
	ID3D11Buffer* pStructuredBuffer;
	EI_Resource*  pResource = new EI_Resource;
	HRESULT       hr = pManager->m_pDevice->CreateBuffer(&nextDesc, NULL, &pStructuredBuffer);
	Util::printHResult(hr);
	//r.structCount = structCount;
	//r.structSize = structSize;

	//SU_ASSERT(r.buffer);

	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
	srvDesc.Format = DXGI_FORMAT_UNKNOWN;
	srvDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
	srvDesc.Buffer.FirstElement = 0;
	//this union caused me lots of pain
	//srvDesc.Buffer.ElementOffset = 0;
	srvDesc.Buffer.NumElements = structCount;
	//srvDesc.Buffer.ElementWidth = structSize;
	//logger::info("creating srv");
	HRESULT hr1 = pManager->m_pDevice->CreateShaderResourceView(pStructuredBuffer, &srvDesc, &pResource->SRView);
	Util::printHResult(hr1);
	//SU_ASSERT(r.srv);

	if (uav) {
		D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc;
		uavDesc.Format = DXGI_FORMAT_UNKNOWN;
		uavDesc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
		uavDesc.Buffer.FirstElement = 0;
		uavDesc.Buffer.NumElements = structCount;
		if (counted) {
			//logger::info("adding uav counter");
			uavDesc.Buffer.Flags = D3D11_BUFFER_UAV_FLAG_COUNTER;
		} else {
			uavDesc.Buffer.Flags = 0;
		}
		//logger::info("Creating UAV");
		HRESULT hr2 = pManager->m_pDevice->CreateUnorderedAccessView(pStructuredBuffer, &uavDesc, &pResource->UAView);
		Util::printHResult(hr2);
		logger::info("Addr. of uav: {}", Util::ptr_to_string(pResource->UAView));
	}
}
//end device funcs

EI_Resource::EI_Resource() :
	m_ResourceType(EI_ResourceType::Undefined),
	m_pTexture(nullptr)
{
}

EI_Resource::~EI_Resource()
{
	if (m_ResourceType == EI_ResourceType::Buffer || m_ResourceType == EI_ResourceType::IndexBuffer && m_pBuffer != nullptr) {
		m_pBuffer->Release();
	} else if (m_ResourceType == EI_ResourceType::Texture) {
		m_pTexture->Release();
	} else if (m_ResourceType == EI_ResourceType::Sampler) {
		m_pSampler->Release();
	} else {
		assert(false && "Trying to destroy an undefined resource");
	}
}

void EI_RenderTargetSet::SetResources(const EI_Resource** pResourcesArray)
{
	for (uint32_t i = 0; i < m_NumResources; ++i)
		m_RenderResources[i] = pResourcesArray[i];
}

EI_RenderTargetSet::~EI_RenderTargetSet()
{
	// Nothing to clean up
}
