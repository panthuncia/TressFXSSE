#include "MarkerRender.h"
#include <d3dcompiler.h>
#include <vector>
MarkerRender::MarkerRender() {
	InitRenderResources();
}

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
}

void MarkerRender::DrawMarkers(std::vector<DirectX::XMFLOAT3> worldPositions, DirectX::XMMATRIX viewMatrix, DirectX::XMMATRIX projectionMatrix)
{
	logger::info("In draw markers");
	// Set the vertex buffer and index buffer
	ID3D11DeviceContext* pDeviceContext = RE::BSGraphics::Renderer::GetSingleton()->GetRuntimeData().context;
	UINT stride = sizeof(Vertex);
	UINT offset = 0;
	pDeviceContext->IASetVertexBuffers(0, 1, &m_pVertexBuffer, &stride, &offset);
	pDeviceContext->IASetIndexBuffer(m_pIndexBuffer, DXGI_FORMAT_R32_UINT, 0);
	pDeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	pDeviceContext->IASetInputLayout(m_pInputLayout);
	logger::info("1");
	// Set the vertex shader and input layout
	pDeviceContext->VSSetShader(m_pVertexShader, nullptr, 0);
	pDeviceContext->VSSetConstantBuffers(0, 1, &m_pConstantBuffer);

	// Set the pixel shader
	pDeviceContext->PSSetShader(m_pPixelShader, nullptr, 0);
	logger::info("2");
	CBMatrix cbMatrix;
	cbMatrix.viewMatrix = viewMatrix;
	cbMatrix.projectionMatrix = projectionMatrix;

	//set rasterizer state
	pDeviceContext->RSSetState(m_pWireframeRSState);

	//draw markers
	for (const DirectX::XMFLOAT3& worldPosition : worldPositions) {
		logger::info("Drawing marker");
		auto worldMatrix = DirectX::XMMatrixTranspose(DirectX::XMMatrixTranslation(worldPosition.x, worldPosition.y, worldPosition.z));
		// Set the constant buffer data
		cbMatrix.worldMatrix = worldMatrix;
		pDeviceContext->UpdateSubresource(m_pConstantBuffer, 0, nullptr, &cbMatrix, 0, 0);
		// Render the cube
		pDeviceContext->DrawIndexed(36, 0, 0);
	}
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

	// Create the constant buffer
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
	const auto vsPath = std::filesystem::current_path() /= "data\\Shaders\\MarkerRender\\markerVS.hlsl"sv;
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
	const auto psPath = std::filesystem::current_path() /= "data\\Shaders\\MarkerRender\\markerPS.hlsl"sv;
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
	logger::info("Pixel blob address: {}", ptr_to_string(m_pPixelShaderBlob->GetBufferPointer()));
	logger::info("Vertex blob address: {}", ptr_to_string(m_pVertexShaderBlob->GetBufferPointer()));
	logger::info("D3D11Device address: {}", ptr_to_string(pDevice));

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
