#ifndef _DEMO_H_
#define _DEMO_H_ 1

#include <sdkddkver.h>
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN 1
#endif
#ifndef NOMINMAX
#define NOMINMAX 1
#endif
#include <windows.h>

#include <dxgi.h>
#include <d3d11.h>

class Demo
{
	ID3D11SamplerState *m_sampler;

	ID3D11ShaderResourceView *m_diffuse_srv;

	ID3D11ShaderResourceView *m_specular_srv;

	ID3D11ShaderResourceView *m_glossiness_srv;

	uint32_t m_material_width;
	uint32_t m_material_height;

	ID3D11Texture2D *m_base_color;
	ID3D11RenderTargetView *m_base_color_rtv;
	ID3D11ShaderResourceView *m_base_color_srv;

	ID3D11Texture2D *m_metallic_roughness;
	ID3D11RenderTargetView *m_metallic_roughness_rtv;
	ID3D11ShaderResourceView *m_metallic_roughness_srv;

	ID3D11VertexShader *m_full_screen_vs;
	ID3D11RasterizerState *m_full_screen_rs;
	ID3D11PixelShader *m_full_screen_conversion_ps;
	ID3D11PixelShader *m_full_screen_shading_ps;

public:
	void Init(ID3D11Device *device, ID3D11DeviceContext *device_context);
	void Tick(ID3D11Device *device, ID3D11DeviceContext *device_context, ID3D11RenderTargetView *swap_chain_back_buffer_rtv, uint32_t swap_chain_image_width, uint32_t swap_chain_image_height);
	void Destroy(ID3D11Device *device, ID3D11DeviceContext *device_context);
};

#endif