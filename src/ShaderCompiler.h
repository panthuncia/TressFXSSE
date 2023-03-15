#pragma once

#include "d3d11.h"
#include "Effects11/d3dx11effect.h"
namespace ShaderCompiler
{
	ID3D11DeviceChild* CompileShader(const wchar_t* FilePath, const std::vector<std::pair<const char*, const char*>>& Defines, const char* ProgramType);
	ID3DX11Effect* CompileAndRegisterEffectShader(const std::wstring a_filePath, ID3D11Device* device, std::vector<D3D_SHADER_MACRO> defines = std::vector<D3D_SHADER_MACRO>());
}
