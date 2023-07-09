#pragma once
#include <string>
#include <d3d11.h>
class MarkerRender
{
public:
	MarkerRender();
	~MarkerRender();
	void InitRenderResources();
	void DrawMarkers(std::vector<DirectX::XMFLOAT3> worldPositions, DirectX::XMMATRIX viewMatrix, DirectX::XMMATRIX projectionMatrix);

private:
	void CompileShaders(ID3D11Device* pDevice);
	void CreateBuffers(ID3D11Device* pDevice);
	void CreateLayoutsAndStates(ID3D11Device* pDevice);
	struct Vertex
	{
		DirectX::XMFLOAT3 position;
	};
	struct CBMatrix
	{
		DirectX::XMMATRIX worldMatrix;
		DirectX::XMMATRIX viewMatrix;
		DirectX::XMMATRIX projectionMatrix;
	};
	// Cube vertices
	Vertex m_cubeVertices[8] = {
		{ DirectX::XMFLOAT3(-0.5f, 0.5f, -0.5f) },
		{ DirectX::XMFLOAT3(0.5f, 0.5f, -0.5f) },
		{ DirectX::XMFLOAT3(-0.5f, -0.5f, -0.5f) },
		{ DirectX::XMFLOAT3(0.5f, -0.5f, -0.5f) },
		{ DirectX::XMFLOAT3(-0.5f, 0.5f, 0.5f) },
		{ DirectX::XMFLOAT3(0.5f, 0.5f, 0.5f) },
		{ DirectX::XMFLOAT3(-0.5f, -0.5f, 0.5f) },
		{ DirectX::XMFLOAT3(0.5f, -0.5f, 0.5f) }
	};

	// Cube indices
	DWORD m_cubeIndices[36] = {
		0, 1, 2,  // Side 0
		2, 1, 3,
		4, 0, 6,  // Side 1
		6, 0, 2,
		7, 5, 6,  // Side 2
		6, 5, 4,
		3, 1, 7,  // Side 3
		7, 1, 5,
		4, 5, 0,  // Side 4
		0, 5, 1,
		3, 7, 2,  // Side 5
		2, 7, 6
	};
	ID3D11Buffer* m_pVertexBuffer = nullptr;
	ID3D11Buffer* m_pIndexBuffer = nullptr;
	ID3D11Buffer* m_pConstantBuffer = nullptr;
	ID3D10Blob* m_pVertexShaderBlob = nullptr;
	ID3D10Blob* m_pPixelShaderBlob = nullptr;
	ID3D11VertexShader* m_pVertexShader = nullptr;
	ID3D11PixelShader* m_pPixelShader = nullptr;
	ID3D11InputLayout* m_pInputLayout = nullptr;
	ID3D11RasterizerState* m_pWireframeRSState = nullptr;

	void printHResult(HRESULT hr)
	{
		if (hr == S_OK) {
			logger::info("S_OK");
		}
		else if (hr == E_INVALIDARG) {
			logger::error("E_INVALIDARG");
		}
		else if (hr == S_FALSE) {
			logger::error("S_FALSE");
		}
		else if(hr == E_NOTIMPL)
		{
			logger::error("E_NOTIMPL");
		}
		else if(hr == E_OUTOFMEMORY)
		{
			logger::error("E_OUTOFMEMORY");
		}
		else if(hr == E_FAIL)
		{
			logger::error("E_FAIL");
		}
		else if(hr == DXGI_ERROR_INVALID_CALL)
		{
			logger::error("DXGI_ERROR_INVALID_CALL");
		}
		else if(hr == E_UNEXPECTED)
		{
			logger::error("E_UNEXPECTED");
		}
	}
	std::string ptr_to_string(void* ptr)
	{
		const void*       address = static_cast<const void*>(ptr);
		std::stringstream ss;
		ss << address;
		std::string name = ss.str();
		return name;
	}

	void PrintAllD3D11DebugMessages(ID3D11Device* d3dDevice)
	{
		Microsoft::WRL::ComPtr<ID3D11Device> m_d3dDevice;
		*(m_d3dDevice.GetAddressOf()) = d3dDevice;
		Microsoft::WRL::ComPtr<ID3D11Debug>     d3dDebug;
		Microsoft::WRL::ComPtr<ID3D11InfoQueue> d3dInfoQueue;
		HRESULT                                 hr = m_d3dDevice.As(&d3dDebug);
		if (SUCCEEDED(hr)) {
			hr = d3dDebug.As(&d3dInfoQueue);
			if (SUCCEEDED(hr)) {
#ifdef _DEBUG
				d3dInfoQueue->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_CORRUPTION, true);
				d3dInfoQueue->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_ERROR, true);
#endif
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

			D3D11_MESSAGE* message = (D3D11_MESSAGE*)malloc(message_size);  //allocate enough space
			d3dInfoQueue->GetMessage(i, message, &message_size);            //get the actual message

			//do whatever you want to do with it
			logger::info("{}", message->pDescription);

			free(message);
		}

		d3dInfoQueue->ClearStoredMessages();
	}
};
