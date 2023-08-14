#pragma once
#include "EngineInterface.h"
#include "TressFX/TressFXCommon.h"
#include <d3d11.h>
typedef const char*               EI_StringHash;
typedef ID3D11ShaderResourceView  EI_SRV;
typedef ID3D11RenderTargetView    EI_RTV;
typedef ID3D11DepthStencilView    EI_DSV;
typedef ID3D11UnorderedAccessView EI_UAV;

typedef DXGI_FORMAT EI_ResourceFormat;

const static int MaxRenderAttachments = 5;

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
	EI_ShaderStage                   stage;
};

class EI_Resource
{
public:
	// need this for automatic destruction
	EI_Resource();
	~EI_Resource();

	//int GetHeight() const { return m_ResourceType == EI_ResourceType::Texture ? m_pTexture->GetHeight() : 0; }
	//int GetWidth() const { return m_ResourceType == EI_ResourceType::Texture ? m_pTexture->GetWidth() : 0; }

	UINT m_totalMemSize;
	UINT m_textureWidth;
	UINT m_textureHeight;
	UINT m_indexBufferNumIndices;

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

	const EI_Resource*      m_RenderResources[MaxRenderAttachments] = { nullptr };
	ID3D11RenderTargetView* m_renderTargets[MaxRenderAttachments] = { nullptr };
	ID3D11DepthStencilView* m_depthView = nullptr;
	EI_ResourceFormat       m_RenderResourceFormats[MaxRenderAttachments];  // Needed for PS0 creation when we don't have resources yet (i.e gltf)
	D3D11_CLEAR_VALUE       m_ClearValues[MaxRenderAttachments];
	bool                    m_ClearColor[MaxRenderAttachments] = { false };
	uint32_t                m_NumResources = 0;
	bool                    m_HasDepth = false;
	bool                    m_ClearDepth = false;
};

class EI_PSO
{
public:
	EI_BindPoint               bp;
	ID3D11VertexShader*        VS = nullptr;
	ID3D11ClassInstance**      VSClassInstances;
	UINT                       numVSClassInstances = 0;
	ID3D11PixelShader*         PS = nullptr;
	ID3D11ClassInstance**      PSClassInstances;
	UINT                       numPSClassInstances = 0;
	ID3D11ComputeShader*       CS = nullptr;
	ID3D11ClassInstance**      CSClassInstances;
	UINT                       numCSClassInstances = 0;
	ID3D11BlendState*          blendState = nullptr;
	FLOAT                      blendFactor[4] = { 0 };
	UINT                       sampleMask = 0;
	ID3D11RasterizerState*     rasterizerState = nullptr;
	ID3D11DepthStencilState*   depthStencilState = nullptr;
	UINT                       stencilRef = 0;
	ID3D11InputLayout*         inputLayout = nullptr;
	D3D11_PRIMITIVE_TOPOLOGY   primitiveTopology;
	ID3D11Buffer*              indexBuffer = nullptr;
	DXGI_FORMAT                indexBufferFormat;
	UINT                       indexBufferOffset = 0;
	UINT                       numVertexBuffers = D3D11_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT;
	ID3D11Buffer*              vertexBuffers[D3D11_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT];
	UINT                       vertexBufferStrides[D3D11_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT];
	UINT                       vertexBufferOffsets[D3D11_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT];
	UINT                       numRTVs = D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT;
	ID3D11DepthStencilView*    DSV;
};

class EI_CommandContext
{
public:
	EI_CommandContext() {}
	~EI_CommandContext() {}
	void UpdateBuffer(EI_Resource* res, void* data);
	void BindSets(EI_PSO* pso, int numBindSets, EI_BindSet** bindSets);
	void DrawIndexedInstanced(EI_PSO& pso, EI_IndexedDrawParams& drawParams);
	void DrawInstanced(EI_PSO& pso, EI_DrawParams& drawParams);
	void SubmitBarrier(int numBarriers, EI_Barrier* barriers);
	void ClearUint32Image(EI_Resource* res, uint32_t value);
	void BindPSO(EI_PSO* pso);
	void Dispatch(int numGroups);
};

EI_Device* GetDevice();

class EI_Device
{
public:
	EI_Device();
	~EI_Device();

	EI_CommandContext&                  GetCurrentCommandContext() { return m_currentCommandContext; }
	EI_Resource*                        GetColorBufferResource() { return m_colorBuffer.get(); }
	EI_Resource*                        GetDepthBufferResource() { return m_depthBuffer.get(); }
	EI_ResourceFormat                   GetDepthBufferFormat() { return DXGI_FORMAT_R24G8_TYPELESS; }
	EI_ResourceFormat                   GetColorBufferFormat() { return DXGI_FORMAT_R8G8B8A8_UNORM; }
	EI_BindSet*                         GetSamplerBindSet() { return m_pSamplerBindSet.get(); }
	void                                OnCreate();
	std::unique_ptr<EI_Resource>        CreateBufferResource(int structSize, const int structCount, const unsigned int flags, EI_StringHash name);
	std::unique_ptr<EI_BindLayout>      CreateLayout(const EI_LayoutDescription& description);
	std::unique_ptr<EI_BindSet>         CreateBindSet(EI_BindLayout* layout, EI_BindSetDescription& bindSet);
	std::unique_ptr<EI_Resource>        CreateResourceFromFile(const char* szFilename, bool useSRGB /*= false*/);
	std::unique_ptr<EI_Resource>        CreateUint32Resource(const int width, const int height, const int arraySize, const char* name, uint32_t ClearValue /*= 0*/);
	std::unique_ptr<EI_RenderTargetSet> CreateRenderTargetSet(const EI_ResourceFormat* pResourceFormats, const uint32_t numResources, const EI_AttachmentParams* AttachmentParams, float* clearValues);
	std::unique_ptr<EI_RenderTargetSet> CreateRenderTargetSet(const EI_Resource** pResourcesArray, const uint32_t numResources, const EI_AttachmentParams* AttachmentParams, float* clearValues);
	std::unique_ptr<EI_Resource>        CreateRenderTargetResource(const int width, const int height, const size_t channels, const size_t channelSize, const char* name, AMD::float4* ClearValues /*= nullptr*/);
	std::unique_ptr<EI_PSO>             CreateComputeShaderPSO(const char* shaderName, const char* entryPoint, EI_BindLayout** layouts, int numLayouts);
	std::unique_ptr<EI_PSO>             CreateGraphicsPSO(const char* vertexShaderName, const char* vertexEntryPoint, const char* fragmentShaderName, const char* fragmentEntryPoint, EI_PSOParams& psoParams);
	void                                BeginRenderPass(EI_CommandContext& commandContext, const EI_RenderTargetSet* pRenderTargetSet, const wchar_t* pPassName, uint32_t width = 0, uint32_t height = 0);
	void                                EndRenderPass(EI_CommandContext& commandContext);
	
	//why is this in EI_Device?
	void DrawFullScreenQuad(EI_CommandContext& commandContext, EI_PSO& pso, EI_BindSet** bindSets, uint32_t numBindSets);
	void GetTimeStamp(const char* name);

private:
	std::unique_ptr<EI_Resource> CreateIndexBufferResource(int structSize, const int structCount, EI_StringHash name);
	std::unique_ptr<EI_Resource> CreateConstantBufferResource(int structSize, const int structCount, EI_StringHash name);
	std::unique_ptr<EI_Resource> CreateStructuredBufferResource(int structSize, const int structCount, EI_StringHash name);

	std::unique_ptr<EI_Resource>  m_depthBuffer;
	std::unique_ptr<EI_Resource> m_colorBuffer;
	std::unique_ptr<EI_Resource> m_shadowBuffer;
	std::unique_ptr<EI_BindSet>   m_pSamplerBindSet = nullptr;

	std::unique_ptr<EI_Resource> m_pFullscreenIndexBuffer;
	EI_CommandContext            m_currentCommandContext;
};

//??
class EI_Marker
{
public:
	EI_Marker(EI_CommandContext& ctx, const char* string)
	{
		UNREFERENCED_PARAMETER(ctx);
		UNREFERENCED_PARAMETER(string);
	}

private:
};
