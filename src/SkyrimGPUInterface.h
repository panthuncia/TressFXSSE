#pragma once
#include "AMD_TressFX.h"
#include "TressFXGPUInterface.h"
#include "SkyrimGPUResourceManager.h"
#include "SuAssert.h"
#include "Effects11/d3dx11effect.h"
#include <d3d11.h>
template <typename T>
constexpr auto type_name()
{
	std::string_view name, prefix, suffix;
#ifdef __clang__
	name = __PRETTY_FUNCTION__;
	prefix = "auto type_name() [T = ";
	suffix = "]";
#elif defined(__GNUC__)
	name = __PRETTY_FUNCTION__;
	prefix = "constexpr auto type_name() [with T = ";
	suffix = "]";
#elif defined(_MSC_VER)
	name = __FUNCSIG__;
	prefix = "auto __cdecl type_name<";
	suffix = ">(void)";
#endif
	name.remove_prefix(prefix.size());
	name.remove_suffix(suffix.size());
	return name;
}
EXTERN_C
{
	EI_BindLayout* CreateLayout(EI_Device* pDevice, EI_LayoutManagerRef layoutManager, const AMD::TressFXLayoutDescription& description);
	void DestroyLayout(EI_Device* pDevice, EI_BindLayout* pLayout);

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
	void Bind(EI_CommandContextRef commandContext, EI_BindLayout * pLayout, EI_BindSet & set);
	void DrawIndexedInstanced(EI_CommandContextRef commandContext, EI_PSO & pso, AMD::EI_IndexedDrawParams & drawParams);
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
	std::vector<ID3DX11EffectShaderResourceVariable*> srvs;
	std::vector<ID3DX11EffectUnorderedAccessViewVariable*> uavs;
	std::vector<ID3DX11EffectVariable*> constants;
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
class EI_PSO
{
public:
	ID3DX11Effect*          m_pEffect;
	ID3DX11EffectTechnique* m_pTechnique;
};
EI_PSO* GetPSO(const char* techniqueName, ID3DX11Effect* pEffect);
