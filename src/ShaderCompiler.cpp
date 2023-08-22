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
}
