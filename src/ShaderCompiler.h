#pragma once

#include "d3d11.h"
namespace ShaderCompiler
{
	ID3D11DeviceChild* CompileShader(const wchar_t* FilePath, const char* entryPoint, std::vector<D3D_SHADER_MACRO> Defines, const char* ProgramType);
}
