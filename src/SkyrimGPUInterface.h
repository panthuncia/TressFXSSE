#pragma once
#include "AMD_TressFX.h"
#include "TressFXGPUInterface.h"
#include "SkyrimGPUResourceManager.h"
#include "SuAssert.h"
#include <d3d11.h>
EXTERN_C
{
	EI_BindLayout* SuCreateLayout(EI_Device* pDevice, EI_LayoutManagerRef layoutManager, const AMD::TressFXLayoutDescription& description);
	void SuDestroyLayout(EI_Device* pDevice, EI_BindLayout* pLayout);

	// Creates a structured buffer and srv (StructuredBuffer). It necessarily needs data to start, so
	// begin state should be EI_STATE_COPY_DEST.
	EI_StructuredBuffer* CreateReadOnlySB(EI_Device* pDevice,
		const AMD::uint32 structSize,
		const AMD::uint32 structCount,
		EI_StringHash  resourceName,
		EI_StringHash  objectName);

	// Creates a structured buffer and default UAV/SRV (StructuredBuffer and RWStructuredBuffer in
	// HLSL).  It will be used as UAV, so begin state should be EI_STATE_UAV.
	EI_StructuredBuffer* CreateReadWriteSB(EI_Device* pDevice,
		const AMD::uint32 structSize,
		const AMD::uint32 structCount,
		EI_StringHash  resourceName,
		EI_StringHash  objectName);

	// Same as EI_CreateReadWriteSB, but also includes an atomic counter.  Atomic counter is cleared to
	// zero each frame (EI_SB_ClearCounter)
	EI_StructuredBuffer* CreateCountedSB(EI_Device* pDevice,
		const AMD::uint32 structSize,
		const AMD::uint32 structCount,
		EI_StringHash  resourceName,
		EI_StringHash  objectName);

	//void SuDestroy(EI_Device* pDevice, EI_Resource* pResource);
	EI_BindSet* CreateBindSet(EI_Device* commandContext, AMD::TressFXBindSet& bindSet);
	void DestroyBindSet(EI_Device* pDevice, EI_BindSet* pBindSet);
	void SubmitBarriers(EI_CommandContextRef context, int numBarriers, AMD::EI_Barrier* barriers);
	void* Map(EI_CommandContextRef pContext, EI_StructuredBuffer& sb);
	bool Unmap(EI_CommandContextRef pContext, EI_StructuredBuffer& sb);
	EI_IndexBuffer* CreateIndexBuffer(EI_Device* context,
		AMD::uint32      indexCount,
		void* pInitialData, EI_StringHash objectName);
	void DestroyIB(EI_Device* pDevice, EI_IndexBuffer* pBuffer);
	void LogError(const char* msg);
}
class EI_Resource
{
public:
	ID3D11Buffer* resource;
	ID3D11UnorderedAccessView*    uav;
	ID3D11ShaderResourceView*   srv;
	/*SuGPURenderableResourceViewPtr rtv;*/
	D3D11_BUFFER_DESC desc;
	bool hasUAV;
	uint32_t structCount;
	uint32_t structSize;
	void* data;
};
struct EI_BindLayout
{
	//std::vector<const SuTextureSlot*> srvs;
	//std::vector<const SuUAVSlot*> uavs;
	//std::vector<SuEffectParameter*> constants;
};
class EI_BindSet
{
public:
	int     nSRVs;
	ID3D11ShaderResourceView** srvs;
	int     nUAVs;
	ID3D11UnorderedAccessView** uavs;
	//SuArray<SuGPUSamplingResourceViewPtr> srvs;
	//SuArray<SuGPUUnorderedAccessViewPtr> uavs;

	void* values;
	int     nBytes;
};
