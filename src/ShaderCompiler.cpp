#include "ShaderCompiler.h"
#include "d3dcompiler.h"
#include "Util.h"
namespace ShaderCompiler
{
	std::string what(const std::exception_ptr& eptr = std::current_exception())
	{
		if (!eptr) { throw std::bad_exception(); }

		try { std::rethrow_exception(eptr); }
		catch (const std::exception& e) { return e.what(); }
		catch (const std::string& e) { return e; }
		catch (const char* e) { return e; }
		catch (...) { return "who knows"; }
	}
	ID3D11DeviceChild* CompileShader(const wchar_t* FilePath, const char* entryPoint, std::vector<D3D_SHADER_MACRO> Defines, const char* ProgramType)
	{
		auto device = RE::BSGraphics::Renderer::GetSingleton()->GetRuntimeData().forwarder;


		// Compiler setup
		uint32_t  flags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
		//uint32_t flags = D3DCOMPILE_ENABLE_STRICTNESS | D3DCOMPILE_OPTIMIZATION_LEVEL3;
		ID3DBlob* shaderBlob;
		ID3DBlob* shaderErrors;

		logger::info("Compiling shader");

		if (FAILED(D3DCompileFromFile(FilePath, nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, entryPoint, ProgramType, flags, 0, &shaderBlob, &shaderErrors))) {
			logger::warn("Shader compilation failed:\n\n{}", shaderErrors ? (const char*)shaderErrors->GetBufferPointer() : "Unknown error");
			return nullptr;
		}
		logger::info("Creating shader");
		if (!_stricmp(ProgramType, "ps_5_0")) {
			ID3D11PixelShader* regShader;
			HRESULT hr = device->CreatePixelShader(shaderBlob->GetBufferPointer(), shaderBlob->GetBufferSize(), nullptr, &regShader);
			if (FAILED(hr)) {
				logger::error("Pixel shader creation failed");
				Util::PrintAllD3D11DebugMessages();
				return nullptr;
			}
			return regShader;
		} else if (!_stricmp(ProgramType, "vs_5_0")){
			ID3D11VertexShader* regShader;
			HRESULT hr = device->CreateVertexShader(shaderBlob->GetBufferPointer(), shaderBlob->GetBufferSize(), nullptr, &regShader);
			if (FAILED(hr)) {
				logger::error("Vertex shader creation failed");
				Util::PrintAllD3D11DebugMessages();
				return nullptr;
			}
			return regShader;
		} else if (!_stricmp(ProgramType, "hs_5_0")){
			ID3D11HullShader* regShader;
			HRESULT hr = device->CreateHullShader(shaderBlob->GetBufferPointer(), shaderBlob->GetBufferSize(), nullptr, &regShader);
			if (FAILED(hr)) {
				logger::error("Hull shader creation failed");
				Util::PrintAllD3D11DebugMessages();
				return nullptr;
			}
			return regShader;
		} else if (!_stricmp(ProgramType, "ds_5_0")) {
			ID3D11DomainShader* regShader;
			HRESULT hr = device->CreateDomainShader(shaderBlob->GetBufferPointer(), shaderBlob->GetBufferSize(), nullptr, &regShader);
			if (FAILED(hr)) {
				logger::error("Domain shader creation failed");
				Util::PrintAllD3D11DebugMessages();
				return nullptr;
			}
			return regShader;
		} else if (!_stricmp(ProgramType, "cs_5_0")) {
			ID3D11ComputeShader* regShader;
			HRESULT hr = device->CreateComputeShader(shaderBlob->GetBufferPointer(), shaderBlob->GetBufferSize(), nullptr, &regShader);
			if (FAILED(hr)) {
				logger::error("Compute shader creation failed");
				Util::PrintAllD3D11DebugMessages();
				return nullptr;
			}
			return regShader;
		}
		logger::error("Shader creation failed, unknown type");
		return nullptr;
	}
	ID3DX11Effect* CompileAndRegisterEffectShader(const std::wstring a_filePath, ID3D11Device* device, std::vector<D3D_SHADER_MACRO> defines)
	{
		device = device;
		//compile shader
		//logger::info("1");
		ID3DX11Effect* effect;
		ID3DBlob* shaderBlob;
		ID3DBlob* errorBlob;
		DWORD shaderFlags = D3DCOMPILE_ENABLE_STRICTNESS;
		shaderFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;

		//logger::info("2");
		//char* path;
		//wcstombs(path, a_filePath.c_str(), 20);
		logger::info("Compiling from file");
			//if (FAILED(D3DX11CompileEffectFromFile(L"color.fx", nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, shaderFlags, 0, device, &effect, &errorBlob))) {
		if (FAILED(D3DCompileFromFile(a_filePath.c_str(), defines.data(), D3D_COMPILE_STANDARD_FILE_INCLUDE, NULL, "fx_5_0", shaderFlags, NULL, &shaderBlob, &errorBlob))) {
			logger::warn("Shader compilation failed:\n\n{}", errorBlob ? (const char*)errorBlob->GetBufferPointer() : "Unknown error");
			return nullptr;
		}
		logger::info("Creating effect from memory");
		logger::info("device address: {}", Util::ptr_to_string(device));
		D3DX11CreateEffectFromMemory(shaderBlob->GetBufferPointer(), shaderBlob->GetBufferSize(), 0, device, &effect);
		/*HRESULT hr = D3DX11CompileEffectFromFile(a_filePath.c_str(), nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, shaderFlags,
			0, device, &effect, &errorBlob);*/
		//if (FAILED(hr))
		//{
		//	MessageBox(nullptr, (LPCWSTR)errorBlob->GetBufferPointer(), L"error", MB_OK);
		//}
		logger::info("Done creating effect");
		return effect;
	}
	ID3DX11Effect* ShaderCompiler::CreateEffect(std::string_view filePath, ID3D11Device* pDevice, std::vector<D3D_SHADER_MACRO> defines)
	{
		//compile effect
		const auto  path = std::filesystem::current_path() /= filePath;
		struct stat buffer;
		if (stat(path.string().c_str(), &buffer) == 0) {
			logger::info("file exists");
		} else {
			logger::error("Effect file {} missing!", path.string());
			throw std::runtime_error("File not found");
		}
		logger::info("Compiling hair effect shader");
		ID3DX11Effect* pEffect = ShaderCompiler::CompileAndRegisterEffectShader(path.wstring(), pDevice, defines);
		if (pEffect->IsValid())
			logger::info("Effect is valid");
		else
			logger::info("Error: Effect is invalid!");
	/*D3DX11_EFFECT_DESC effectDesc;
	pEffect->GetDesc(&effectDesc);
	logger::info("Number of variables: {}", effectDesc.GlobalVariables);
	logger::info("Number of constant buffers: {}", effectDesc.ConstantBuffers);
	logger::info("Number of constant buffers: {}", effectDesc.ConstantBuffers);
	for (uint16_t i = 0; i < effectDesc.ConstantBuffers; ++i) {
		auto var = pEffect->GetConstantBufferByIndex(i);
		D3DX11_EFFECT_VARIABLE_DESC vardesc;
		var->GetDesc(&vardesc);
		logger::info("{}", vardesc.Name);
	}
	logger::info("Number of variables: {}", effectDesc.GlobalVariables);
	for (uint16_t i = 0; i < effectDesc.GlobalVariables; ++i) {
		auto var = pEffect->GetVariableByIndex(i);
		D3DX11_EFFECT_VARIABLE_DESC vardesc;
		var->GetDesc(&vardesc);
		logger::info("{}", vardesc.Name);
	}*/
		return pEffect;
	}
}
