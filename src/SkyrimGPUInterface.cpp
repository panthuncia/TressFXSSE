#include "SkyrimGPUInterface.h"
#include "Buffer.h"
#include "LOCAL_RE/BSGraphics.h"
#include <sstream>  //for std::stringstream
#include <string>   //for std::string
#include <DirectXMath.h>
void printHResult(HRESULT hr);

//////////////////////////////////////////////////////////////////////////////////////////////
///   Helper functions.
//////////////////////////////////////////////////////////////////////////////////////////////
void printEffectVariables(ID3DX11Effect* pEffect){
	D3DX11_EFFECT_DESC desc;
	pEffect->GetDesc(&desc);
	logger::info("Effect buffer count: {}", desc.ConstantBuffers);
	for (uint32_t i = 0; i < desc.ConstantBuffers; i++) {
		D3DX11_EFFECT_VARIABLE_DESC varDesc;
		ID3DX11EffectConstantBuffer* buffer = pEffect->GetConstantBufferByIndex(i);
		buffer->GetDesc(&varDesc);
		logger::info("{}", varDesc.Name);
	}
	logger::info("Effect global variables count: {}", desc.GlobalVariables);
	for (uint32_t i = 0; i < desc.GlobalVariables; i++) {
		D3DX11_EFFECT_VARIABLE_DESC  varDesc;
		ID3DX11EffectVariable* buffer = pEffect->GetVariableByIndex(i);
		buffer->GetDesc(&varDesc);
		logger::info("{}", varDesc.Name);
	}
}
std::string ptr_to_string(void* ptr) {
	const void* address = static_cast<const void*>(ptr);
	std::stringstream ss;
	ss << address;
	std::string name = ss.str();
	return name;
}
void PrintAllD3D11DebugMessages(ID3D11Device* d3dDevice)
{
	Microsoft::WRL::ComPtr<ID3D11Device> m_d3dDevice;
	*(m_d3dDevice.GetAddressOf()) = d3dDevice;
	Microsoft::WRL::ComPtr<ID3D11Debug> d3dDebug;
	Microsoft::WRL::ComPtr<ID3D11InfoQueue> d3dInfoQueue;
	HRESULT hr = m_d3dDevice.As(&d3dDebug);
	if (SUCCEEDED(hr)) {
		hr = d3dDebug.As(&d3dInfoQueue);
		if (SUCCEEDED(hr)) {
#	ifdef _DEBUG
			d3dInfoQueue->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_CORRUPTION, true);
			d3dInfoQueue->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_ERROR, true);
#	endif
			D3D11_MESSAGE_ID hide[] = {
				D3D11_MESSAGE_ID_SETPRIVATEDATA_CHANGINGPARAMS,
				// TODO: Add more message IDs here as needed
			};
			D3D11_INFO_QUEUE_FILTER filter = {};
			filter.DenyList.NumIDs = _countof(hide);
			filter.DenyList.pIDList = hide;
			d3dInfoQueue->AddStorageFilterEntries(&filter);
		}
	}
	//HANDLE_HRESULT is just a macro of mine to check for S_OK return value
	d3dInfoQueue->PushEmptyStorageFilter();
	UINT64 message_count = d3dInfoQueue->GetNumStoredMessages();

	for (UINT64 i = 0; i < message_count; i++) {
		SIZE_T message_size = 0;
		d3dInfoQueue->GetMessage(i, nullptr, &message_size);  //get the size of the message

		D3D11_MESSAGE* message = (D3D11_MESSAGE*)malloc(message_size);            //allocate enough space
		d3dInfoQueue->GetMessage(i, message, &message_size);  //get the actual message

		//do whatever you want to do with it
		logger::info("{}",message->pDescription);

		free(message);
	}

	d3dInfoQueue->ClearStoredMessages();
}
	//handles transfering staging buffer data to UAV
static void Transition(EI_CommandContextRef context,
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
			ID3D11DeviceContext* pContext = *((ID3D11DeviceContext**)(&context));
			ID3D11UnorderedAccessView* UAV_NULL = NULL;
			pContext->CSSetUnorderedAccessViews(3, 1, &UAV_NULL, 0);
		}
		else
		{
			logger::warn("transition from {} to {}", (int)from, (int)to);
		}
	}
	else if (from == AMD::EI_STATE_COPY_DEST && (to == AMD::EI_STATE_UAV || to == AMD::EI_STATE_NON_PS_SRV))
	{
		/*ID3D11Device* device = RE::BSRenderManager::GetSingleton()->GetRuntimeData().forwarder;
		D3D11_SUBRESOURCE_DATA data;
		data.pSysMem = pResource->data;
		logger::info("creating buffer with data");
		HRESULT hr = device->CreateBuffer(&(pResource->desc), &data, &(pResource->buffer));
		if (hr != S_OK) {
			printHResult(hr);
		}*/
	} else if (from == AMD::EI_STATE_UAV && to == AMD::EI_STATE_VS_SRV){
		logger::info("Transition CS->VS");
		ID3D11DeviceContext* pContext = *((ID3D11DeviceContext**)(&context)); 
		ID3D11UnorderedAccessView* UAV_NULL = NULL;
		pContext->CSSetUnorderedAccessViews(0, 1, &UAV_NULL, 0);
		pContext->CSSetUnorderedAccessViews(1, 1, &UAV_NULL, 0);
		pContext->IASetInputLayout(nullptr);

	} else if (from == AMD::EI_STATE_VS_SRV && to == AMD::EI_STATE_UAV) {
		logger::info("Transition VS->CS");
		ID3D11DeviceContext* pContext = *((ID3D11DeviceContext**)(&context));
		ID3D11ShaderResourceView* SRV_NULL = NULL;
		pContext->VSSetShaderResources(1, 1, &SRV_NULL);
		pContext->VSSetShaderResources(2, 1, &SRV_NULL);
	}
}

//void UnbindUAVs(){
//	pContext->OMSetRenderTargets(0, nullptr, nullptr);
//	pContext->CSSetUnorderedAccessViews(0, 0, nullptr, nullptr);
//	pContext->OMSetRenderTargetsAndUnorderedAccessViews(0, NULL, NULL, 0, 0, NULL, NULL);
//}

static void UpdateConstants(std::vector<ID3DX11EffectVariable*> cb, void* values, int nBytes)
{
	//logger::info("Update constants");
	
	uint8_t* pCurrent = (uint8_t*)values;
	for (AMD::int32 i = 0; i < cb.size(); ++i) {
		ID3DX11EffectVariable* pParam = cb[i];
		D3DX11_EFFECT_VARIABLE_DESC varDesc;
		pParam->GetDesc(&varDesc);
		D3DX11_EFFECT_TYPE_DESC typeDesc;
		pParam->GetType()->GetDesc(&typeDesc);
		uint32_t               nParamBytes = typeDesc.PackedSize;  //pParam->GetParameterSize();
		logger::info("Updating paramater {}, {} bytes", varDesc.Name, typeDesc.PackedSize);
		uint32_t             nCummulativeBytes = static_cast<uint32_t>(pCurrent - (uint8_t*)values) + nParamBytes;
		SU_ASSERT(nBytes >= 0);
		if (nCummulativeBytes > (uint32_t)nBytes) {
			logger::warn("Layout looking for {} bytes so far, but bindset only has {} bytes.",
				(uint32_t)nCummulativeBytes,
				(uint32_t)nBytes);
		}

		pParam->SetRawValue(pCurrent, 0, nParamBytes);
		pCurrent += nParamBytes;
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

	SkyrimGPUResourceManager* pManager = (SkyrimGPUResourceManager*)pContext;

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
	logger::info("creating buffer");
	HRESULT hr = pManager->m_pDevice->CreateBuffer(&nextDesc, NULL, &pSB->buffer);
	printHResult(hr);
	r.structCount = structCount;
	r.structSize = structSize;

	SU_ASSERT(r.buffer);

	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
	srvDesc.Format = DXGI_FORMAT_UNKNOWN;
	srvDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
	srvDesc.Buffer.FirstElement = 0;
	srvDesc.Buffer.ElementOffset = 0;
	srvDesc.Buffer.NumElements = structCount;
	srvDesc.Buffer.ElementWidth = structSize;
	logger::info("creating srv");
	HRESULT hr1 = pManager->m_pDevice->CreateShaderResourceView(pSB->buffer, &srvDesc, &r.srv);
	printHResult(hr1);
	//SU_ASSERT(r.srv);

	if (bUAV)
	{
		D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc;
		uavDesc.Format = DXGI_FORMAT_UNKNOWN;
		uavDesc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
		uavDesc.Buffer.FirstElement = 0;
		uavDesc.Buffer.NumElements = structCount;
		if (bCounter) {
			logger::info("adding uav counter");
			uavDesc.Buffer.Flags = D3D11_BUFFER_UAV_FLAG_COUNTER;
		} else {
			uavDesc.Buffer.Flags = 0;
		}
		logger::info("Creating UAV");
		HRESULT hr2 = pManager->m_pDevice->CreateUnorderedAccessView(pSB->buffer, &uavDesc, &r.uav);
		printHResult(hr2);
		const void*       uav_address = static_cast<const void*>(pSB->uav);
		std::stringstream uav_ss;
		uav_ss << uav_address;
		std::string uav_addr = uav_ss.str();
		logger::info("Addr. of uav: {}", uav_addr);
		SU_ASSERT(r.uav);
	}
	if (bUAV) {
		r.hasUAV = true;
	}
	const void* srv_address = static_cast<const void*>(pSB->srv);
	std::stringstream srv_ss;
	srv_ss << srv_address;
	std::string srv_addr = srv_ss.str();
	logger::info("Addr. of srv: {}", srv_addr);
	PrintAllD3D11DebugMessages(pManager->m_pDevice);
	return pSB;
}

extern "C"
{

	EI_Resource* Create2D(EI_Device* pContext,
		const size_t                   width,
		const size_t                   height,
		const size_t                   arraySize,
		EI_StringHash                  strHash)
	{
		(void)strHash;
		EI_Resource* pRW2D = new EI_Resource;
		SkyrimGPUResourceManager* pManager = (SkyrimGPUResourceManager*)pContext;

		//sampler desc
		DXGI_SAMPLE_DESC sampleDesc;
		sampleDesc.Count = 1;
		sampleDesc.Quality = 0;

		//create texture
		D3D11_TEXTURE2D_DESC texDesc;
		texDesc.Width = (UINT)width;
		texDesc.Height = (UINT)height;
		texDesc.MipLevels = 1;
		texDesc.ArraySize = (UINT)arraySize;
		texDesc.Format = DXGI_FORMAT_R32_SINT;
		texDesc.SampleDesc = sampleDesc;
		texDesc.Usage = D3D11_USAGE_DEFAULT;
		texDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS;
		texDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		texDesc.MiscFlags = 0;
		ID3D11Texture2D* texPtr;
		logger::info("Creating texture");
		pManager->m_pDevice->CreateTexture2D(&texDesc, NULL, &texPtr);

		//create SRV
		D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
		srvDesc.Format = DXGI_FORMAT_R32_SINT;
		/*srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DARRAY;
		srvDesc.Texture2DArray.MostDetailedMip = 0;
		srvDesc.Texture2DArray.MipLevels = 1;
		srvDesc.Texture2DArray.FirstArraySlice = 0;
		srvDesc.Texture2DArray.ArraySize = (UINT)arraySize;*/
		srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D; 
		srvDesc.Texture2D.MipLevels = 1;
		srvDesc.Texture2D.MostDetailedMip = 0;
		ID3D11ShaderResourceView* pSRV;
		logger::info("Creating SRV");
		pManager->m_pDevice->CreateShaderResourceView(texPtr, &srvDesc, &pSRV);

		//create UAV
		D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc;
		uavDesc.Format = DXGI_FORMAT_R32_SINT;
		/*uavDesc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2DARRAY;
		uavDesc.Texture2DArray.ArraySize = (UINT)arraySize;
		uavDesc.Texture2DArray.FirstArraySlice = 0;
		uavDesc.Texture2DArray.MipSlice = 0;*/
		uavDesc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2D;
		uavDesc.Texture2D.MipSlice = 0;
		ID3D11UnorderedAccessView* pUAV;
		logger::info("Creating UAV");
		pManager->m_pDevice->CreateUnorderedAccessView(texPtr, &uavDesc, &pUAV);

		pRW2D->texture = texPtr;
		pRW2D->srv = pSRV;
		pRW2D->uav = pUAV;
		pRW2D->rtv = NULL;

		return pRW2D;
	}

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

	EI_BindLayout* CreateLayout(EI_Device* pDevice, EI_LayoutManagerRef layoutManager, const AMD::TressFXLayoutDescription& description)
	{
		logger::info("In CreateLayout");
		(void)pDevice;

		EI_BindLayout* pLayout = new EI_BindLayout();
		pLayout->uavNames = description.uavNames;
		pLayout->srvNames = description.srvNames;
		ID3DX11Effect* pEffect = (ID3DX11Effect*)&layoutManager;
		for (int i = 0; i < description.nSRVs; ++i) {
			logger::info("Getting SRV: {}", description.srvNames[i]);
			auto var = pEffect->GetVariableByName(description.srvNames[i]);
			pLayout->srvs.push_back(var->AsShaderResource());
		}
		for (int i = 0; i < description.nUAVs; ++i) {
			ID3DX11EffectUnorderedAccessViewVariable* pSlot = pEffect->GetVariableByName(description.uavNames[i])->AsUnorderedAccessView();
			SU_ASSERT(pSlot != nullptr);
			pLayout->uavs.push_back(pSlot);
		}
		(void)description.constants.constantBufferName;  // Sushi doesn't use constant buffer names.  It sets individually.
		for (int i = 0; i < description.constants.nConstants; i++) {
			//logger::info("getting constant with name: {}", description.constants.parameterNames[i]);
			ID3DX11EffectVariable* var = pEffect->GetVariableByName(description.constants.parameterNames[i]);
			pLayout->constants.push_back(var);
			/*D3DX11_EFFECT_VARIABLE_DESC desc;
			var->GetDesc(&desc);
			logger::info("got constant with name: {}", desc.Name);*/

		}
		logger::info("Layout num UAVs: {}", pLayout->uavs.size());
		logger::info("Layout num SRVs: {}", pLayout->srvs.size());
		logger::info("Layout num constants: {}", pLayout->constants.size());
		return pLayout;
	}

	void DestroyLayout(EI_Device* pDevice, EI_BindLayout* pLayout)
	{
		(void)pDevice;
		AMD_SAFE_DELETE(pLayout);
	}

	// Creates structured buffer. SRV only.  Begin state should be Upload.
	EI_Resource* CreateReadOnlySB(EI_Device* pContext,
		const AMD::uint32 structSize,
		const AMD::uint32 structCount,
		EI_StringHash     resourceName, EI_StringHash objectName)
	{
		//TFX calls this read only and then wants to map it for writing...
		//return CreateSB(pContext, structSize, structCount, resourceName, objectName, false, false);
		return CreateReadWriteSB(pContext, structSize, structCount, resourceName, objectName);
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

	void Destroy(EI_Device* pDevice, EI_Resource* pRW2D)
	{
		logger::info("In destroy:");
		(void*)pDevice;
		if (pRW2D->uav != NULL)
			pRW2D->uav->Release();
		if (pRW2D->srv != NULL)
			pRW2D->srv->Release();
		if (pRW2D->rtv != NULL)
			pRW2D->rtv->Release();
		if (pRW2D->buffer != NULL)
			pRW2D->buffer->Release();
		if (pRW2D->texture != NULL)
			pRW2D->texture->Release();
		delete pRW2D;
	}
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
				const void*       address = static_cast<const void*>(bindSet.srvs[i]->srv);
				std::stringstream ss;
				ss << address;
				std::string name = ss.str(); 
				logger::info("Addr. of srv: {}", name);
				logger::info("Type of srv: {}", type_name<decltype(bindSet.srvs[i]->srv)>());
				/*D3D11_SHADER_RESOURCE_VIEW_DESC desc;
				pBindSet->srvs[i]->GetDesc(&desc);
				logger::info("Got format: {}", desc.Format);*/
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
		logger::info("Destroying bindset");
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
			Transition(context, barriers[i].pResource, barriers[i].from, barriers[i].to);
		}
	}
	// Map gets a pointer to upload heap / mapped memory.
   // Unmap issues the copy.
   // This is only ever used for the initial upload.
   //
	void* Map(EI_CommandContextRef pContext, EI_StructuredBuffer& sb)
	{
		ID3D11DeviceContext* pDeviceContext = *((ID3D11DeviceContext**)(&pContext)); 
		//SU_ASSERT(sb.resource->GetType() == D3D11_RESOURCE_DIMENSION_BUFFER);
		logger::info("mapping buffer");
		D3D11_BUFFER_DESC desc;
		sb.buffer->GetDesc(&desc);
		logger::info("byte width: {}", desc.ByteWidth);
		logger::info("usage: {}", desc.Usage);
		logger::info("bind flags: {}", desc.BindFlags);
		logger::info("CPU access flags: {}", desc.CPUAccessFlags);
		//SuGPUBuffer* pBuffer = static_cast<SuGPUBuffer*>(sb.resource.get());
		D3D11_MAPPED_SUBRESOURCE mapped_buffer{};
		ZeroMemory(&mapped_buffer, sizeof(D3D11_MAPPED_SUBRESOURCE));
		HRESULT hr;
		hr = pDeviceContext->Map(sb.buffer, 0u, D3D11_MAP_WRITE, 0u, &mapped_buffer);
		logger::info("mapped resource");
		if (hr != S_OK) {
			printHResult(hr);
		}
		auto manager = RE::BSRenderManager::GetSingleton();
		auto device = manager->GetRuntimeData().forwarder;
		PrintAllD3D11DebugMessages(device);
		return (void*)mapped_buffer.pData;
		//sb.data = calloc(sb.structCount, sb.structSize);
		//return sb.data;
	}

	bool Unmap(EI_CommandContextRef pContext, EI_StructuredBuffer& sb)
	{
		ID3D11DeviceContext* pDeviceContext = *((ID3D11DeviceContext**)(&pContext)); 
		pDeviceContext->Unmap(sb.buffer, 0u);
		logger::info("Unmapped buffer");
		//TODO remove this warning...
		/*sb = sb;
		pContext;*/
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
		logger::info("Creating index buffer with {} indices, {} bytes total", indexCount, bufferDesc.ByteWidth);
		bufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
		bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		bufferDesc.MiscFlags = 0;
		D3D11_SUBRESOURCE_DATA data;
		data.pSysMem = pInitialData;
		HRESULT hr = pResourceManager->m_pDevice->CreateBuffer(&bufferDesc, &data, &g_pIndexBuffer);
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
	void Bind(EI_CommandContextRef commandContext, EI_BindLayout* pLayout, EI_BindSet& set)
	{
		(void)commandContext;
		SU_ASSERT(set.nSRVs == pLayout->srvs.size());
		for (AMD::int32 i = 0; i < set.nSRVs; i++) {
			//pLayout->srvs[i]->BindResource(set.srvs[i]);
			D3D11_SHADER_RESOURCE_VIEW_DESC desc;

			set.srvs[i]->GetDesc(&desc);
			logger::info("Binding SRV: {}", pLayout->srvNames[i]);
			pLayout->srvs[i]->SetResource(set.srvs[i]);
		}
		SU_ASSERT(set.nUAVs == pLayout->uavs.size());
		for (AMD::int32 i = 0; i < set.nUAVs; i++) {
			logger::info("Binding UAV: {}", pLayout->uavNames[i]);
			pLayout->uavs[i]->SetUnorderedAccessView(set.uavs[i]);
		}
		UpdateConstants(pLayout->constants, set.values, set.nBytes);
	}

	EI_PSO* CreateComputeShaderPSO(EI_Device* pDevice,
		EI_LayoutManagerRef                     layoutManager,
		const EI_StringHash&                    shaderName)
	{
		(void)pDevice;
		ID3DX11Effect* pEffect = (ID3DX11Effect*)&layoutManager;
		// SuEffect& effect = (SuEffect&)layoutManager;
		//return const_cast<SuEffectTechnique*>(
		std::string prefix = "TressFXSimulation_";
		std::string shader_name = prefix.append(shaderName);
		ID3DX11EffectTechnique* pTechnique = pEffect->GetTechniqueByName(shader_name.c_str());
		EI_PSO* pso = new EI_PSO;
		pso->m_pEffect = pEffect;
		pso->m_pTechnique = pTechnique;
		return pso;	

	}
	// All our compute shaders have dimensions of (N,1,1)
	void Dispatch(EI_CommandContextRef commandContext, EI_PSO& pso, int nThreadGroups)
	{
		//SuEffectTechnique* pTechnique = pso;
		ID3DX11EffectTechnique* pTechnique = pso.m_pTechnique;
		ID3D11DeviceContext*    pContext = *((ID3D11DeviceContext**)(&commandContext)); 
		auto pass = pTechnique->GetPassByIndex(0);
		if (pass->IsValid()) {
		} else {
			logger::info("Pass: Effect is invalid!");
		}
		//pContext;
		pass->Apply(0, pContext);
		//nThreadGroups;
		pContext->Dispatch(nThreadGroups, 1, 1);
	}
	void DrawIndexedInstanced(EI_CommandContextRef commandContext,
		EI_PSO&                                      pso,
		AMD::EI_IndexedDrawParams&                   drawParams)
	{
		//TODO need to update variables
		//pEffect->BindIndexBuffer(pIndexBuffer);
		ID3D11DeviceContext* pContext = *((ID3D11DeviceContext**)(&commandContext));
		logger::info("Setting index buffer");

		pContext->IASetIndexBuffer((ID3D11Buffer*)drawParams.pIndexBuffer, DXGI_FORMAT_R32_UINT, 0);
		D3DX11_TECHNIQUE_DESC techDesc;
		pso.m_pTechnique->GetDesc(&techDesc); 
		uint32_t numPasses = techDesc.Passes;
		//bool   techniqueFound = pEffect->Begin(&technique, &numPasses);
		int indicesPerInstance = drawParams.numIndices / drawParams.numInstances;
		//if (techniqueFound) {
		for (uint32_t i = 0; i < numPasses; ++i) {
			auto pass = pso.m_pTechnique->GetPassByIndex(i);
			if (pass->IsValid()){
			} else {
				logger::info("Pass: Effect is invalid!");
			}
			pass->Apply(0, pContext);
			pContext->DrawIndexedInstanced(indicesPerInstance, drawParams.numInstances, 0, 0, 0);
		}
	}
	void Clear2D(EI_CommandContext* pContext, EI_RWTexture2D* pResource, AMD::uint32 clearValue)
	{
		pResource;
		//logger::info("Clear 2d");
		ID3D11DeviceContext* pDeviceContext = (ID3D11DeviceContext*)*((ID3D11DeviceContext**)pContext);
		uint32_t clearVector[4];
		clearVector[0] = clearVector[1] = clearVector[2] = clearVector[3] = clearValue;
		//ID3D11UnorderedAccessView* nullUAV = nullptr;
		pDeviceContext->OMSetRenderTargetsAndUnorderedAccessViews(D3D11_KEEP_RENDER_TARGETS_AND_DEPTH_STENCIL, NULL, NULL, 0, 0, NULL, NULL);
		//logger::info("cleared 2d");
		//pResource->uav->ClearUInt(clearVector);
		//pResource->resource->UAVBarrier();
	}
	void ClearCounter(EI_CommandContextRef pContext,
		EI_StructuredBufferRef               sb,
		AMD::uint32                          clearValue)
	{
		sb;
		clearValue;
		//logger::info("Clear counter");
		ID3D11DeviceContext* pDeviceContext = *((ID3D11DeviceContext**)(&pContext));
		pDeviceContext;
		//pDeviceContext->Count
		//sb.uav->SetInitialCount(clearValue);
	}
}
EI_PSO* GetPSO(const char* techniqueName, ID3DX11Effect* pEffect)
{
	if (pEffect == nullptr) {
		logger::warn("Could not get technique named {} because effect is null.", techniqueName);
		return nullptr;
	}
	ID3DX11EffectTechnique* pTechnique = pEffect->GetTechniqueByName(techniqueName);
	if (pTechnique == nullptr) {
		logger::warn("Could not get technique named {}", techniqueName);
		return nullptr;
	}
	EI_PSO* pso = new EI_PSO;
	pso->m_pEffect = pEffect;
	pso->m_pTechnique = pTechnique;
	return pso;	
}

FullscreenPass::FullscreenPass(SkyrimGPUResourceManager* pManager)
{
	DirectX::XMFLOAT2 quadVertBuffer[] = {
		{ -1.0, -1.0 },
		{ 1.0, -1.0 },
		{ -1.0, 1.0 },
		{ 1.0, 1.0 },
	};
	// Create quad vertex buffer.
	//SuMemoryBufferPtr quadVertBuffer(SuMemoryBuffer::Allocate(sizeof(SuVector2) * 4));
	//quadVertBuffer->Set(0, 0, SuVector2(-1.0, -1.0));
	//quadVertBuffer->Set(0, 1, SuVector2(1.0, -1.0));
	//quadVertBuffer->Set(0, 2, SuVector2(-1.0, 1.0));
	//quadVertBuffer->Set(0, 3, SuVector2(1.0, 1.0));
	D3D11_BUFFER_DESC bufferDesc;
	bufferDesc.Usage = D3D11_USAGE_DEFAULT;
	bufferDesc.ByteWidth = sizeof(DirectX::XMFLOAT2) * 4;
	bufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bufferDesc.CPUAccessFlags = 0;
	bufferDesc.MiscFlags = 0;

	D3D11_SUBRESOURCE_DATA InitData;
	InitData.pSysMem = quadVertBuffer;
	InitData.SysMemPitch = 0;
	InitData.SysMemSlicePitch = 0;

	pManager->m_pDevice->CreateBuffer(&bufferDesc, &InitData, &m_QuadVertexBuffer);
	/*m_QuadVertexBuffer =
		SuGPUResourceManager::GetRef().CreateResourceVertex(SuGPUResource::GPU_RESOURCE_STATIC,
			sizeof(SuVector2),
			4,
			quadVertBuffer,
			SuGPUResource::BIND_VERTEX_BUFFER);*/
}

FullscreenPass::~FullscreenPass() {}

void FullscreenPass::Draw(SkyrimGPUResourceManager* pManager, EI_PSO* pPSO)
{
	// Local hack
	//SuEffectTechnique& technique = GetTechniqueRef(pso);
	ID3DX11Effect* pEffect = pPSO->m_pEffect;
	ID3DX11EffectTechnique* pTechnique = pPSO->m_pTechnique;
	if (pEffect) {
		ID3D11DeviceContext* pContext;
		pManager->m_pDevice->GetImmediateContext(&pContext);
		logger::info("Setting vertex buffers");
		pContext->IASetVertexBuffers(0, 1, &m_QuadVertexBuffer , m_DataSize, m_DataOffsets);
		//pEffect->BindVertexBuffer("QuadStream", m_QuadVertexBuffer);
		D3DX11_TECHNIQUE_DESC techDesc;
		pTechnique->GetDesc(&techDesc);
		uint32_t numPasses = techDesc.Passes;
		for (uint32_t i = 0; i < numPasses; ++i) {
			auto pass = pTechnique->GetPassByIndex(i);
			if (pass->IsValid()) {
			} else {
				logger::info("Pass: Effect is invalid!");
			}
			pass->Apply(0, pContext);
			pContext->Draw(4, 0);
		}
		/*bool   techniqueFound = pEffect->Begin(pTechnique, &numPasses);
		if (techniqueFound) {
			for (uint32 i = 0; i < numPasses; ++i) {
				pEffect->BeginPass(i);
				SuRenderManager::GetRef().DrawNonIndexed(SuRenderManager::TRIANGLE_STRIP, 4, 0);
				pEffect->EndPass();
			}
			pEffect->End();
		}*/
	}
}
