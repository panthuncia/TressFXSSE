#include "SkyrimTressFX.h"
#include "TressFX/TressFXPPLL.h"
#include "TressFX/TressFXShortCut.h"

SkyrimTressFX::SkyrimTressFX() {

}

SkyrimTressFX::~SkyrimTressFX() {

}

void SkyrimTressFX::Draw() {
	// Do hair draw - will pick correct render approach
	if (m_drawHair) {
		DrawHair();
	}
	// Render debug collision mesh if needed
	EI_CommandContext& commandList = GetDevice()->GetCurrentCommandContext();

	if (m_drawCollisionMesh || m_drawMarchingCubes) {
		if (m_drawMarchingCubes) {
			GenerateMarchingCubes();
		}

		GetDevice()->BeginRenderPass(commandList, m_pDebugRenderTargetSet.get(), L"DrawCollisionMesh Pass");
		if (m_drawCollisionMesh) {
			DrawCollisionMesh();
		}
		if (m_drawMarchingCubes) {
			DrawSDF();
		}
		GetDevice()->EndRenderPass(GetDevice()->GetCurrentCommandContext());
	}
}

void SkyrimTressFX::DrawHair() {
	int numHairStrands = (int)m_activeScene.objects.size();
	std::vector<HairStrands*> hairStrands(numHairStrands);
	for (int i = 0; i < numHairStrands; ++i) {
		hairStrands[i] = m_activeScene.objects[i].hairStrands.get();
	}

	EI_CommandContext& pRenderCommandList = GetDevice()->GetCurrentCommandContext();

	switch (m_eOITMethod) {
	case OIT_METHOD_PPLL:
		m_pPPLL->Draw(pRenderCommandList, numHairStrands, hairStrands.data(), m_activeScene.viewBindSet.get(), m_activeScene.lightBindSet.get());
		break;
	case OIT_METHOD_SHORTCUT:
		m_pShortCut->Draw(pRenderCommandList, numHairStrands, hairStrands.data(), m_activeScene.viewBindSet.get(), m_activeScene.lightBindSet.get());
		break;
	};

	for (size_t i = 0; i < hairStrands.size(); i++)
		hairStrands[i]->TransitionRenderingToSim(pRenderCommandList);
}


void SkyrimTressFX::InitializeLayouts()
{
	InitializeAllLayouts(GetDevice());
}

void SkyrimTressFX::DestroyLayouts()
{
	DestroyAllLayouts(GetDevice());
}

void SkyrimTressFX::SetOITMethod(OITMethod method)
{
	if (method == m_eOITMethod)
		return;

	// Flush the GPU before switching
	//GetDevice()->FlushGPU();

	// Destroy old resources
	DestroyOITResources(m_eOITMethod);

	m_eOITMethod = method;
	RecreateSizeDependentResources();
}

void SkyrimTressFX::DestroyOITResources(OITMethod method)
{
	// TressFX Usage
	switch (m_eOITMethod) {
	case OIT_METHOD_PPLL:
		m_pPPLL.reset();
		break;
	case OIT_METHOD_SHORTCUT:
		m_pShortCut.reset();
		break;
	};
}

void SkyrimTressFX::RecreateSizeDependentResources()
{
	//GetDevice()->FlushGPU();
	//GetDevice()->OnResize(m_nScreenWidth, m_nScreenHeight);

	// Whenever there is a resize, we need to re-create on render pass set as it depends on
	// the main color/depth buffers which get resized
	//const EI_Resource* ResourceArray[] = { GetDevice()->GetColorBufferResource(), GetDevice()->GetDepthBufferResource() };
	//m_pGLTFRenderTargetSet->SetResources(ResourceArray);
	//m_pDebugRenderTargetSet->SetResources(ResourceArray);

	//m_activeScene.scene->OnResize(m_nScreenWidth, m_nScreenHeight);

	//// Needs to be created in OnResize in case we have debug buffers bound which vary by screen width/height
	//EI_BindSetDescription lightSet = {
	//	{ m_activeScene.lightConstantBuffer.GetBufferResource(), GetDevice()->GetShadowBufferResource() }
	//};
	//m_activeScene.lightBindSet = GetDevice()->CreateBindSet(GetLightLayout(), lightSet);

	//// TressFX Usage
	//switch (m_eOITMethod) {
	//case OIT_METHOD_PPLL:
	//	m_nPPLLNodes = m_nScreenWidth * m_nScreenHeight * AVE_FRAGS_PER_PIXEL;
	//	m_pPPLL.reset(new TressFXPPLL);
	//	m_pPPLL->Initialize(m_nScreenWidth, m_nScreenHeight, m_nPPLLNodes, PPLL_NODE_SIZE);
	//	break;
	//case OIT_METHOD_SHORTCUT:
	//	m_pShortCut.reset(new TressFXShortCut);
	//	m_pShortCut->Initialize(m_nScreenWidth, m_nScreenHeight);
	//	break;
	//};
}
