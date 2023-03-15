#include "Hair.h"
#include <sys/stat.h>
Hair::Hair(AMD::TressFXAsset* asset, SkyrimGPUResourceManager* resourceManager, ID3D11DeviceContext* context, EI_StringHash name) {
	deviceContext = context;
	initialize(resourceManager);
	hairEIResource.srv = hairSRV;
	hairObject.Create(asset, (EI_Device*)resourceManager, (EI_CommandContextRef)context, name, &hairEIResource);
	logger::info("Created hair object");
	hairs["hairTest"] = this;
}
void Hair::draw() {
	//hairObject.DrawStrands((EI_CommandContextRef)deviceContext, );
}
void Hair::initialize(SkyrimGPUResourceManager* pManager) {
	//create texture and SRV (empty for now)
	D3D11_TEXTURE2D_DESC desc;
	desc.Width = 256;
	desc.Height = 256;
	desc.MipLevels = desc.ArraySize = 1;
	desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	desc.SampleDesc.Count = 1;
	desc.Usage = D3D11_USAGE_DYNAMIC;
	desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	desc.MiscFlags = 0;
	pManager->device->CreateTexture2D(&desc, NULL, &hairTexture);
	pManager->device->CreateShaderResourceView(hairTexture, nullptr, &hairSRV);
	//compile effect
	const auto path = std::filesystem::current_path() /= "data\\Shaders\\TressFX\\oHair.fx"sv;
	struct stat buffer;
	if (stat(path.string().c_str(), &buffer) == 0) {
		logger::info("file exists");
	}
	else {
		logger::error("Effect file {} missing!", path.string());
		throw std::runtime_error("File not found");
	}
	logger::info("Compiling hair effect shader");
	hairEffect = ShaderCompiler::CompileAndRegisterEffectShader(path.wstring(), pManager->device);
}
