#include "ShaderCompiler.h"
#include "d3dcompiler.h"
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
	ID3D11DeviceChild* CompileShader(const wchar_t* FilePath, const std::vector<std::pair<const char*, const char*>>& Defines, const char* ProgramType)
	{
		auto device = RE::BSRenderManager::GetSingleton()->GetRuntimeData().forwarder;

		// Build defines (aka convert vector->D3DCONSTANT array)
		std::vector<D3D_SHADER_MACRO> macros;

		for (auto& i : Defines)
			macros.push_back({ i.first, i.second });

		if (!_stricmp(ProgramType, "ps_5_0"))
			macros.push_back({ "PIXELSHADER", "" });
		else if (!_stricmp(ProgramType, "vs_5_0"))
			macros.push_back({ "VERTEXSHADER", "" });
		else if (!_stricmp(ProgramType, "hs_5_0"))
			macros.push_back({ "HULLSHADER", "" });
		else if (!_stricmp(ProgramType, "ds_5_0"))
			macros.push_back({ "DOMAINSHADER", "" });
		else if (!_stricmp(ProgramType, "cs_5_0"))
			macros.push_back({ "COMPUTESHADER", "" });
		else
			return nullptr;

		// Add null terminating entry
		macros.push_back({ "WINPC", "" });
		macros.push_back({ "DX11", "" });
		macros.push_back({ nullptr, nullptr });

		// Compiler setup
		uint32_t  flags = D3DCOMPILE_DEBUG | D3DCOMPILE_ENABLE_STRICTNESS | D3DCOMPILE_OPTIMIZATION_LEVEL3;
		ID3DBlob* shaderBlob;
		ID3DBlob* shaderErrors;

		if (FAILED(D3DCompileFromFile(FilePath, macros.data(), D3D_COMPILE_STANDARD_FILE_INCLUDE, "main", ProgramType, flags, 0, &shaderBlob, &shaderErrors))) {
			logger::warn("Shader compilation failed:\n\n{}", shaderErrors ? (const char*)shaderErrors->GetBufferPointer() : "Unknown error");
			return nullptr;
		}

		if (!_stricmp(ProgramType, "ps_5_0")) {
			ID3D11PixelShader* regShader;
			device->CreatePixelShader(shaderBlob->GetBufferPointer(), shaderBlob->GetBufferSize(), nullptr, &regShader);
			return regShader;
		} else if (!_stricmp(ProgramType, "vs_5_0")){
			ID3D11VertexShader* regShader;
			device->CreateVertexShader(shaderBlob->GetBufferPointer(), shaderBlob->GetBufferSize(), nullptr, &regShader);
			return regShader;
		} else if (!_stricmp(ProgramType, "hs_5_0")){
			ID3D11HullShader* regShader;
			device->CreateHullShader(shaderBlob->GetBufferPointer(), shaderBlob->GetBufferSize(), nullptr, &regShader);
			return regShader;
		} else if (!_stricmp(ProgramType, "ds_5_0")) {
			ID3D11DomainShader* regShader;
			device->CreateDomainShader(shaderBlob->GetBufferPointer(), shaderBlob->GetBufferSize(), nullptr, &regShader);
			return regShader;
		} else if (!_stricmp(ProgramType, "cs_5_0")) {
			ID3D11ComputeShader* regShader;
			DX::ThrowIfFailed(device->CreateComputeShader(shaderBlob->GetBufferPointer(), shaderBlob->GetBufferSize(), nullptr, &regShader));
			return regShader;
		}

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
		shaderFlags = D3DCOMPILE_DEBUG;

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
}
