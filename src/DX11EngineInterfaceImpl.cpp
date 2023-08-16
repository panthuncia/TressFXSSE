#include "DX11EngineInterfaceImpl.h"
#include "Buffer.h"
#include "DDSTextureLoader.h"
#include "ShaderCompiler.h"
#include "SkyrimGPUResourceManager.h"
#include "TressFX/AMD_TressFX.h"
#include "Util.h"
#include "TressFX/TressFXLayouts.h"

inline D3D11_BLEND_OP operator*(EI_BlendOp Enum)
{
	if (Enum == EI_BlendOp::Add)
		return D3D11_BLEND_OP_ADD;
	if (Enum == EI_BlendOp::Subtract)
		return D3D11_BLEND_OP_SUBTRACT;
	if (Enum == EI_BlendOp::ReverseSubtract)
		return D3D11_BLEND_OP_REV_SUBTRACT;
	if (Enum == EI_BlendOp::Min)
		return D3D11_BLEND_OP_MIN;
	if (Enum == EI_BlendOp::Max)
		return D3D11_BLEND_OP_MAX;
	assert(false && "Using an EI_BlendOp that has not been mapped to DX11 yet!");
	return D3D11_BLEND_OP_ADD;
};

inline D3D11_BLEND operator*(EI_BlendFactor Enum)
{
	if (Enum == EI_BlendFactor::Zero)
		return D3D11_BLEND_ZERO;
	if (Enum == EI_BlendFactor::One)
		return D3D11_BLEND_ONE;
	if (Enum == EI_BlendFactor::SrcColor)
		return D3D11_BLEND_SRC_COLOR;
	if (Enum == EI_BlendFactor::InvSrcColor)
		return D3D11_BLEND_INV_SRC_COLOR;
	if (Enum == EI_BlendFactor::DstColor)
		return D3D11_BLEND_DEST_COLOR;
	if (Enum == EI_BlendFactor::InvDstColor)
		return D3D11_BLEND_INV_DEST_COLOR;
	if (Enum == EI_BlendFactor::SrcAlpha)
		return D3D11_BLEND_SRC_ALPHA;
	if (Enum == EI_BlendFactor::InvSrcAlpha)
		return D3D11_BLEND_INV_SRC_ALPHA;
	if (Enum == EI_BlendFactor::DstAlpha)
		return D3D11_BLEND_DEST_ALPHA;
	if (Enum == EI_BlendFactor::InvDstAlpha)
		return D3D11_BLEND_INV_DEST_ALPHA;
	assert(false && "Using an EI_BlendFactor that has not been mapped to DX11 yet!");
	return D3D11_BLEND_ZERO;
};

inline D3D11_STENCIL_OP operator*(EI_StencilOp Enum)
{
	if (Enum == EI_StencilOp::Keep)
		return D3D11_STENCIL_OP_KEEP;
	if (Enum == EI_StencilOp::Zero)
		return D3D11_STENCIL_OP_ZERO;
	if (Enum == EI_StencilOp::Replace)
		return D3D11_STENCIL_OP_REPLACE;
	if (Enum == EI_StencilOp::IncrementClamp)
		return D3D11_STENCIL_OP_INCR_SAT;
	if (Enum == EI_StencilOp::DecrementClamp)
		return D3D11_STENCIL_OP_DECR_SAT;
	if (Enum == EI_StencilOp::Invert)
		return D3D11_STENCIL_OP_INVERT;
	if (Enum == EI_StencilOp::IncrementWrap)
		return D3D11_STENCIL_OP_INCR;
	if (Enum == EI_StencilOp::DecrementWrap)
		return D3D11_STENCIL_OP_DECR;
	assert(false && "Using an EI_StencilOp that has not been mapped to DX11 yet!");
	return D3D11_STENCIL_OP_KEEP;
}

inline D3D11_COMPARISON_FUNC operator*(EI_CompareFunc Enum)
{
	if (Enum == EI_CompareFunc::Never)
		return D3D11_COMPARISON_NEVER;
	if (Enum == EI_CompareFunc::Less)
		return D3D11_COMPARISON_LESS;
	if (Enum == EI_CompareFunc::Equal)
		return D3D11_COMPARISON_EQUAL;
	if (Enum == EI_CompareFunc::LessEqual)
		return D3D11_COMPARISON_LESS_EQUAL;
	if (Enum == EI_CompareFunc::Greater)
		return D3D11_COMPARISON_GREATER;
	if (Enum == EI_CompareFunc::NotEqual)
		return D3D11_COMPARISON_NOT_EQUAL;
	if (Enum == EI_CompareFunc::GreaterEqual)
		return D3D11_COMPARISON_GREATER_EQUAL;
	if (Enum == EI_CompareFunc::Always)
		return D3D11_COMPARISON_ALWAYS;
	assert(false && "Using an EI_CompareFunc that has not been mapped to DX11 yet!");
	return D3D11_COMPARISON_NEVER;
}

inline D3D11_PRIMITIVE_TOPOLOGY operator*(EI_Topology Enum)
{
	if (Enum == EI_Topology::TriangleList)
		return D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	if (Enum == EI_Topology::TriangleStrip)
		return D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP;
	assert(false && "Using an EI_Topology that has not been mapped to DX12 yet!");
	return D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
}

static EI_Device* g_device = NULL;

EI_Device* GetDevice()
{
	if (g_device == NULL) {
		g_device = new EI_Device();
	}
	return g_device;
}

EI_Device::EI_Device()
{
}

EI_Device::~EI_Device()
{
}
//public
std::unique_ptr<EI_Resource> EI_Device::CreateBufferResource(int structSize, const int structCount, const unsigned int flags, EI_StringHash name)
{
	logger::info("CreateBufferResource: {}", name);
	if (flags & EI_BF_INDEXBUFFER) {
		logger::info("Index buffer");
		return CreateIndexBufferResource(structSize, structCount, name);
	} else if (flags & EI_BF_UNIFORMBUFFER) {
		return CreateConstantBufferResource(structSize, structCount, name);
		logger::info("Constant buffer");
	} else if (flags & EI_BF_NEEDSUAV) {
		logger::info("Structured buffer with UAV");
		return CreateStructuredBufferResource(structSize, structCount, true, name);
	} else if (flags == 0) {
		logger::info("Structured buffer no uav");
		return CreateStructuredBufferResource(structSize, structCount, false, name);
	} else {
		logger::error("Requested unimplemented EI_BufferFlags combination: {}", flags);
		return nullptr;
	}
}
std::unique_ptr<EI_BindLayout> EI_Device::CreateLayout(const EI_LayoutDescription& description)
{
	logger::info("CreateLayout");
	EI_BindLayout* pLayout = new EI_BindLayout;
	pLayout->description = description;
	return std::unique_ptr<EI_BindLayout>(pLayout);
}
std::unique_ptr<EI_BindSet> EI_Device::CreateBindSet(EI_BindLayout* layout, EI_BindSetDescription& bindSet)
{
	logger::info("CreateBindSet");
	EI_BindSet* set = new EI_BindSet;
	logger::info("BindSet address: {}", Util::ptr_to_string(set));
	set->stage = layout->description.stage;
	int i = 0;
	for (EI_ResourceDescription resourceDescription : layout->description.resources) {
		EI_Resource* resource = bindSet.resources[i];
		logger::info("resource name: {}", resource->name);
		switch (resourceDescription.type) {
		case EI_RESOURCETYPE_BUFFER_RO:
			logger::info("Resource: {} type: SRV", resource->name);
			set->srvs.push_back(resource->SRView);
			break;
		case EI_RESOURCETYPE_IMAGE_RO:
			logger::info("Resource: {} type: SRV", resource->name);
			set->srvs.push_back(resource->SRView);
			break;
		case EI_RESOURCETYPE_BUFFER_RW:
			logger::info("Resource: {} type: UAV", resource->name);
			set->uavs.push_back(resource->UAView);
			break;
		case EI_RESOURCETYPE_IMAGE_RW:
			logger::info("Resource: {} type: UAV", resource->name);
			set->uavs.push_back(resource->UAView);
			break;
		case EI_RESOURCETYPE_SAMPLER:
			logger::info("Resource: {} type: sampler", resource->name);
			set->samplers.push_back(resource->m_pSampler);
			break;
		case EI_RESOURCETYPE_UNIFORM:
			logger::info("Resource: {} type: CBuffer", resource->name);
			set->cbuffers.push_back(resource->m_pBuffer);
			break;
		}
		i += 1;
	}
	return std::unique_ptr<EI_BindSet>(set);
}

std::unique_ptr<EI_Resource> EI_Device::CreateResourceFromFile(const char* szFilename, bool useSRGB /*= false*/)
{
	UNREFERENCED_PARAMETER(useSRGB);
	EI_Resource* res = new EI_Resource;
	res->m_ResourceType = EI_ResourceType::Texture;
	auto                 pDevice = SkyrimGPUResourceManager::GetInstance()->m_pDevice;
	ID3D11DeviceContext* pContext;
	pDevice->GetImmediateContext(&pContext);
	std::filesystem::path name = szFilename;
	logger::info("Creating texture from file");
	DirectX::CreateDDSTextureFromFile(pDevice, pContext, name.generic_wstring().c_str(), (ID3D11Resource**)&res->m_pTexture, &res->SRView);
	res->name = szFilename;
	return std::unique_ptr<EI_Resource>(res);
}

std::unique_ptr<EI_Resource> EI_Device::CreateUint32Resource(const int width, const int height, const int arraySize, const char* name, uint32_t ClearValue /*= 0*/)
{
	UNREFERENCED_PARAMETER(ClearValue);
	logger::info("CreateUint32Resource");
	auto         pDevice = SkyrimGPUResourceManager::GetInstance()->m_pDevice;
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
	desc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS;
	desc.MipLevels = 1;
	desc.Usage = D3D11_USAGE_DEFAULT;
	desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	desc.SampleDesc = sampleDesc;
	desc.MiscFlags = 0;
	res->m_textureWidth = width;
	res->m_textureHeight = height;
	res->name = name;
	auto hr = pDevice->CreateTexture2D(&desc, nullptr, &res->m_pTexture);
	if (FAILED(hr)) {
		logger::info("UInt32 texture creation failed");
		Util::PrintAllD3D11DebugMessages();
	}

	//create SRV
	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
	srvDesc.Format = DXGI_FORMAT_R32_UINT;
	/*srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DARRAY;
		srvDesc.Texture2DArray.MostDetailedMip = 0;
		srvDesc.Texture2DArray.MipLevels = 1;
		srvDesc.Texture2DArray.FirstArraySlice = 0;
		srvDesc.Texture2DArray.ArraySize = (UINT)arraySize;*/
	srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = 1;
	srvDesc.Texture2D.MostDetailedMip = 0;
	//logger::info("Creating SRV");
	hr = pDevice->CreateShaderResourceView(res->m_pTexture, &srvDesc, &res->SRView);
	if (FAILED(hr)) {
		logger::info("UInt32 texture creation failed");
		Util::PrintAllD3D11DebugMessages();
	}
	//create UAV
	D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc;
	uavDesc.Format = DXGI_FORMAT_R32_UINT;
	/*uavDesc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2DARRAY;
		uavDesc.Texture2DArray.ArraySize = (UINT)arraySize;
		uavDesc.Texture2DArray.FirstArraySlice = 0;
		uavDesc.Texture2DArray.MipSlice = 0;*/
	uavDesc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2D;
	uavDesc.Texture2D.MipSlice = 0;
	//logger::info("Creating UAV");
	hr = pDevice->CreateUnorderedAccessView(res->m_pTexture, &uavDesc, &res->UAView);
	if (FAILED(hr)) {
		logger::info("UInt32 texture creation failed");
		Util::PrintAllD3D11DebugMessages();
	}

	return std::unique_ptr<EI_Resource>(res);
}

std::unique_ptr<EI_RenderTargetSet> EI_Device::CreateRenderTargetSet(const EI_ResourceFormat* pResourceFormats, const uint32_t numResources, const EI_AttachmentParams* AttachmentParams, float* clearValues)
{
	logger::info("CreateRenderTargetSet");
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
				pNewRenderTargetSet->m_ClearValues[i].DepthStencil.Stencil = (uint8_t)clearValues[currentClearValueRef++];
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

std::unique_ptr<EI_RenderTargetSet> EI_Device::CreateRenderTargetSet(const EI_Resource** pResourcesArray, const uint32_t numResources, const EI_AttachmentParams* AttachmentParams, float* clearValues)
{
	logger::info("Create render target set 1");
	logger::info("num resources: {}", numResources);
	std::vector<EI_ResourceFormat> FormatArray(numResources);

	for (uint32_t i = 0; i < numResources; ++i) {
		logger::info("Resource address: {}", Util::ptr_to_string((void*)pResourcesArray[i]));
		assert(pResourcesArray[i]->m_ResourceType == EI_ResourceType::Texture);
		D3D11_TEXTURE2D_DESC desc;
		logger::info("Texture address: {}", Util::ptr_to_string(pResourcesArray[i]->m_pTexture));
		pResourcesArray[i]->m_pTexture->GetDesc(&desc);
		logger::info("got desc");
		FormatArray.push_back(desc.Format);
		if (FormatArray[i] == DXGI_FORMAT_R32_TYPELESS) {
			FormatArray[i] = DXGI_FORMAT_D32_FLOAT;
		}
	}
	std::unique_ptr<EI_RenderTargetSet> result = CreateRenderTargetSet(FormatArray.data(), numResources, AttachmentParams, clearValues);
	result->SetResources(pResourcesArray);
	return result;
}

std::unique_ptr<EI_Resource> EI_Device::CreateRenderTargetResource(const int width, const int height, const size_t channels, const size_t channelSize, const char* name, AMD::float4* ClearValues /*= nullptr*/)
{
	logger::info("CreateRenderTarget");
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
	}  //else
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
	res->name = name;
	return std::unique_ptr<EI_Resource>(res);
}

//private
std::unique_ptr<EI_Resource> EI_Device::CreateStandardBufferResource(int structSize, const int structCount, EI_StringHash name)
{
	logger::info("Create standard buffer");
	EI_Resource*      res = new EI_Resource;
	auto              pManager = SkyrimGPUResourceManager::GetInstance();
	ID3D11Buffer*     pBuffer = NULL;
	UINT              size = structSize * structCount;
	D3D11_BUFFER_DESC bufferDesc;
	DXGI_FORMAT       format = DXGI_FORMAT_R16_FLOAT;
	if (structSize == 4) {
		format = DXGI_FORMAT_R16_FLOAT;
	} else if (structSize == 16) {
		format = DXGI_FORMAT_R16G16B16A16_FLOAT;
	} else if (structSize == 32) {
		format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	} else {
		logger::error("Unexpected buffer type! bytes: {}", structSize);
	}
	bufferDesc.Usage = D3D11_USAGE_DEFAULT;
	bufferDesc.ByteWidth = size;
	bufferDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS;
	bufferDesc.CPUAccessFlags = 0;
	bufferDesc.MiscFlags = 0;
	HRESULT hr = SkyrimGPUResourceManager::GetInstance()->m_pDevice->CreateBuffer(&bufferDesc, nullptr, &pBuffer);
	if (FAILED(hr)) {
		logger::error("standard buffer creation failed");
		Util::PrintAllD3D11DebugMessages();
	}
	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
	srvDesc.Format = format;
	srvDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
	srvDesc.Buffer.FirstElement = 0;
	//this union caused me lots of pain
	//srvDesc.Buffer.ElementOffset = 0;
	srvDesc.Buffer.NumElements = structCount;
	//srvDesc.Buffer.ElementWidth = structSize;
	//logger::info("creating srv");
	HRESULT hr1 = pManager->m_pDevice->CreateShaderResourceView(pBuffer, &srvDesc, &res->SRView);
	Util::printHResult(hr1);
	if (FAILED(hr1)) {
		logger::error("standard buffer SRV creation failed");
		Util::PrintAllD3D11DebugMessages();
	}
	//SU_ASSERT(r.srv);

	D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc;
	uavDesc.Format = format;
	uavDesc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
	uavDesc.Buffer.FirstElement = 0;
	uavDesc.Buffer.NumElements = structCount;
	uavDesc.Buffer.Flags = 0;
	//if (counted) {
	//	//logger::info("adding uav counter");
	//	uavDesc.Buffer.Flags = D3D11_BUFFER_UAV_FLAG_COUNTER;
	//} else {
	//	uavDesc.Buffer.Flags = 0;
	//}
	//logger::info("Creating UAV");
	HRESULT hr2 = pManager->m_pDevice->CreateUnorderedAccessView(pBuffer, &uavDesc, &res->UAView);
	Util::printHResult(hr2);
	if (FAILED(hr2)) {
		logger::error("standard buffer UAV creation failed");
		Util::PrintAllD3D11DebugMessages();
	}
	logger::info("Addr. of uav: {}", Util::ptr_to_string(res->UAView));
	res->name = name;
	return std::unique_ptr<EI_Resource>(res);
}

std::unique_ptr<EI_Resource> EI_Device::CreateIndexBufferResource(int structSize, const int structCount, EI_StringHash name)
{
	logger::info("CreateIndexBuffer");
	ID3D11Buffer*     g_pIndexBuffer = NULL;
	UINT              size = structSize * structCount;
	D3D11_BUFFER_DESC bufferDesc;
	bufferDesc.Usage = D3D11_USAGE_DEFAULT;
	bufferDesc.ByteWidth = size;
	bufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
	bufferDesc.CPUAccessFlags = 0;
	bufferDesc.MiscFlags = 0;
	/*D3D11_SUBRESOURCE_DATA data;
	data.pSysMem = nullptr;*/
	HRESULT hr = SkyrimGPUResourceManager::GetInstance()->m_pDevice->CreateBuffer(&bufferDesc, nullptr, &g_pIndexBuffer);
	if (FAILED(hr)) {
		logger::error("index buffer creation failed");
		Util::PrintAllD3D11DebugMessages();
	}
	EI_Resource* res = new EI_Resource;
	res->m_indexBufferNumIndices = structCount;
	res->m_ResourceType = EI_ResourceType::IndexBuffer;
	res->m_pBuffer = g_pIndexBuffer;
	res->m_totalMemSize = size;
	res->name = name;
	return std::unique_ptr<EI_Resource>(res);
}
std::unique_ptr<EI_Resource> EI_Device::CreateConstantBufferResource(int structSize, const int structCount, EI_StringHash name)
{
	logger::info("CreateConstantBuffer");
	ID3D11Buffer*     g_pConstantBuffer;
	D3D11_BUFFER_DESC cbDesc;
	cbDesc.ByteWidth = structSize * structCount;
	cbDesc.Usage = D3D11_USAGE_DEFAULT;
	cbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	cbDesc.CPUAccessFlags = 0;
	cbDesc.MiscFlags = 0;
	cbDesc.StructureByteStride = 0;

	// Fill in the subresource data.
	/*D3D11_SUBRESOURCE_DATA InitData;
	InitData.pSysMem = nullptr;
	InitData.SysMemPitch = 0;
	InitData.SysMemSlicePitch = 0;*/
	HRESULT hr = SkyrimGPUResourceManager::GetInstance()->m_pDevice->CreateBuffer(&cbDesc, nullptr,
		&g_pConstantBuffer);

	if (FAILED(hr)) {
		logger::info("Constant buffer creation failed");
		Util::PrintAllD3D11DebugMessages();
	}
	EI_Resource* res = new EI_Resource;
	res->m_ResourceType = EI_ResourceType::ConstantBuffer;
	res->m_pBuffer = g_pConstantBuffer;
	res->name = name;
	res->m_totalMemSize = cbDesc.ByteWidth;
	return std::unique_ptr<EI_Resource>(res);
}

std::unique_ptr<EI_Resource> EI_Device::CreateStructuredBufferResource(int structSize, const int structCount, bool uav, EI_StringHash name)
{
	logger::info("CreateStructuredBuffer");
	bool              counted = true;
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
	if (FAILED(hr)) {
		logger::info("Structured buffer SRV creation failed");
		Util::PrintAllD3D11DebugMessages();
	}
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
		if (FAILED(hr2)) {
			logger::info("Structured buffer UAV creation failed");
			Util::PrintAllD3D11DebugMessages();
		}
		logger::info("Addr. of uav: {}", Util::ptr_to_string(pResource->UAView));
	}
	pResource->name = name;
	pResource->m_pBuffer = pStructuredBuffer;
	pResource->m_totalMemSize = nextDesc.ByteWidth;
	return std::unique_ptr<EI_Resource>(pResource);
}
std::unique_ptr<EI_PSO> EI_Device::CreateComputeShaderPSO(const char* shaderName, const char* entryPoint, EI_BindLayout** layouts, int numLayouts)
{
	logger::info("CreateComputeShaderPSO");
	EI_PSO*                       PSO = new EI_PSO;
	const auto                    path = std::filesystem::current_path() / ("data/Shaders/TressFX/" + std::string(shaderName));
	auto                          num_bones = std::to_string(AMD_TRESSFX_MAX_NUM_BONES);
	auto                          group_render = std::to_string(AMD_TRESSFX_MAX_HAIR_GROUP_RENDER);
	std::vector<D3D_SHADER_MACRO> defines = { { "AMD_TRESSFX_MAX_NUM_BONES", num_bones.c_str() }, { "AMD_TRESSFX_MAX_HAIR_GROUP_RENDER", group_render.c_str() }, { "AMD_TRESSFX_DX12", "0" }, { NULL, NULL } };
	auto                          shader = ShaderCompiler::CompileShader(path.generic_wstring().c_str(), entryPoint, defines, "cs_5_0");
	logger::info("Compute shader address: {}", Util::ptr_to_string(shader));

	PSO->CS = reinterpret_cast<ID3D11ComputeShader*>(shader);
	for (int i = 0; i < numLayouts; i++) {
		assert(layouts[i]->description.stage == EI_CS);
		for (auto resource : layouts[i]->description.resources) {
			logger::info("CS resource type: {}", resource.type);
		}
	}
	PSO->bp = EI_BP_COMPUTE;
	return std::unique_ptr<EI_PSO>(PSO);
}

std::unique_ptr<EI_PSO> EI_Device::CreateGraphicsPSO(const char* vertexShaderName, const char* vertexEntryPoint, const char* fragmentShaderName, const char* fragmentEntryPoint, EI_PSOParams& psoParams)
{
	logger::info("CreateGraphicsPSO");
	auto                          pDevice = SkyrimGPUResourceManager::GetInstance()->m_pDevice;
	EI_PSO*                       result = new EI_PSO;
	const auto                    vsPath = std::filesystem::current_path() / ("data/Shaders/TressFX/" + std::string(vertexShaderName));
	const auto                    psPath = std::filesystem::current_path() / ("data/Shaders/TressFX/" + std::string(fragmentShaderName));
	auto                          num_bones = std::to_string(AMD_TRESSFX_MAX_NUM_BONES);
	auto                          group_render = std::to_string(AMD_TRESSFX_MAX_HAIR_GROUP_RENDER);
	std::vector<D3D_SHADER_MACRO> strand_defines = { { "AMD_TRESSFX_MAX_NUM_BONES", num_bones.c_str() }, { "AMD_TRESSFX_MAX_HAIR_GROUP_RENDER", group_render.c_str() }, { "AMD_TRESSFX_DX12", "0" }, { NULL, NULL } };

	auto vertexShader = ShaderCompiler::CompileShader(vsPath.generic_wstring().c_str(), vertexEntryPoint, strand_defines, "vs_5_0");
	auto pixelShader = ShaderCompiler::CompileShader(psPath.generic_wstring().c_str(), fragmentEntryPoint, strand_defines, "ps_5_0");

	logger::info("Vertex shader address: {}", Util::ptr_to_string(vertexShader));
	logger::info("Pixel shader address: {}", Util::ptr_to_string(pixelShader));

	D3D11_BLEND_DESC blendDesc;
	blendDesc.AlphaToCoverageEnable = false;
	blendDesc.IndependentBlendEnable = false;
	blendDesc.RenderTarget[0].BlendEnable = psoParams.colorBlendParams.colorBlendEnabled;
	blendDesc.RenderTarget[0].SrcBlend = *psoParams.colorBlendParams.colorSrcBlend;
	blendDesc.RenderTarget[0].DestBlend = *psoParams.colorBlendParams.colorDstBlend;
	blendDesc.RenderTarget[0].BlendOp = *psoParams.colorBlendParams.colorBlendOp;
	blendDesc.RenderTarget[0].SrcBlendAlpha = *psoParams.colorBlendParams.alphaSrcBlend;
	blendDesc.RenderTarget[0].DestBlendAlpha = *psoParams.colorBlendParams.alphaDstBlend;
	blendDesc.RenderTarget[0].BlendOpAlpha = *psoParams.colorBlendParams.alphaBlendOp;
	blendDesc.RenderTarget[0].RenderTargetWriteMask = psoParams.colorWriteEnable ? D3D11_COLOR_WRITE_ENABLE_ALL : 0;
	ID3D11BlendState* blendState;
	pDevice->CreateBlendState(&blendDesc, &blendState);

	D3D11_DEPTH_STENCIL_DESC depthDesc;
	depthDesc.DepthEnable = psoParams.depthTestEnable;
	depthDesc.StencilEnable = psoParams.stencilTestEnable;
	depthDesc.DepthFunc = *psoParams.depthCompareOp;
	depthDesc.DepthWriteMask = psoParams.depthWriteEnable ? D3D11_DEPTH_WRITE_MASK_ALL : D3D11_DEPTH_WRITE_MASK_ZERO;
	depthDesc.StencilReadMask = (uint8_t)psoParams.stencilReadMask;
	depthDesc.StencilWriteMask = (uint8_t)psoParams.stencilWriteMask;
	depthDesc.BackFace.StencilDepthFailOp = *psoParams.backDepthFailOp;
	depthDesc.BackFace.StencilFailOp = *psoParams.backFailOp;
	depthDesc.BackFace.StencilFunc = *psoParams.backCompareOp;
	depthDesc.BackFace.StencilPassOp = *psoParams.backPassOp;
	depthDesc.FrontFace.StencilDepthFailOp = *psoParams.frontDepthFailOp;
	depthDesc.FrontFace.StencilFailOp = *psoParams.frontFailOp;
	depthDesc.FrontFace.StencilFunc = *psoParams.frontCompareOp;
	depthDesc.FrontFace.StencilPassOp = *psoParams.frontPassOp;
	ID3D11DepthStencilState* depthState;
	pDevice->CreateDepthStencilState(&depthDesc, &depthState);

	D3D11_RASTERIZER_DESC rasterizerDesc;
	rasterizerDesc.FillMode = D3D11_FILL_SOLID;
	if (psoParams.primitiveTopology == EI_Topology::TriangleStrip) {
		rasterizerDesc.CullMode = D3D11_CULL_NONE;
	} else {
		rasterizerDesc.CullMode = D3D11_CULL_BACK;
	}
	rasterizerDesc.FrontCounterClockwise = true;
	rasterizerDesc.DepthBias = 0;
	rasterizerDesc.DepthBiasClamp = 0;
	rasterizerDesc.SlopeScaledDepthBias = 0;
	rasterizerDesc.DepthClipEnable = true;
	rasterizerDesc.MultisampleEnable = true;
	rasterizerDesc.AntialiasedLineEnable = false;
	rasterizerDesc.ScissorEnable = false;
	ID3D11RasterizerState* rsState;
	pDevice->CreateRasterizerState(&rasterizerDesc, &rsState);

	bool depthOnly = (psoParams.renderTargetSet && psoParams.renderTargetSet->m_NumResources == 1 && psoParams.renderTargetSet->m_HasDepth);
	//bool hasDepth = (psoParams.renderTargetSet && psoParams.renderTargetSet->m_HasDepth);

	result->VS = (ID3D11VertexShader*)vertexShader;
	result->PS = (ID3D11PixelShader*)pixelShader;
	result->blendState = blendState;
	result->depthStencilState = depthState;
	result->sampleMask = UINT_MAX;
	result->rasterizerState = rsState;
	result->primitiveTopology = *psoParams.primitiveTopology;
	result->numRTVs = depthOnly ? 0 : 1;
	result->bp = EI_BP_GRAPHICS;

	return std::unique_ptr<EI_PSO>(result);
}

void EI_Device::BeginRenderPass(EI_CommandContext& commandContext, const EI_RenderTargetSet* pRenderTargetSet, const wchar_t* pPassName, uint32_t width /*= 0*/, uint32_t height /*= 0*/)
{
	UNREFERENCED_PARAMETER(commandContext);
	UNREFERENCED_PARAMETER(pPassName);
	UNREFERENCED_PARAMETER(width);
	UNREFERENCED_PARAMETER(height);
	logger::info("Begin render pass");
	assert(pRenderTargetSet->m_NumResources == 1 || (pRenderTargetSet->m_NumResources == 2 && pRenderTargetSet->m_HasDepth) && "Currently only support 1 render target with (or without) depth");
	const D3D11_CLEAR_VALUE* pDepthClearValue = nullptr;
	uint32_t                 numRenderTargets = 0;
	// This is a depth render
	if (pRenderTargetSet->m_NumResources == 1 && pRenderTargetSet->m_HasDepth) {
		if (pRenderTargetSet->m_ClearDepth)
			pDepthClearValue = &pRenderTargetSet->m_ClearValues[0];
	} else {
		++numRenderTargets;
	}

	if (pRenderTargetSet->m_HasDepth && pRenderTargetSet->m_NumResources > 1) {
		if (pRenderTargetSet->m_ClearDepth)
			pDepthClearValue = &pRenderTargetSet->m_ClearValues[1];
	}

	auto pContext = SkyrimGPUResourceManager::GetInstance()->m_pContext;
	pContext->OMSetRenderTargets(numRenderTargets, pRenderTargetSet->m_renderTargets, pRenderTargetSet->m_depthView);

	// Do we need to clear?
	if (numRenderTargets && pRenderTargetSet->m_ClearColor[0])
		pContext->ClearRenderTargetView(pRenderTargetSet->m_renderTargets[0], pRenderTargetSet->m_ClearValues[0].Color);

	if (pDepthClearValue)
		pContext->ClearDepthStencilView(pRenderTargetSet->m_depthView, D3D11_CLEAR_DEPTH, pDepthClearValue->DepthStencil.Depth, pDepthClearValue->DepthStencil.Stencil);
}

void EI_Device::EndRenderPass(EI_CommandContext& commandContext)
{
	UNREFERENCED_PARAMETER(commandContext);
	//TODO: Do we need to unbind anything?
}

void EI_Device::DrawFullScreenQuad(EI_CommandContext& commandContext, EI_PSO& pso, EI_BindSet** bindSets, uint32_t numBindSets)
{
	logger::info("draw fullscreen quad");
	// Set everything
	commandContext.BindSets(&pso, numBindSets, bindSets);

	EI_IndexedDrawParams drawParams;
	drawParams.pIndexBuffer = m_pFullscreenIndexBuffer.get();
	drawParams.numIndices = 4;
	drawParams.numInstances = 1;

	commandContext.DrawIndexedInstanced(pso, drawParams);
}

std::unique_ptr<EI_Resource> EI_Device::CreateSampler(EI_Filter MinFilter, EI_Filter MaxFilter, EI_Filter MipFilter, EI_AddressMode AddressMode)
{
	logger::info("CreateSampler");
	EI_Resource* res = new EI_Resource;
	res->m_ResourceType = EI_ResourceType::Sampler;

	D3D11_FILTER Filter;

	if (MinFilter == EI_Filter::Linear) {
		if (MaxFilter == EI_Filter::Linear) {
			if (MipFilter == EI_Filter::Linear)
				Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
			else
				Filter = D3D11_FILTER_MIN_MAG_LINEAR_MIP_POINT;
		} else {
			if (MipFilter == EI_Filter::Linear)
				Filter = D3D11_FILTER_MIN_LINEAR_MAG_POINT_MIP_LINEAR;
			else
				Filter = D3D11_FILTER_MIN_LINEAR_MAG_MIP_POINT;
		}
	} else {
		if (MaxFilter == EI_Filter::Linear) {
			if (MipFilter == EI_Filter::Linear)
				Filter = D3D11_FILTER_MIN_POINT_MAG_MIP_LINEAR;
			else
				Filter = D3D11_FILTER_MIN_POINT_MAG_LINEAR_MIP_POINT;
		} else {
			if (MipFilter == EI_Filter::Linear)
				Filter = D3D11_FILTER_MIN_MAG_POINT_MIP_LINEAR;
			else
				Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
		}
	}

	D3D11_SAMPLER_DESC SamplerDesc = {};
	SamplerDesc.Filter = Filter;
	SamplerDesc.AddressU = (AddressMode == EI_AddressMode::Wrap) ? D3D11_TEXTURE_ADDRESS_WRAP : D3D11_TEXTURE_ADDRESS_CLAMP;
	SamplerDesc.AddressV = (AddressMode == EI_AddressMode::Wrap) ? D3D11_TEXTURE_ADDRESS_WRAP : D3D11_TEXTURE_ADDRESS_CLAMP;
	SamplerDesc.AddressW = (AddressMode == EI_AddressMode::Wrap) ? D3D11_TEXTURE_ADDRESS_WRAP : D3D11_TEXTURE_ADDRESS_CLAMP;
	SamplerDesc.MipLODBias = 0;
	SamplerDesc.MaxAnisotropy = 1;
	SamplerDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
	SamplerDesc.BorderColor[0] = 0.f;
	SamplerDesc.BorderColor[1] = 0.f;
	SamplerDesc.BorderColor[2] = 0.f;
	SamplerDesc.BorderColor[3] = 0.f;
	SamplerDesc.MinLOD = 0.f;
	SamplerDesc.MaxLOD = D3D11_FLOAT32_MAX;

	logger::info("Creating sampler");
	auto pDevice = SkyrimGPUResourceManager::GetInstance()->m_pDevice;
	pDevice->CreateSamplerState(&SamplerDesc, &res->m_pSampler);
	res->samplerDesc = SamplerDesc;
	res->name = "sampler";
	return std::unique_ptr<EI_Resource>(res);
}

std::unique_ptr<EI_Resource> EI_Device::CreateDepthResource(const int width, const int height, const char* name) {
	EI_Resource* res = new EI_Resource;
	res->m_ResourceType = EI_ResourceType::Texture;
	res->name = name;
	auto pDevice = SkyrimGPUResourceManager::GetInstance()->m_pDevice;

	D3D11_TEXTURE2D_DESC desc;
	desc.Width = width;
	desc.Height = height;
	desc.MipLevels = 1;
	desc.ArraySize = 1;
	desc.Format = DXGI_FORMAT_R32_TYPELESS;
	desc.SampleDesc.Count = 1;
	desc.SampleDesc.Quality = 0;
	desc.Usage = D3D11_USAGE_DEFAULT;
	desc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
	desc.CPUAccessFlags = 0;
	desc.MiscFlags = 0;
	HRESULT hr = pDevice->CreateTexture2D(&desc, NULL, &res->m_pTexture);
	if (FAILED(hr)) {
		logger::error("depth texture creation failed");
		Util::PrintAllD3D11DebugMessages();
	}

	D3D11_SHADER_RESOURCE_VIEW_DESC viewDesc;
	viewDesc.Format = DXGI_FORMAT_R32_TYPELESS;
	viewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	viewDesc.Texture2D.MipLevels = 1;
	viewDesc.Texture2D.MostDetailedMip = 0;
	hr = pDevice->CreateShaderResourceView(res->m_pTexture, &viewDesc, &res->SRView);
	if (FAILED(hr)) {
		logger::error("Depth SRV creation failed");
		Util::PrintAllD3D11DebugMessages();
	}

	D3D11_DEPTH_STENCIL_VIEW_DESC stencilDesc;
	stencilDesc.Flags = 0;
	stencilDesc.Format = DXGI_FORMAT_R32_TYPELESS;
	stencilDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
	stencilDesc.Texture2D.MipSlice = 0;

	hr = pDevice->CreateDepthStencilView(res->m_pTexture, &stencilDesc, &res->DSView);
	if (FAILED(hr)) {
		logger::error("DSV creation failed");
		Util::PrintAllD3D11DebugMessages();
	}
	return std::unique_ptr<EI_Resource>(res);
}

void EI_Device::OnCreate()
{
	logger::info("Create fullscreen index buffer");
	m_pFullscreenIndexBuffer = CreateBufferResource(sizeof(uint32_t), 4, EI_BF_INDEXBUFFER, "FullScreenIndexBuffer");
	// finish creating the index buffer
	uint32_t indexArray[] = { 0, 1, 2, 3 };
	m_currentCommandContext.UpdateBuffer(m_pFullscreenIndexBuffer.get(), indexArray);

	logger::info("Create default texture");
	m_DefaultWhiteTexture = CreateResourceFromFile("DefaultWhite.png", true);
	m_LinearWrapSampler = CreateSampler(EI_Filter::Linear, EI_Filter::Linear, EI_Filter::Linear, EI_AddressMode::Wrap);
	EI_BindSetDescription bindSetDesc = { {
		m_LinearWrapSampler.get(),
	} };
	logger::info("Creating sampler bindset");
	m_pSamplerBindSet = CreateBindSet(GetSamplerLayout(), bindSetDesc);
	// Create shadow buffer. Because GLTF only allows us 1 buffer, we are going to create a HUGE one and divy it up as needed.
	m_shadowBuffer = CreateDepthResource(4096, 4096, "Shadow Buffer");
}

void EI_Device::GetTimeStamp(const char* name)
{
	UNREFERENCED_PARAMETER(name);
	//???
}

void EI_Device::CreateColorAndDepthResources(ID3D11RenderTargetView* pColorTextureView, ID3D11DepthStencilView* pDepthStencilView)
{
	EI_Resource* pColorResource = new EI_Resource;
	EI_Resource* pDepthResource = new EI_Resource;

	pColorResource->name = "Main game RTV";
	pColorResource->RTView = pColorTextureView;
	ID3D11Resource* pColorTexture;
	pColorTextureView->GetResource(&pColorTexture);
	pColorResource->m_pTexture = (ID3D11Texture2D*)pColorTexture;

	pDepthResource->name = "Main game DSV";
	pDepthResource->DSView = pDepthStencilView;
	ID3D11Resource* pDepthTexture;
	pDepthStencilView->GetResource(&pDepthTexture);
	pDepthResource->m_pTexture = (ID3D11Texture2D*)pDepthTexture;

	m_colorBuffer = std::unique_ptr<EI_Resource>(pColorResource);
	m_depthBuffer = std::unique_ptr<EI_Resource>(pDepthResource);
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
	//logger::info("Setting resources");
	for (uint32_t i = 0; i < m_NumResources; ++i) {
		m_RenderResources[i] = pResourcesArray[i];
	}
	for (uint32_t i = 0; i < m_NumResources - 1; i++) {
		m_renderTargets[i] = pResourcesArray[i]->RTView;
	}
	if (m_HasDepth) {
		m_depthView = pResourcesArray[m_NumResources - 1]->DSView;
	} else if (m_NumResources > 0) {
		m_renderTargets[m_NumResources - 1] = pResourcesArray[m_NumResources - 1]->RTView;
	}
}

EI_RenderTargetSet::~EI_RenderTargetSet()
{
	// Nothing to clean up
}

void EI_CommandContext::UpdateBuffer(EI_Resource* res, void* data)
{
	//logger::info("Updating buffer: {}", res->name);
	auto pContext = SkyrimGPUResourceManager::GetInstance()->m_pContext;

	//logger::info("New data address: {}", Util::ptr_to_string(data));
	//logger::info("buffer address: {}", Util::ptr_to_string(res->m_pBuffer));
	//logger::info("total mem size: {}", res->m_totalMemSize);

	pContext->UpdateSubresource(res->m_pBuffer, 0, nullptr, data, res->m_totalMemSize, res->m_totalMemSize);
}

UINT zeros[8] = {0,0,0,0,0,0,0,0};
void EI_CommandContext::BindSets(EI_PSO* pso, int numBindSets, EI_BindSet** bindSets)
{
	UNREFERENCED_PARAMETER(pso);
	//logger::info("BindSets");
	auto pContext = SkyrimGPUResourceManager::GetInstance()->m_pContext;

	std::vector<ID3D11Buffer*>             vsCbuffers;
	std::vector<ID3D11ShaderResourceView*> vsSRVs;

	std::vector<ID3D11Buffer*>              psCbuffers;
	std::vector<ID3D11ShaderResourceView*>  psSRVs;
	std::vector<ID3D11UnorderedAccessView*> psUAVs;

	std::vector<ID3D11SamplerState*> samplers;

	std::vector<ID3D11Buffer*> csCbuffers;
	std::vector<ID3D11UnorderedAccessView*> csUAVs;
	std::vector<ID3D11ShaderResourceView*> csSRVs;

	bool isCS = false;
	if (pso->bp == EI_BP_COMPUTE)
		isCS = true;

	//logger::info("assembling resource lists");
	//logger::info("{} BindSets", numBindSets);
	for (int i = 0; i < numBindSets; i++) {
		auto bindSet = bindSets[i];
		//logger::info("BindSet address: {}", Util::ptr_to_string(bindSet));
		//logger::info("Inserting {} samplers", bindSet->samplers.size());
		samplers.insert(samplers.end(), bindSet->samplers.begin(), bindSet->samplers.end());
		switch (bindSet->stage) {
		case EI_VS:
			//logger::info("inserting VS");
			vsCbuffers.insert(vsCbuffers.end(), bindSet->cbuffers.begin(), bindSet->cbuffers.end());
			vsSRVs.insert(vsSRVs.end(), bindSet->srvs.begin(), bindSet->srvs.end());
			break;
		case EI_PS:
			//logger::info("inserting PS");
			psCbuffers.insert(psCbuffers.end(), bindSet->cbuffers.begin(), bindSet->cbuffers.end());
			psSRVs.insert(psSRVs.end(), bindSet->srvs.begin(), bindSet->srvs.end());
			psUAVs.insert(psUAVs.end(), bindSet->uavs.begin(), bindSet->uavs.end());
			break;
		case EI_ALL:
			//logger::info("Inserting ALL");
			if (bindSet->cbuffers.size() > 0) {
				vsCbuffers.insert(vsCbuffers.end(), bindSet->cbuffers.begin(), bindSet->cbuffers.end());
				psCbuffers.insert(psCbuffers.end(), bindSet->cbuffers.begin(), bindSet->cbuffers.end());
			}
			if (bindSet->srvs.size() > 0) {
				vsSRVs.insert(vsSRVs.end(), bindSet->srvs.begin(), bindSet->srvs.end());
				psSRVs.insert(psSRVs.end(), bindSet->srvs.begin(), bindSet->srvs.end());
			}
			break;
		case EI_CS:
			//logger::info("Inserting CS");
			isCS = true;
			csCbuffers.insert(csCbuffers.end(), bindSet->cbuffers.begin(), bindSet->cbuffers.end());
			csUAVs.insert(csUAVs.end(), bindSet->uavs.begin(), bindSet->uavs.end());
			csSRVs.insert(csSRVs.end(), bindSet->srvs.begin(), bindSet->srvs.end());
			break;
		}
		//logger::info("Done with BindSet {}", i+1);
	}
	//logger::info("Done building lists");
	if (isCS) {
		logger::info("Binding CS resources: {} UAVs, {} SRVs, {} constant buffers, {} samplers",csUAVs.size(), csSRVs.size(), csCbuffers.size(), samplers.size());
		pContext->CSSetShaderResources(0, (UINT)csSRVs.size(), csSRVs.data());
		pContext->CSSetUnorderedAccessViews(0, (UINT)csUAVs.size(), csUAVs.data(), zeros);
		pContext->CSSetConstantBuffers(0, (UINT)csCbuffers.size(), csCbuffers.data());
		pContext->CSSetSamplers(0, (UINT)samplers.size(), samplers.data());
		return;
	} else {
		BindPSO(pso);
	}

	if (samplers.size() > 0) {
		//logger::info("Binding samplers");
		pContext->VSSetSamplers(0, (UINT)samplers.size(), samplers.data());
		pContext->PSSetSamplers(0, (UINT)samplers.size(), samplers.data());
	}

	//logger::info("Binding VS resources");
	if (vsCbuffers.size()>0)
		pContext->VSSetConstantBuffers(0, (UINT)vsCbuffers.size(), vsCbuffers.data());
	if (vsSRVs.size() > 0)
		pContext->VSSetShaderResources(0, (UINT)vsSRVs.size(), vsSRVs.data());

	//logger::info("Binding PS resources");
	if (psCbuffers.size()>0)
		pContext->PSSetConstantBuffers(0, (UINT)psCbuffers.size(), psCbuffers.data());
	if (psSRVs.size()>0)
		pContext->PSSetShaderResources(0, (UINT)psSRVs.size(), psSRVs.data());
	
	//logger::info("Setting UAVs");
	if (psUAVs.size() >= 0)
		pContext->OMSetRenderTargetsAndUnorderedAccessViews(D3D11_KEEP_RENDER_TARGETS_AND_DEPTH_STENCIL, nullptr, nullptr, 0, (UINT)psUAVs.size(), psUAVs.data(), zeros);

	//logger::info("Done binding");
}

void EI_CommandContext::DrawIndexedInstanced(EI_PSO& pso, EI_IndexedDrawParams& drawParams)
{
	UNREFERENCED_PARAMETER(pso);
	logger::info("DrawIndexedInstanced");
	auto pContext = SkyrimGPUResourceManager::GetInstance()->m_pContext;
	pContext->IASetIndexBuffer(drawParams.pIndexBuffer->m_pBuffer, DXGI_FORMAT_R32_UINT, 0);
	pContext->DrawIndexedInstanced(drawParams.pIndexBuffer->m_indexBufferNumIndices, drawParams.numInstances, 0, 0, 0);
}

void EI_CommandContext::DrawInstanced(EI_PSO& pso, EI_DrawParams& drawParams)
{
	UNREFERENCED_PARAMETER(pso);
	logger::info("DrawIndexed");
	auto pContext = SkyrimGPUResourceManager::GetInstance()->m_pContext;
	pContext->DrawInstanced(drawParams.numVertices, drawParams.numInstances, 0, 0);
}

void EI_CommandContext::SubmitBarrier(int numBarriers, EI_Barrier* barriers)
{
	UNREFERENCED_PARAMETER(numBarriers);
	UNREFERENCED_PARAMETER(barriers);
	//do we need anything?
	//logger::info("SubmitBarriers");
}

void EI_CommandContext::ClearUint32Image(EI_Resource* res, uint32_t value)
{
	//logger::info("Resource address: {}", Util::ptr_to_string(res));
	//logger::info("ClearUint32: {}", res->name);
	assert(res->m_ResourceType == EI_ResourceType::Buffer && "Trying to clear a non-UAV resource");
	auto   pContext = SkyrimGPUResourceManager::GetInstance()->m_pContext;
	UINT32 values[4] = { value, value, value, value };
	//logger::info("clear value: {}", value);
	//logger::info("texture address: {}", Util::ptr_to_string(res->m_pTexture));
	//logger::info("dimensions: {}, {}", res->m_textureWidth, res->m_textureHeight);
	pContext->ClearUnorderedAccessViewUint(res->UAView, values);
	//logger::info("After clear");
}

void EI_CommandContext::BindPSO(EI_PSO* pso)
{
	//logger::info("Bind PSO");
	//TFX never calls this method directly for graphics for some reason, only for compute?
	//will put it in BindSets() I guess...
	auto pContext = SkyrimGPUResourceManager::GetInstance()->m_pContext;
	if (pso->bp == EI_BP_GRAPHICS) {
		pContext->IASetPrimitiveTopology(pso->primitiveTopology);
		pContext->RSSetState(pso->rasterizerState);
		//do we need blend factors?
		pContext->OMSetBlendState(pso->blendState, nullptr, pso->sampleMask);
		//need different stencil ref?
		pContext->OMSetDepthStencilState(pso->depthStencilState, 0);
		logger::info("Setting vertex and pixel shaders");
		logger::info("VS address: {}", Util::ptr_to_string(pso->VS));
		logger::info("PS address: {}", Util::ptr_to_string(pso->PS));
		pContext->VSSetShader(pso->VS, nullptr, 0);
		pContext->PSSetShader(pso->PS, nullptr, 0);
	} else {
		logger::info("Setting compute shader");
		pContext->CSSetShader(pso->CS, nullptr, 0);
	}
}

void EI_CommandContext::Dispatch(int numGroups)
{
	auto pContext = SkyrimGPUResourceManager::GetInstance()->m_pContext;
	pContext->Dispatch(numGroups, 1, 1);
}
