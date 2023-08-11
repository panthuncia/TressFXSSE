#pragma once
#include "EngineInterface.h"
#include "TressFXCommon.h"
#include <d3d11.h>
typedef const char*               EI_StringHash;
typedef ID3D11ShaderResourceView  EI_SRV;
typedef ID3D11RenderTargetView    EI_RTV;
typedef ID3D11DepthStencilView    EI_DSV;
typedef ID3D11UnorderedAccessView EI_UAV;

typedef DXGI_FORMAT EI_ResourceFormat;

const static int MaxRenderAttachments = 5;

class EI_Resource
{
public:
	ID3D11Buffer*              buffer = nullptr;
	ID3D11Texture2D*           texture = nullptr;
	ID3D11UnorderedAccessView* uav = nullptr;
	ID3D11ShaderResourceView*  srv = nullptr;
	ID3D11ShaderResourceView*  rtv = nullptr;  //renderable texture view- same as SRV in DX11, sushi has a separate type
	D3D11_BUFFER_DESC          desc;
	bool                       hasUAV;
	uint32_t                   structCount;
	uint32_t                   structSize;
	void*                      data;
};
struct EI_BindLayout
{
	//~EI_BindLayout();
	EI_LayoutDescription description;
};
class EI_BindSet
{
public:
	std::vector<EI_SRV*>             srvs;
	std::vector<EI_UAV*>             uavs;
	std::vector<ID3D11Buffer*>       cbuffers;
	std::vector<ID3D11SamplerState*> samplers;
};

class EI_Resource
{
public:
	// need this for automatic destruction
	EI_Resource();
	~EI_Resource();

	//int GetHeight() const { return m_ResourceType == EI_ResourceType::Texture ? m_pTexture->GetHeight() : 0; }
	//int GetWidth() const { return m_ResourceType == EI_ResourceType::Texture ? m_pTexture->GetWidth() : 0; }

	EI_ResourceType m_ResourceType = EI_ResourceType::Undefined;

	union
	{
		ID3D11Buffer*       m_pBuffer;
		ID3D11Texture2D*    m_pTexture;
		ID3D11SamplerState* m_pSampler;
	};

	// Needed since we will allocate samplers directly in the table range
	D3D11_SAMPLER_DESC samplerDesc = {};

	EI_RTV* RTView = nullptr;
	EI_DSV* DSView = nullptr;
	EI_SRV* SRView = nullptr;
	EI_UAV* UAView = nullptr;

private:
};
typedef struct D3D11_DEPTH_STENCIL_VALUE
{
	FLOAT Depth;
	UINT8 Stencil;
} D3D11_DEPTH_STENCIL_VALUE;
typedef struct D3D11_CLEAR_VALUE
{
	DXGI_FORMAT Format;
	union
	{
		FLOAT                     Color[4];
		D3D11_DEPTH_STENCIL_VALUE DepthStencil;
	};
} D3D11_CLEAR_VALUE;
struct EI_RenderTargetSet
{
	~EI_RenderTargetSet();
	void SetResources(const EI_Resource** pResourcesArray);

	const EI_Resource* m_RenderResources[MaxRenderAttachments] = { nullptr };
	EI_ResourceFormat  m_RenderResourceFormats[MaxRenderAttachments];  // Needed for PS0 creation when we don't have resources yet (i.e gltf)
	D3D11_CLEAR_VALUE  m_ClearValues[MaxRenderAttachments];
	bool               m_ClearColor[MaxRenderAttachments] = { false };
	uint32_t           m_NumResources = 0;
	bool               m_HasDepth = false;
	bool               m_ClearDepth = false;
};

class EI_Device
{
public:
	EI_Device();
	~EI_Device();

	std::unique_ptr<EI_Resource>        CreateBufferResource(int structSize, const int structCount, const unsigned int flags, EI_StringHash name);
	std::unique_ptr<EI_BindLayout>      CreateLayout(const EI_LayoutDescription& description);
	std::unique_ptr<EI_BindSet>         CreateBindSet(EI_BindLayout* layout, EI_BindSetDescription& bindSet);
	std::unique_ptr<EI_Resource>        CreateResourceFromFile(const char* szFilename, bool useSRGB /*= false*/);
	std::unique_ptr<EI_Resource>        CreateUint32Resource(const int width, const int height, const int arraySize, const char* name, uint32_t ClearValue /*= 0*/);
	std::unique_ptr<EI_RenderTargetSet> CreateRenderTargetSet(const EI_ResourceFormat* pResourceFormats, const uint32_t numResources, const EI_AttachmentParams* AttachmentParams, float* clearValues);
	std::unique_ptr<EI_Resource>        CreateRenderTargetResource(const int width, const int height, const size_t channels, const size_t channelSize, const char* name, AMD::float4* ClearValues /*= nullptr*/);

private:
	std::unique_ptr<EI_Resource> CreateIndexBufferResource(int structSize, const int structCount, EI_StringHash name);
	std::unique_ptr<EI_Resource> CreateConstantBufferResource(int structSize, const int structCount, EI_StringHash name);
	std::unique_ptr<EI_Resource> CreateStructuredBufferResource(int structSize, const int structCount, EI_StringHash name);
};
