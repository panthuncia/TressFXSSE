#include "SkyrimGPUInterface.h"
#include "Buffer.h"
void printHResult(HRESULT hr);
//////////////////////////////////////////////////////////////////////////////////////////////
///   Helper functions.
//////////////////////////////////////////////////////////////////////////////////////////////
//handles transfering staging buffer data to UAV
static void Transition(
	EI_Resource* pResource,
	AMD::EI_ResourceState  from,
	AMD::EI_ResourceState  to)
{
	//TODO disable warning
	pResource = pResource;
	if (from == to)
	{
		if (from == AMD::EI_STATE_UAV)
		{
			//pResource->resource->UAVBarrier();
		}
		else
		{
			logger::warn("transition from {} to {}", (int)from, (int)to);
		}
	}
	else if (from == AMD::EI_STATE_COPY_DEST && (to == AMD::EI_STATE_UAV || to == AMD::EI_STATE_NON_PS_SRV))
	{
		ID3D11Device* device = RE::BSRenderManager::GetSingleton()->GetRuntimeData().forwarder;
		D3D11_SUBRESOURCE_DATA data;
		data.pSysMem = pResource->data;
		logger::info("creating buffer with data");
		HRESULT hr = device->CreateBuffer(&(pResource->desc), &data, &(pResource->resource));
		if (hr != S_OK) {
			printHResult(hr);
		}
	}
}
// Generates a name from object and resource name to create a unique, single string.  
// For example, "ruby.positions" or just "positions".
static void GenerateCompositeName(std::string& fullName, const char* resourceName, const char* objectName)
{
	SU_ASSERT(resourceName != nullptr && objectName != nullptr);

	if (strlen(objectName) > 0)
		fullName = std::format("{}.{}", resourceName, objectName);
	else
		fullName = std::format("{}", resourceName);
}
// This is just a helper function right now.
EI_Resource* CreateSB(EI_Device* pContext,
	const AMD::uint32 structSize,
	const AMD::uint32 structCount,
	EI_StringHash     resourceName,
	EI_StringHash objectName,
	bool              bUAV,
	bool              bCounter)
{
	SU_ASSERT(!bCounter ||
		(bCounter && bUAV));  // you can't have a counter if you don't also have a UAV.
	//TODO fix
	bCounter = bCounter;
	pContext = pContext;
	EI_Resource* pSB = new EI_Resource;

	EI_Resource& r = *pSB;

	//    r.resource = SuGPUStructuredBufferPtr(0);
	//    r.uav      = SuGPUUnorderedAccessViewPtr(0);
	//    r.srv      = SuGPUSamplingResourceViewPtr(0);

	std::string strHash;
	GenerateCompositeName(strHash, resourceName, objectName);

	//SkyrimGPUResourceManager* pManager = (SkyrimGPUResourceManager*)pContext;

	//ID3D11Buffer* sbPtr = pManager->CreateStructuredBuffer(
	//	(uint32_t)structSize,
	//	(uint32_t)structCount,
	//	(bUAV ? D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS
	//		: D3D11_BIND_SHADER_RESOURCE),
	//	NULL,  // initial data.  We will map and upload explicitly.
	//	strHash,
	//	0,
	//	0);
	logger::info("Creating buffer desc");
	//D3D11_BUFFER_DESC desc = StructuredBufferDesc(structCount, structSize, bUAV, false, true);
	D3D11_BUFFER_DESC nextDesc = StructuredBufferDesc(structCount, structSize, bUAV, false);
	r.desc = nextDesc;
	r.structCount = structCount;
	r.structSize = structSize;

	SU_ASSERT(r.resource);

	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
	srvDesc.Format = DXGI_FORMAT_UNKNOWN;
	srvDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
	//pManager->device->CreateShaderResourceView(sbPtr->GetResource(), &srvDesc, &r.srv);
	//SU_ASSERT(r.srv);

	/*if (bUAV)
	{
		D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc;
		uavDesc.Format = DXGI_FORMAT_UNKNOWN;
		uavDesc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
		uavDesc.Buffer.FirstElement = 0;
		uavDesc.Buffer.NumElements = structCount;
		if (bCounter) {
			uavDesc.Buffer.Flags = D3D11_BUFFER_UAV_FLAG_COUNTER;
		}
		logger::info("Creating UAV");
		pManager->device->CreateUnorderedAccessView(sbPtr->GetResource(), &uavDesc, &r.uav);
		logger::info("Created UAV");

		SU_ASSERT(r.uav);
	}*/
	if (bUAV) {
		r.hasUAV = true;
	}
	return pSB;
}

extern "C"
{

	//EI_Resource* SuCreate2D(EI_Device* pContext,
	//	const size_t     width,
	//	const size_t     height,
	//	const size_t     arraySize,
	//	EI_StringHash    strHash)
	//{
	//	(void)pContext;

	//	EI_Resource* pRW2D = new EI_Resource;

	//	SuGPUTexture2DArrayPtr texPtr = SuGPUResourceManager::GetPtr()->CreateTexture2DArray(SuGPUResource::GPU_RESOURCE_DYNAMIC,
	//		SuGPUResource::BIND_RENDER_TARGET |
	//		SuGPUResource::BIND_SHADER_RESOURCE |
	//		SuGPUResource::BIND_UNORDERED_ACCESS,
	//		0,
	//		SuGPUResourceFormat::SU_FORMAT_R32_UINT,
	//		SuGPUTexture::MIP_NONE,
	//		1,  // Mip levels
	//		(uint16)width,
	//		(uint16)height,
	//		(uint16)arraySize,
	//		1,
	//		0,  // sample count and quality
	//		SuMemoryBufferPtr(0),
	//		strHash);

	//	pRW2D->resource = texPtr.cast<SuGPUResource>();
	//	pRW2D->srv = texPtr->GetDefaultSamplingView();
	//	SuGPUResourceDescription     desc = texPtr->GetDefaultResourceDesc();
	//	SuGPUResourceViewDescription viewDesc =
	//		SuGPUResourceViewDescription(SU_UNORDERED_ACCESS_VIEW,
	//			texPtr->GetFormat(),
	//			SuGPUResource::GPU_RESOURCE_TEXTURE_2D_ARRAY,
	//			desc);
	//	pRW2D->uav = texPtr->GetUnorderedAccessView(viewDesc);
	//	pRW2D->rtv = SuGPURenderableResourceViewPtr(0);

	//	return pRW2D;
	//}

	//EI_Resource* SuCreateRT(EI_Device* pContext,
	//	const size_t     width,
	//	const size_t     height,
	//	const size_t     channels,
	//	EI_StringHash    strHash,
	//	float            clearR,
	//	float            clearG,
	//	float            clearB,
	//	float            clearA)
	//{
	//	EI_Resource* pRW2D = new EI_Resource;

	//	SuGPUResourceFormat format;
	//	if (channels == 1)
	//	{
	//		format = SU_FORMAT_R16_FLOAT;
	//	}
	//	else if (channels == 2)
	//	{
	//		format = SU_FORMAT_R16G16_FLOAT;
	//	}
	//	else if (channels == 4)
	//	{
	//		format = SU_FORMAT_R16G16B16A16_FLOAT;
	//	}

	//	SuGPUTexture2DArrayClearInfo clearInfo;
	//	clearInfo.viewFormat = format;
	//	clearInfo.color = SuVector4(clearR, clearG, clearB, clearA);

	//	SuGPUTexture2DArrayPtr texPtr = SuGPUResourceManager::GetPtr()->CreateTexture2DArray(SuGPUResource::GPU_RESOURCE_DYNAMIC,
	//		SuGPUResource::BIND_RENDER_TARGET |
	//		SuGPUResource::BIND_SHADER_RESOURCE,
	//		0,
	//		format,
	//		SuGPUTexture::MIP_NONE,
	//		1,  // Mip levels
	//		(uint16)width,
	//		(uint16)height,
	//		1,
	//		1,
	//		0,  // sample count and quality
	//		SuMemoryBufferPtr(0),
	//		strHash,
	//		NULL,
	//		0,
	//		&clearInfo);

	//	pRW2D->resource = texPtr.cast<SuGPUResource>();

	//	pRW2D->srv = texPtr->GetDefaultSamplingView();
	//	pRW2D->uav = SuGPUUnorderedAccessViewPtr(0);
	//	SuGPUResourceDescription     desc = pRW2D->resource->GetDefaultResourceDesc();
	//	SuGPUResourceViewDescription viewDesc =
	//		SuGPUResourceViewDescription(SU_RENDERABLE_VIEW,
	//			texPtr->GetFormat(),
	//			SuGPUResource::GPU_RESOURCE_TEXTURE_2D_ARRAY,
	//			desc);
	//	pRW2D->rtv = texPtr->GetRenderableView(viewDesc);

	//	return pRW2D;
	//}

	//EI_BindLayout* SuCreateLayout(EI_Device* pDevice, EI_LayoutManagerRef layoutManager, const AMD::TressFXLayoutDescription& description)
	//{
	//	(void)pDevice;

	//	EI_BindLayout* pLayout = new EI_BindLayout;

	//	SuEffectPtr pEffect = GetEffect(layoutManager);


	//	for (int i = 0; i < description.nSRVs; ++i)
	//	{
	//		pLayout->srvs.push_back(pEffect->GetTextureSlot(description.srvNames[i]));
	//	}

	//	for (int i = 0; i < description.nUAVs; ++i)
	//	{
	//		const SuUAVSlot* pSlot = pEffect->GetUAVSlot(description.uavNames[i]);
	//		SU_ASSERT(pSlot != nullptr);
	//		pLayout->uavs.push_back(pSlot);
	//	}

	//	(void)description.constants.constantBufferName; // Sushi doesn't use constant buffer names.  It sets individually.

	//	for (int i = 0; i < description.constants.nConstants; i++)
	//	{
	//		pLayout->constants.push_back(pEffect->GetParameter(description.constants.parameterNames[i]));
	//	}


	//	return pLayout;

	//}

	//void SuDestroyLayout(EI_Device* pDevice, EI_BindLayout* pLayout)
	//{
	//	(void)pDevice;
	//	AMD_SAFE_DELETE(pLayout);
	//}

	// Creates structured buffer. SRV only.  Begin state should be Upload.
	EI_Resource* CreateReadOnlySB(EI_Device* pContext,
		const AMD::uint32 structSize,
		const AMD::uint32 structCount,
		EI_StringHash     resourceName, EI_StringHash objectName)
	{
		return CreateSB(pContext, structSize, structCount, resourceName, objectName, false, false);
	}


	// Creates read/write structured buffer. has UAV so begin state should be UAV.
	EI_Resource* CreateReadWriteSB(EI_Device* pContext,
		const AMD::uint32 structSize,
		const AMD::uint32 structCount,
		EI_StringHash     resourceName, EI_StringHash objectName)
	{
		return CreateSB(pContext, structSize, structCount, resourceName, objectName, true, false);
	}

	// Creates read/write structured buffer with a counter. has UAV so begin state should be UAV.
	EI_Resource* CreateCountedSB(EI_Device* pContext,
		const AMD::uint32 structSize,
		const AMD::uint32 structCount,
		EI_StringHash     resourceName, EI_StringHash objectName)
	{
		return CreateSB(pContext, structSize, structCount, resourceName, objectName, true, true);
	}

	/*void SuDestroy(EI_Device* pDevice, EI_Resource* pRW2D)
	{
		SuGPUResourceManager* pResourceManager = GetResourceManager(pDevice);

		pRW2D->uav = SuGPUUnorderedAccessViewPtr(0);
		pRW2D->srv = SuGPUSamplingResourceViewPtr(0);
		pRW2D->rtv = SuGPURenderableResourceViewPtr(0);
		pRW2D->resource = SuGPUResourcePtr(0);
		delete pRW2D;
	}*/
	EI_BindSet* CreateBindSet(EI_Device* commandContext, AMD::TressFXBindSet& bindSet)
	{
		//TODO ignore
		commandContext = commandContext;
		logger::info("creating bind set");
		EI_BindSet* pBindSet = new EI_BindSet;

		// should just replace with a SuArray.  Same as constant buffers.

		pBindSet->nBytes = bindSet.nBytes;
		pBindSet->values = bindSet.values;

		// Maybe replace this with SuArray usage.

		pBindSet->nSRVs = bindSet.nSRVs;
		if (bindSet.nSRVs > 0)
		{
			pBindSet->srvs = new ID3D11ShaderResourceView*[bindSet.nSRVs];
			for (int i = 0; i < bindSet.nSRVs; ++i)
			{
				pBindSet->srvs[i] = bindSet.srvs[i]->srv;
			}
		}
		else
		{
			pBindSet->srvs = nullptr;
		}

		pBindSet->nUAVs = bindSet.nUAVs;
		if (pBindSet->nUAVs > 0)
		{
			pBindSet->uavs = new ID3D11UnorderedAccessView*[bindSet.nUAVs];
			for (int i = 0; i < bindSet.nUAVs; ++i)
			{
				pBindSet->uavs[i] = bindSet.uavs[i]->uav;
			}
		}
		else
		{
			pBindSet->uavs = nullptr;
		}

		return pBindSet;
	}

	void DestroyBindSet(EI_Device* pDevice, EI_BindSet* pBindSet)
	{
		//TODO ignore
		pDevice = pDevice;
		if (pBindSet->nSRVs > 0)
		{
			// should just call destructors,which should be setting to null.
			delete[] pBindSet->srvs;
		}

		if (pBindSet->nUAVs > 0)
		{
			delete[] pBindSet->uavs;
		}

		delete pBindSet;
	}
	void SubmitBarriers(EI_CommandContextRef context,
		int numBarriers,
		AMD::EI_Barrier* barriers)
	{
		(void)context;
		for (int i = 0; i < numBarriers; ++i)
		{
			Transition(barriers[i].pResource, barriers[i].from, barriers[i].to);
		}
	}
	// Map gets a pointer to upload heap / mapped memory.
   // Unmap issues the copy.
   // This is only ever used for the initial upload.
   //
	void* Map(EI_CommandContextRef pContext, EI_StructuredBuffer& sb)
	{
		pContext;
		////SU_ASSERT(sb.resource->GetType() == D3D11_RESOURCE_DIMENSION_BUFFER);
		//logger::info("mapping buffer");
		//D3D11_BUFFER_DESC desc;
		//sb.resource->GetDesc(&desc);
		//logger::info("byte width: {}", desc.ByteWidth);
		//logger::info("usage: {}", desc.Usage);
		//logger::info("bind flags: {}", desc.BindFlags);
		//logger::info("CPU access flags: {}", desc.CPUAccessFlags);
		////SuGPUBuffer* pBuffer = static_cast<SuGPUBuffer*>(sb.resource.get());
		//logger::info("Casting context");
		//ID3D11DeviceContext* ctx = RE::BSRenderManager::GetSingleton()->GetRuntimeData().context;
		//D3D11_MAPPED_SUBRESOURCE mapped_buffer{};
		//ZeroMemory(&mapped_buffer, sizeof(D3D11_MAPPED_SUBRESOURCE));
		//HRESULT hr;
		//hr = ctx->Map(sb.resource, 0u, D3D11_MAP_WRITE_DISCARD, 0u, &mapped_buffer);
		//logger::info("mapped resource");
		//if (hr != S_OK) {
		//	printHResult(hr);
		//}
		//return (void*)mapped_buffer.pData;
		sb.data = calloc(sb.structCount, sb.structSize);
		return sb.data;
	}

	bool Unmap(EI_CommandContextRef pContext, EI_StructuredBuffer& sb)
	{
		/*ID3D11DeviceContext* context = (ID3D11DeviceContext*)&pContext;
		logger::info("unmapping buffer");
		context->Unmap(sb.resource, 0);*/
		//TODO remove this warning...
		sb = sb;
		pContext;
		return true;
	}
	// Initialize and leave in state for use as index buffer.
   // Index buffers are for either triangle-strip hair, or line segments.
	EI_IndexBuffer* CreateIndexBuffer(EI_Device* context,
		AMD::uint32      indexCount,
		void* pInitialData, EI_StringHash objectName)
	{
		(void)objectName;
		//SuGPUIndexBufferPtr* pIBPtr = new SuGPUIndexBufferPtr;
		ID3D11Buffer* g_pIndexBuffer = NULL;
		//SuGPUIndexBufferPtr& ib = *pIBPtr;
		SkyrimGPUResourceManager* pResourceManager = (SkyrimGPUResourceManager*)context;
		//ib = SuGPUResourceManager::GetPtr()->CreateResourceIndex(SuGPUResource::GPU_RESOURCE_DYNAMIC, TRESSFX_INDEX_SIZE, indexCount, SuMemoryBufferPtr(0));
		D3D11_BUFFER_DESC bufferDesc;
		bufferDesc.Usage = D3D11_USAGE_DYNAMIC;
		bufferDesc.ByteWidth = TRESSFX_INDEX_SIZE * indexCount;
		bufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
		bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		bufferDesc.MiscFlags = 0;
		D3D11_SUBRESOURCE_DATA data;
		data.pSysMem = pInitialData;
		HRESULT hr = pResourceManager->device->CreateBuffer(&bufferDesc, &data, &g_pIndexBuffer);
		if (FAILED(hr))
			logger::error("index buffer creation failed");

		return (EI_IndexBuffer*)g_pIndexBuffer;
	}

	void DestroyIB(EI_Device* pDevice, EI_IndexBuffer* pBuffer)
	{
		//TODO fix? 
		pDevice = pDevice;
		pBuffer = pBuffer;
		//delete[] (ID3D11Buffer*)pBuffer;
	}
	void LogError(const char* msg) {
		logger::error("{}",msg);
	}
}
