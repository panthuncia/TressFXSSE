#include "MarkerRender.h"
#include <d3dcompiler.h>
#include <vector>
#include "Util.h"
//why does windows.h brick std::min and max
#undef max
#undef min
#include <WaveFrontReader.h>
struct float3
{
	union
	{
		struct
		{
			float x, y, z;
		};
		float v[3];
	};
};

MarkerRender::~MarkerRender() {

}
void MarkerRender::InitRenderResources()
{
	logger::info("Initializing marker debug renderer");
	ID3D11Device* pDevice = RE::BSGraphics::Renderer::GetSingleton()->GetRuntimeData().forwarder;

	CompileShaders(pDevice);
	logger::info("Compiled marker shaders");
	CreateBuffers(pDevice);
	logger::info("Created marker buffers");
	CreateLayoutsAndStates(pDevice);
	logger::info("Created marker layouts");
	LoadMeshes(pDevice);
}
void MarkerRender::ClearMarkers() {
	m_markerPositions.clear();
}
void MarkerRender::DrawAllMarkers(DirectX::XMMATRIX viewXMMatrix, DirectX::XMMATRIX projXMMatrix)
{
	//logger::info("Drawing {} markers", m_markerPositions.size());
	MarkerRender::GetSingleton()->DrawMarkers(m_markerPositions, viewXMMatrix, projXMMatrix);
	m_markerPositions.clear();
}
void MarkerRender::DrawAllMarkers(AMD::float4x4 viewMatrix, AMD::float4x4 projMatrix)
{
	DirectX::XMFLOAT4X4 vm = DirectX::XMFLOAT4X4(&viewMatrix.m[0]);
	DirectX::XMFLOAT4X4 pm = DirectX::XMFLOAT4X4(&projMatrix.m[0]);
	DirectX::XMMATRIX   vmx = DirectX::XMLoadFloat4x4(&vm);
	DirectX::XMMATRIX   pmx = DirectX::XMLoadFloat4x4(&pm);
	DrawAllMarkers(vmx, pmx);
}
void MarkerRender::DrawWorldAxes(AMD::float4x4 cameraWorldTransform, AMD::float4x4 viewMatrix, AMD::float4x4 projMatrix)
{
	DirectX::XMFLOAT4X4 wm = DirectX::XMFLOAT4X4(&cameraWorldTransform.m[0]);
	DirectX::XMFLOAT4X4 vm = DirectX::XMFLOAT4X4(&viewMatrix.m[0]);
	DirectX::XMFLOAT4X4 pm = DirectX::XMFLOAT4X4(&projMatrix.m[0]);
	DirectX::XMMATRIX   wmx = DirectX::XMLoadFloat4x4(&wm);
	DirectX::XMMATRIX   vmx = DirectX::XMLoadFloat4x4(&vm);
	DirectX::XMMATRIX   pmx = DirectX::XMLoadFloat4x4(&pm);
	DrawWorldAxes(wmx, vmx, pmx);
}
void MarkerRender::DrawWorldAxes(DirectX::XMMATRIX cameraWorldTransform, DirectX::XMMATRIX viewMatrix, DirectX::XMMATRIX projectionMatrix)
{
	ID3D11DeviceContext* pDeviceContext = RE::BSGraphics::Renderer::GetSingleton()->GetRuntimeData().context;
	////draw world arrows
	for (int i = 0; i < m_pArrowMesh.get()->meshParts.size(); i++) {
		auto part = m_pArrowMesh.get()->meshParts.at(i).get();

		//vertex buffers
		auto       partVertexBuffer = m_pArrowVertexBuffer;
		auto       partVertexStride = part->vertexStride;
		const UINT partVertexOffset = part->vertexOffset;
		pDeviceContext->IASetVertexBuffers(0, 1, &partVertexBuffer, &partVertexStride, &partVertexOffset);

		//index buffers
		auto       partIndexBuffer = m_pArrowIndexBuffer;
		auto       partIndexFormat = part->indexFormat;
		const UINT partIndexOffset = part->startIndex;
		pDeviceContext->IASetIndexBuffer(partIndexBuffer, partIndexFormat, partIndexOffset);

		//primitive type
		auto partPrimitiveTechnology = part->primitiveType;
		pDeviceContext->IASetPrimitiveTopology(partPrimitiveTechnology);

		//input layout
		auto partInputLayout = part->inputLayout.Get();
		pDeviceContext->IASetInputLayout(partInputLayout);

		pDeviceContext->VSSetShader(m_pVertexShader, nullptr, 0);
		pDeviceContext->VSSetConstantBuffers(0, 1, &m_pConstantBuffer);

		// Set the pixel shader
		pDeviceContext->PSSetShader(m_pPixelShader, nullptr, 0);
		pDeviceContext->PSSetConstantBuffers(0, 1, &m_pConstantBuffer);

		pDeviceContext->OMSetDepthStencilState(m_pDepthStencilState, 0);
		pDeviceContext->RSSetState(m_pWireframeRSState);

		//set position
		CBMatrix          cbMatrix;
		DirectX::XMMATRIX scale = DirectX::XMMatrixScaling(2, 2, 2);

		DirectX::XMFLOAT4X4 camWorldFloats;
		DirectX::XMStoreFloat4x4(&camWorldFloats, cameraWorldTransform);
		DirectX::XMFLOAT3 cameraPosition;
		cameraPosition.x = camWorldFloats._14;
		cameraPosition.y = camWorldFloats._24;
		cameraPosition.z = camWorldFloats._34;

		//forward
		DirectX::XMFLOAT3 cameraForward;
		cameraForward.x = camWorldFloats._11;
		cameraForward.y = camWorldFloats._21;
		cameraForward.z = camWorldFloats._31;
		DirectX::XMVECTOR forwardVector = DirectX::XMVector3Normalize(DirectX::XMLoadFloat3(&cameraForward));
		XMStoreFloat3(&cameraForward, forwardVector);
		const float       distanceAhead = 100.0f;
		DirectX::XMFLOAT3 targetPosition;
		targetPosition.x = cameraPosition.x + (cameraForward.x * distanceAhead);
		targetPosition.y = cameraPosition.y + (cameraForward.y * distanceAhead);
		targetPosition.z = cameraPosition.z + (cameraForward.z * distanceAhead);

		// left
		DirectX::XMFLOAT3 cameraRight;
		cameraRight.x = camWorldFloats._13;
		cameraRight.y = camWorldFloats._23;
		cameraRight.z = camWorldFloats._33;
		DirectX::XMVECTOR rightVector = DirectX::XMVector3Normalize(DirectX::XMLoadFloat3(&cameraRight));
		XMStoreFloat3(&cameraRight, rightVector);
		const float distanceRight = -60.0f;
		targetPosition.x = targetPosition.x + (cameraRight.x * distanceRight);
		targetPosition.y = targetPosition.y + (cameraRight.y * distanceRight);
		targetPosition.z = targetPosition.z + (cameraRight.z * distanceRight);

		// Down
		DirectX::XMFLOAT3 cameraUp;
		cameraUp.x = camWorldFloats._12;
		cameraUp.y = camWorldFloats._22;
		cameraUp.z = camWorldFloats._32;
		DirectX::XMVECTOR upVector = DirectX::XMVector3Normalize(DirectX::XMLoadFloat3(&cameraUp));
		XMStoreFloat3(&cameraUp, upVector);
		const float distanceUp = -30.0f;
		targetPosition.x = targetPosition.x + (cameraUp.x * distanceUp);
		targetPosition.y = targetPosition.y + (cameraUp.y * distanceUp);
		targetPosition.z = targetPosition.z + (cameraUp.z * distanceUp);

		auto arrowWorldMatrix = XMMatrixTranspose(DirectX::XMMatrixTranslationFromVector(DirectX::XMLoadFloat3(&targetPosition)));
		//logger::info("Arrow world:");
		//PrintXMMatrix(arrowWorldMatrix);
		//logger::info("view:");
		//PrintXMMatrix(viewMatrix);
		//cbMatrix.worldViewProjectionMatrix = projectionMatrix * viewMatrix * arrowWorldMatrix * scale;
		cbMatrix.world = arrowWorldMatrix * scale;
		cbMatrix.view = viewMatrix;
		cbMatrix.projection = projectionMatrix;
		cbMatrix.color = DirectX::XMVectorSet(0.0, 1.0, 0.0, 1.0);
		pDeviceContext->UpdateSubresource(m_pConstantBuffer, 0, nullptr, &cbMatrix, 0, 0);

		//draw
		//logger::info("Drawing arrow part");
		pDeviceContext->DrawIndexed(part->indexCount, 0, 0);

		//rotate other axes
		constexpr float angle = DirectX::XMConvertToRadians(90.0f);
		auto            currentMatrix = arrowWorldMatrix * XMMatrixTranspose(DirectX::XMMatrixRotationZ(-angle));
		//cbMatrix.worldViewProjectionMatrix = projectionMatrix * viewMatrix * currentMatrix * scale;
		cbMatrix.world = currentMatrix * scale;
		cbMatrix.view = viewMatrix;
		cbMatrix.projection = projectionMatrix;
		cbMatrix.color = DirectX::XMVectorSet(1.0, 0.0, 0.0, 1.0);
		pDeviceContext->UpdateSubresource(m_pConstantBuffer, 0, nullptr, &cbMatrix, 0, 0);
		pDeviceContext->DrawIndexed(part->indexCount, 0, 0);

		currentMatrix = arrowWorldMatrix * XMMatrixTranspose(DirectX::XMMatrixRotationX(angle));
		//cbMatrix.worldViewProjectionMatrix = projectionMatrix * viewMatrix * currentMatrix * scale;
		cbMatrix.world = currentMatrix * scale;
		cbMatrix.view = viewMatrix;
		cbMatrix.projection = projectionMatrix;
		cbMatrix.color = DirectX::XMVectorSet(0.0, 0.0, 1.0, 1.0);
		pDeviceContext->UpdateSubresource(m_pConstantBuffer, 0, nullptr, &cbMatrix, 0, 0);
		pDeviceContext->DrawIndexed(part->indexCount, 0, 0);
	}
	logger::info("done drawing axes");
}
void MarkerRender::DrawMarkers(std::vector<DirectX::XMMATRIX> worldTransforms, DirectX::XMMATRIX viewMatrix, DirectX::XMMATRIX projectionMatrix)
{

	// Set the vertex buffer and index buffer
	ID3D11DeviceContext* pDeviceContext = RE::BSGraphics::Renderer::GetSingleton()->GetRuntimeData().context;

	UINT stride = sizeof(Vertex);
	UINT offset = 0;
	pDeviceContext->IASetVertexBuffers(0, 1, &m_pVertexBuffer, &stride, &offset);
	pDeviceContext->IASetIndexBuffer(m_pIndexBuffer, DXGI_FORMAT_R32_UINT, 0);
	pDeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	pDeviceContext->IASetInputLayout(m_pInputLayout);
	// Set the vertex shader and input layout
	pDeviceContext->VSSetShader(m_pVertexShader, nullptr, 0);
	pDeviceContext->VSSetConstantBuffers(0, 1, &m_pConstantBuffer);

	// Set the pixel shader
	pDeviceContext->PSSetShader(m_pPixelShader, nullptr, 0);
	pDeviceContext->PSSetConstantBuffers(0, 1, &m_pConstantBuffer);

	//set rasterizer state
	pDeviceContext->RSSetState(m_pWireframeRSState);

	//disable depth testing
	pDeviceContext->OMSetDepthStencilState(m_pDepthStencilState, 0);

	//draw markers
	DirectX::XMMATRIX cubeScale = DirectX::XMMatrixScaling(Util::RenderScale, Util::RenderScale, Util::RenderScale);
	for (const DirectX::XMMATRIX& worldTransform : worldTransforms) {
		// Set the constant buffer data
		CBMatrix cbMatrix;
		//logger::info("cube world:");
		//PrintXMMatrix(worldTransform);
		//cbMatrix.worldViewProjectionMatrix = projectionMatrix * viewMatrix * worldTransform;
		cbMatrix.world = worldTransform*cubeScale;
		cbMatrix.view = viewMatrix;
		cbMatrix.projection = projectionMatrix;
		cbMatrix.color = DirectX::XMVectorSet(1.0, 1.0, 1.0, 1.0);
		pDeviceContext->UpdateSubresource(m_pConstantBuffer, 0, nullptr, &cbMatrix, 0, 0);
		// Render the cube
		//logger::info("Drawing cube");
		pDeviceContext->DrawIndexed(36, 0, 0);
	}

	auto part = m_pArrowMesh.get()->meshParts.at(0).get();

	//vertex buffers
	auto       partVertexBuffer = m_pArrowVertexBuffer;
	auto       partVertexStride = part->vertexStride;
	const UINT partVertexOffset = part->vertexOffset;
	pDeviceContext->IASetVertexBuffers(0, 1, &partVertexBuffer, &partVertexStride, &partVertexOffset);

	//index buffers
	auto       partIndexBuffer = m_pArrowIndexBuffer;
	auto       partIndexFormat = part->indexFormat;
	const UINT partIndexOffset = part->startIndex;
	pDeviceContext->IASetIndexBuffer(partIndexBuffer, partIndexFormat, partIndexOffset);

	//primitive type
	auto partPrimitiveTechnology = part->primitiveType;
	pDeviceContext->IASetPrimitiveTopology(partPrimitiveTechnology);

	//input layout
	auto partInputLayout = part->inputLayout.Get();
	pDeviceContext->IASetInputLayout(partInputLayout);
	//draw cube arrows
	DirectX::XMMATRIX scale = DirectX::XMMatrixScaling(2 * Util::RenderScale, 2 * Util::RenderScale, 2 * Util::RenderScale);
	for (const DirectX::XMMATRIX& worldTransform : worldTransforms) {
		// Set the constant buffer data
		CBMatrix cbMatrix;
		
		//cbMatrix.worldViewProjectionMatrix = projectionMatrix * viewMatrix * worldTransform * scale;
		cbMatrix.world = worldTransform * scale;
		cbMatrix.view = viewMatrix;
		cbMatrix.projection = projectionMatrix;
		cbMatrix.color = DirectX::XMVectorSet(0.0, 1.0, 0.0, 1.0);
		pDeviceContext->UpdateSubresource(m_pConstantBuffer, 0, nullptr, &cbMatrix, 0, 0);

		pDeviceContext->DrawIndexed(part->indexCount, 0, 0);

		//rotate other axes
		constexpr float angle = DirectX::XMConvertToRadians(90.0f);
		auto            currentMatrix = worldTransform * XMMatrixTranspose(DirectX::XMMatrixRotationZ(-angle));
		//cbMatrix.worldViewProjectionMatrix = projectionMatrix * viewMatrix * currentMatrix * scale;
		cbMatrix.world = currentMatrix * scale;
		cbMatrix.view = viewMatrix;
		cbMatrix.projection = projectionMatrix;
		cbMatrix.color = DirectX::XMVectorSet(1.0, 0.0, 0.0, 1.0);
		pDeviceContext->UpdateSubresource(m_pConstantBuffer, 0, nullptr, &cbMatrix, 0, 0);
		pDeviceContext->DrawIndexed(part->indexCount, 0, 0);

		currentMatrix = worldTransform * XMMatrixTranspose(DirectX::XMMatrixRotationX(angle));
		//cbMatrix.worldViewProjectionMatrix = projectionMatrix * viewMatrix * currentMatrix * scale;
		cbMatrix.world = currentMatrix * scale;
		cbMatrix.view = viewMatrix;
		cbMatrix.projection = projectionMatrix;
		cbMatrix.color = DirectX::XMVectorSet(0.0, 0.0, 1.0, 1.0);
		pDeviceContext->UpdateSubresource(m_pConstantBuffer, 0, nullptr, &cbMatrix, 0, 0);
		pDeviceContext->DrawIndexed(part->indexCount, 0, 0);
	}
	//re-set DSV
	//pDeviceContext->OMSetRenderTargets(D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT, renderTargets, currentDSV);
}

void MarkerRender::LoadMeshes(ID3D11Device* pDevice)
{
	const auto modelPath = std::filesystem::current_path() /= "data\\Meshes\\MarkerRender\\arrow.vbo"sv;
	m_pArrowModel = DirectX::Model::CreateFromVBO(pDevice, modelPath.c_str());
	m_pArrowMesh = m_pArrowModel->meshes[0];

	 WaveFrontReader<uint16_t> wfReader;

	//cursed for now, need to load it twice since DirectXTK's buffers are broken.
	//TODO:: remove this garbage and make a real mesh loading system

	logger::info("loading arrow mesh");
	HRESULT hr = wfReader.LoadVBO(modelPath.c_str());
	printHResult(hr);

	// Create the vertex buffer
	D3D11_BUFFER_DESC vertexBufferDesc = {};
	vertexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
	vertexBufferDesc.ByteWidth = UINT(wfReader.vertices.size() * sizeof(WaveFrontReader<uint16_t>::Vertex));
	vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vertexBufferDesc.CPUAccessFlags = 0;

	D3D11_SUBRESOURCE_DATA vertexBufferData = {};
	vertexBufferData.pSysMem = &wfReader.vertices[0];

	pDevice->CreateBuffer(&vertexBufferDesc, &vertexBufferData, &m_pArrowVertexBuffer);

	// Create the index buffer
	D3D11_BUFFER_DESC indexBufferDesc = {};
	indexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
	indexBufferDesc.ByteWidth = UINT(wfReader.indices.size()*sizeof(uint16_t));
	indexBufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
	indexBufferDesc.CPUAccessFlags = 0;

	D3D11_SUBRESOURCE_DATA indexBufferData = {};
	indexBufferData.pSysMem = &wfReader.indices[0];

	pDevice->CreateBuffer(&indexBufferDesc, &indexBufferData, &m_pArrowIndexBuffer);
}

void MarkerRender::CreateLayoutsAndStates(ID3D11Device* pDevice) {
	// Define the input layout
	D3D11_INPUT_ELEMENT_DESC inputLayoutDesc[] = {
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 }
	};

	// Create the input layout
	pDevice->CreateInputLayout(
		inputLayoutDesc, 1,
		m_pVertexShaderBlob->GetBufferPointer(),
		m_pVertexShaderBlob->GetBufferSize(),
		&m_pInputLayout);

	//create wireframe rasterizer state for testing
	D3D11_RASTERIZER_DESC rsState;
	rsState.FillMode = D3D11_FILL_WIREFRAME;
	rsState.CullMode = D3D11_CULL_NONE;
	rsState.FrontCounterClockwise = false;
	rsState.DepthBias = 0;
	rsState.DepthBiasClamp = 0;
	rsState.SlopeScaledDepthBias = 0;
	rsState.DepthClipEnable = false;
	rsState.ScissorEnable = false;
	rsState.MultisampleEnable = true;
	rsState.AntialiasedLineEnable = false;
	pDevice->CreateRasterizerState(&rsState, &m_pWireframeRSState);

	//depth stencil state
	m_pDepthStencilState = Util::CreateDepthStencilState(pDevice, false, D3D11_DEPTH_WRITE_MASK_ZERO, D3D11_COMPARISON_NEVER, false, 0b00000000, 0b00000000, D3D11_STENCIL_OP_KEEP, D3D11_STENCIL_OP_KEEP, D3D11_STENCIL_OP_KEEP, D3D11_COMPARISON_NEVER);
}

void MarkerRender::CreateBuffers(ID3D11Device* pDevice)
{
	// Create the vertex buffer
	D3D11_BUFFER_DESC vertexBufferDesc = {};
	vertexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
	vertexBufferDesc.ByteWidth = sizeof(Vertex) * 8;
	vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vertexBufferDesc.CPUAccessFlags = 0;

	D3D11_SUBRESOURCE_DATA vertexBufferData = {};
	vertexBufferData.pSysMem = m_cubeVertices;

	pDevice->CreateBuffer(&vertexBufferDesc, &vertexBufferData, &m_pVertexBuffer);

	// Create the index buffer
	D3D11_BUFFER_DESC indexBufferDesc = {};
	indexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
	indexBufferDesc.ByteWidth = sizeof(DWORD) * 36;
	indexBufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
	indexBufferDesc.CPUAccessFlags = 0;

	D3D11_SUBRESOURCE_DATA indexBufferData = {};
	indexBufferData.pSysMem = m_cubeIndices;

	pDevice->CreateBuffer(&indexBufferDesc, &indexBufferData, &m_pIndexBuffer);

	// Create the VS constant buffer
	D3D11_BUFFER_DESC constantBufferDesc = {};
	constantBufferDesc.Usage = D3D11_USAGE_DEFAULT;
	constantBufferDesc.ByteWidth = sizeof(CBMatrix);
	constantBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	constantBufferDesc.CPUAccessFlags = 0;

	pDevice->CreateBuffer(&constantBufferDesc, nullptr, &m_pConstantBuffer);
}

void MarkerRender::CompileShaders(ID3D11Device* pDevice) {
	ID3D10Blob* errorMessageBlob = nullptr;

	UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;

	logger::info("Compile vertex shader");
	const auto vsPath = std::filesystem::current_path() /= "data\\Shaders\\MarkerRender\\marker.hlsl"sv;
	HRESULT vsResult = D3DCompileFromFile(
        vsPath.c_str(),               // Vertex shader source code
		nullptr,             // Optional macros
		nullptr,             // Optional include handler
		"VS",                // Name of the entry point function
		"vs_5_0",            // Target shader model
		compileFlags,        // Shader compile flags
		0,                   // Effect compile flags
		&m_pVertexShaderBlob,   // Pointer to the compiled shader
		&errorMessageBlob    // Pointer to the error message (if any)
	);

	if (FAILED(vsResult)) {
		// Handle shader compilation error
		if (errorMessageBlob) {
			char* errorMessage = static_cast<char*>(errorMessageBlob->GetBufferPointer());
			logger::info("Error compiling marker vertex shader: {}", errorMessage);
		}
	}

	logger::info("Result:");
	printHResult(vsResult);

	logger::info("Compile pixel shader");
	const auto psPath = std::filesystem::current_path() /= "data\\Shaders\\MarkerRender\\marker.hlsl"sv;
	HRESULT psResult = D3DCompileFromFile(
        psPath.c_str(),          // Vertex shader source code
		nullptr,                 // Optional macros
		nullptr,                 // Optional include handler
		"PS",                    // Name of the entry point function
		"ps_5_0",                // Target shader model
		compileFlags,			 // Shader compile flags
		0,                       // Effect compile flags
		&m_pPixelShaderBlob,    // Pointer to the compiled shader
		&errorMessageBlob        // Pointer to the error message (if any)
	);

	if (FAILED(psResult)) {
		// Handle shader compilation error
		if (errorMessageBlob) {
			char* errorMessage = static_cast<char*>(errorMessageBlob->GetBufferPointer());
			logger::info("Error compiling marker pixel shader: {}", errorMessage);
		}
	}

	logger::info("Result:");
	printHResult(psResult);
	logger::info("Pixel blob address: {}", Util::ptr_to_string(m_pPixelShaderBlob->GetBufferPointer()));
	logger::info("Vertex blob address: {}", Util::ptr_to_string(m_pVertexShaderBlob->GetBufferPointer()));
	logger::info("D3D11Device address: {}", Util::ptr_to_string(pDevice));

	logger::info("Create vertex shader");
	vsResult = pDevice->CreateVertexShader(
		m_pVertexShaderBlob->GetBufferPointer(),
		m_pVertexShaderBlob->GetBufferSize(),
		nullptr,
		&m_pVertexShader);
	
	if (FAILED(vsResult)) {
		logger::info("Error creating marker vertex shader from blob");
	}

	logger::info("Result:");
	printHResult(vsResult);

	logger::info("Create pixel shader");
	psResult = pDevice->CreatePixelShader(
		m_pPixelShaderBlob->GetBufferPointer(),
		m_pPixelShaderBlob->GetBufferSize(),
		nullptr,
		&m_pPixelShader);

	if (FAILED(psResult)) {
		logger::info("Error creating marker pixel shader from blob");
	}

	logger::info("Result:");
	printHResult(psResult);
}
