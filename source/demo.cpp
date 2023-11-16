
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX 1
#include <sdkddkver.h>
#include <windows.h>
#include <stdint.h>
#include <assert.h>
#include <cmath>
#include <algorithm>
#include <DirectXMath.h>
#include <dxgi.h>
#include <d3d11.h>
#include "../thirdparty/DXUT/Optional/SDKmisc.h"
#include "demo.h"

void Demo::Init(ID3D11Device *device, ID3D11DeviceContext *device_context)
{
	this->m_sampler = NULL;
	{
		D3D11_SAMPLER_DESC sampler_desc;
		sampler_desc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
		sampler_desc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
		sampler_desc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
		sampler_desc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
		sampler_desc.MipLODBias = 0.0;
		sampler_desc.MaxAnisotropy = 0U;
		sampler_desc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
		sampler_desc.BorderColor[0] = 0.0;
		sampler_desc.BorderColor[1] = 0.0;
		sampler_desc.BorderColor[2] = 0.0;
		sampler_desc.BorderColor[3] = 1.0;
		sampler_desc.MinLOD = 0.0;
		sampler_desc.MaxLOD = 4096.0;

		HRESULT res_device_create_sampler = device->CreateSamplerState(&sampler_desc, &this->m_sampler);
		assert(SUCCEEDED(res_device_create_sampler));
	}

	this->m_diffuse_srv = NULL;
	uint32_t diffuse_width = -1;
	uint32_t diffuse_height = -1;
	{
		HRESULT res_create_shader_resource_view_from_file = DXUTCreateShaderResourceViewFromFile(device, L"diffuse.dds", &this->m_diffuse_srv);
		assert(SUCCEEDED(res_create_shader_resource_view_from_file));

		ID3D11Resource *tmp_resource = NULL;
		this->m_diffuse_srv->GetResource(&tmp_resource);

		ID3D11Texture2D *tmp_texture_2d = NULL;
		HRESULT res_query_interface = tmp_resource->QueryInterface(IID_PPV_ARGS(&tmp_texture_2d));
		assert(SUCCEEDED(res_query_interface));

		tmp_resource->Release();

		D3D11_TEXTURE2D_DESC desc;
		tmp_texture_2d->GetDesc(&desc);
		diffuse_width = desc.Width;
		diffuse_height = desc.Height;

		tmp_texture_2d->Release();
	}

	this->m_specular_srv = NULL;
	uint32_t specular_width = -1;
	uint32_t specular_height = -1;
	{
		HRESULT res_create_shader_resource_view_from_file = DXUTCreateShaderResourceViewFromFile(device, L"specular.dds", &this->m_specular_srv);
		assert(SUCCEEDED(res_create_shader_resource_view_from_file));

		ID3D11Resource *tmp_resource = NULL;
		this->m_specular_srv->GetResource(&tmp_resource);

		ID3D11Texture2D *tmp_texture_2d = NULL;
		HRESULT res_query_interface = tmp_resource->QueryInterface(IID_PPV_ARGS(&tmp_texture_2d));
		assert(SUCCEEDED(res_query_interface));

		tmp_resource->Release();

		D3D11_TEXTURE2D_DESC desc;
		tmp_texture_2d->GetDesc(&desc);
		specular_width = desc.Width;
		specular_height = desc.Height;

		tmp_texture_2d->Release();
	}

	this->m_glossiness_srv = NULL;
	uint32_t glossiness_width = -1;
	uint32_t glossiness_height = -1;
	{
		HRESULT res_create_shader_resource_view_from_file = DXUTCreateShaderResourceViewFromFile(device, L"glossiness.dds", &this->m_glossiness_srv);
		assert(SUCCEEDED(res_create_shader_resource_view_from_file));

		ID3D11Resource *tmp_resource = NULL;
		this->m_glossiness_srv->GetResource(&tmp_resource);

		ID3D11Texture2D *tmp_texture_2d = NULL;
		HRESULT res_query_interface = tmp_resource->QueryInterface(IID_PPV_ARGS(&tmp_texture_2d));
		assert(SUCCEEDED(res_query_interface));

		tmp_resource->Release();

		D3D11_TEXTURE2D_DESC desc;
		tmp_texture_2d->GetDesc(&desc);
		glossiness_width = desc.Width;
		glossiness_height = desc.Height;

		tmp_texture_2d->Release();
	}

	this->m_material_width = std::max(std::max(diffuse_width, specular_width), glossiness_width);
	this->m_material_height = std::max(std::max(diffuse_height, specular_height), glossiness_width);

	this->m_base_color = NULL;
	this->m_base_color_rtv = NULL;
	this->m_base_color_srv = NULL;
	{
		D3D11_TEXTURE2D_DESC texture2d_desc;
		texture2d_desc.Width = this->m_material_width;
		texture2d_desc.Height = this->m_material_height;
		texture2d_desc.MipLevels = 1U;
		texture2d_desc.ArraySize = 1U;
		texture2d_desc.Format = DXGI_FORMAT_R16G16B16A16_UNORM;
		texture2d_desc.SampleDesc.Count = 1U;
		texture2d_desc.SampleDesc.Quality = 0U;
		texture2d_desc.Usage = D3D11_USAGE_DEFAULT;
		texture2d_desc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
		texture2d_desc.CPUAccessFlags = 0U;
		texture2d_desc.MiscFlags = 0U;

		HRESULT res_device_create_texture_2d = device->CreateTexture2D(&texture2d_desc, NULL, &this->m_base_color);
		assert(SUCCEEDED(res_device_create_texture_2d));

		WCHAR const debug_object_name[] = {L"Base Color"};
		HRESULT res_device_child_set_debug_object_name = this->m_base_color->SetPrivateData(WKPDID_D3DDebugObjectNameW, sizeof(debug_object_name), debug_object_name);
		assert(SUCCEEDED(res_device_child_set_debug_object_name));

		D3D11_RENDER_TARGET_VIEW_DESC render_target_view_desc;
		render_target_view_desc.Format = DXGI_FORMAT_R16G16B16A16_UNORM;
		render_target_view_desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
		render_target_view_desc.Texture2D.MipSlice = 0U;

		HRESULT res_device_create_render_target_view = device->CreateRenderTargetView(this->m_base_color, &render_target_view_desc, &this->m_base_color_rtv);
		assert(SUCCEEDED(res_device_create_render_target_view));

		D3D11_SHADER_RESOURCE_VIEW_DESC shader_resource_view_desc;
		shader_resource_view_desc.Format = DXGI_FORMAT_R16G16B16A16_UNORM;
		shader_resource_view_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
		shader_resource_view_desc.Texture2D.MostDetailedMip = 0U;
		shader_resource_view_desc.Texture2D.MipLevels = 1U;

		HRESULT res_device_create_shader_resource_view = device->CreateShaderResourceView(this->m_base_color, &shader_resource_view_desc, &this->m_base_color_srv);
		assert(SUCCEEDED(res_device_create_shader_resource_view));
	}

	this->m_metallic_roughness = NULL;
	this->m_metallic_roughness_rtv = NULL;
	this->m_metallic_roughness_srv = NULL;
	{
		D3D11_TEXTURE2D_DESC texture2d_desc;
		texture2d_desc.Width = this->m_material_width;
		texture2d_desc.Height = this->m_material_height;
		texture2d_desc.MipLevels = 1U;
		texture2d_desc.ArraySize = 1U;
		texture2d_desc.Format = DXGI_FORMAT_R16G16B16A16_UNORM;
		texture2d_desc.SampleDesc.Count = 1U;
		texture2d_desc.SampleDesc.Quality = 0U;
		texture2d_desc.Usage = D3D11_USAGE_DEFAULT;
		texture2d_desc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
		texture2d_desc.CPUAccessFlags = 0U;
		texture2d_desc.MiscFlags = 0U;

		HRESULT res_device_create_texture_2d = device->CreateTexture2D(&texture2d_desc, NULL, &this->m_metallic_roughness);
		assert(SUCCEEDED(res_device_create_texture_2d));

		WCHAR const debug_object_name[] = {L"Metallic Roughness"};
		HRESULT res_device_child_set_debug_object_name = this->m_metallic_roughness->SetPrivateData(WKPDID_D3DDebugObjectNameW, sizeof(debug_object_name), debug_object_name);
		assert(SUCCEEDED(res_device_child_set_debug_object_name));

		D3D11_RENDER_TARGET_VIEW_DESC render_target_view_desc;
		render_target_view_desc.Format = DXGI_FORMAT_R16G16B16A16_UNORM;
		render_target_view_desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
		render_target_view_desc.Texture2D.MipSlice = 0U;

		HRESULT res_device_create_render_target_view = device->CreateRenderTargetView(this->m_metallic_roughness, &render_target_view_desc, &this->m_metallic_roughness_rtv);
		assert(SUCCEEDED(res_device_create_render_target_view));

		D3D11_SHADER_RESOURCE_VIEW_DESC shader_resource_view_desc;
		shader_resource_view_desc.Format = DXGI_FORMAT_R16G16B16A16_UNORM;
		shader_resource_view_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
		shader_resource_view_desc.Texture2D.MostDetailedMip = 0U;
		shader_resource_view_desc.Texture2D.MipLevels = 1U;

		HRESULT res_device_create_shader_resource_view = device->CreateShaderResourceView(this->m_metallic_roughness, &shader_resource_view_desc, &this->m_metallic_roughness_srv);
		assert(SUCCEEDED(res_device_create_shader_resource_view));
	};

	this->m_full_screen_vs = NULL;
	{
#include "_internal_full_screen_vertex.inl"
		HRESULT res_device_create_vertex_shader = device->CreateVertexShader(full_screen_vertex_shader_module_code, sizeof(full_screen_vertex_shader_module_code), NULL, &this->m_full_screen_vs);
		assert(SUCCEEDED(res_device_create_vertex_shader));
	}

	this->m_full_screen_rs = NULL;
	{
		D3D11_RASTERIZER_DESC rasterizer_desc;
		rasterizer_desc.FillMode = D3D11_FILL_SOLID;
		rasterizer_desc.CullMode = D3D11_CULL_BACK;
		rasterizer_desc.FrontCounterClockwise = TRUE;
		rasterizer_desc.DepthBias = 0;
		rasterizer_desc.SlopeScaledDepthBias = 0.0f;
		rasterizer_desc.DepthBiasClamp = 0.0f;
		rasterizer_desc.DepthClipEnable = TRUE;
		rasterizer_desc.ScissorEnable = FALSE;
		rasterizer_desc.MultisampleEnable = FALSE;
		rasterizer_desc.AntialiasedLineEnable = FALSE;
		rasterizer_desc.MultisampleEnable = FALSE;
		HRESULT res_device_create_rasterizer_state = device->CreateRasterizerState(&rasterizer_desc, &this->m_full_screen_rs);
		assert(SUCCEEDED(res_device_create_rasterizer_state));
	}

	this->m_full_screen_conversion_ps = NULL;
	{
#include "_internal_full_screen_conversion_pixel.inl"
		HRESULT res_device_create_pixel_shader = device->CreatePixelShader(full_screen_conversion_pixel_shader_module_code, sizeof(full_screen_conversion_pixel_shader_module_code), NULL, &this->m_full_screen_conversion_ps);
		assert(SUCCEEDED(res_device_create_pixel_shader));
	}

	this->m_full_screen_shading_ps = NULL;
	{
#include "_internal_full_screen_transfer_pixel.inl"
		HRESULT res_device_create_pixel_shader = device->CreatePixelShader(full_screen_transfer_pixel_shader_module_code, sizeof(full_screen_transfer_pixel_shader_module_code), NULL, &this->m_full_screen_shading_ps);
		assert(SUCCEEDED(res_device_create_pixel_shader));
	}
}

void Demo::Tick(ID3D11Device *device, ID3D11DeviceContext *device_context, ID3D11RenderTargetView *swap_chain_back_buffer_rtv, uint32_t swap_chain_image_width, uint32_t swap_chain_image_height)
{
	// Conversion
	{
		ID3D11RenderTargetView *const bind_rtvs[2] = {this->m_base_color_rtv, this->m_metallic_roughness_rtv};
		device_context->OMSetRenderTargets(sizeof(bind_rtvs) / sizeof(bind_rtvs[0]), bind_rtvs, NULL);

		D3D11_VIEWPORT viewport = {0.0F, 0.0F, static_cast<FLOAT>(this->m_material_width), static_cast<FLOAT>(this->m_material_height), 0.0F, 1.0F};
		device_context->RSSetViewports(1U, &viewport);

		device_context->RSSetState(this->m_full_screen_rs);

		device_context->VSSetShader(this->m_full_screen_vs, NULL, 0U);
		device_context->PSSetShader(this->m_full_screen_conversion_ps, NULL, 0U);

		ID3D11ShaderResourceView *const bind_srvs[3] = {this->m_diffuse_srv, this->m_specular_srv, this->m_glossiness_srv};
		device_context->PSSetShaderResources(0U, sizeof(bind_srvs) / sizeof(bind_srvs[0]), bind_srvs);
		device_context->PSSetSamplers(0U, 1U, &this->m_sampler);

		device_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		device_context->DrawInstanced(3U, 1U, 0U, 0U);

		ID3D11ShaderResourceView *const unbind_srvs[3] = {NULL, NULL, NULL};
		device_context->PSSetShaderResources(0U, sizeof(unbind_srvs) / sizeof(unbind_srvs[0]), unbind_srvs);

		ID3D11RenderTargetView *const unbind_rtvs[2] = {NULL, NULL};
		device_context->OMSetRenderTargets(sizeof(unbind_rtvs) / sizeof(unbind_rtvs[0]), unbind_rtvs, NULL);
	}

	// Transfer
	{
		ID3D11RenderTargetView *const bind_rtvs[1] = {swap_chain_back_buffer_rtv};

		device_context->OMSetRenderTargets(sizeof(bind_rtvs) / sizeof(bind_rtvs[0]), bind_rtvs, NULL);

		D3D11_VIEWPORT viewport = {0.0F, 0.0F, static_cast<FLOAT>(swap_chain_image_width), static_cast<FLOAT>(swap_chain_image_height), 0.0F, 1.0F};
		device_context->RSSetViewports(1U, &viewport);

		device_context->RSSetState(this->m_full_screen_rs);

		device_context->VSSetShader(this->m_full_screen_vs, NULL, 0U);
		device_context->PSSetShader(this->m_full_screen_shading_ps, NULL, 0U);

		ID3D11ShaderResourceView *const bind_srvs[2] = {this->m_base_color_srv, this->m_metallic_roughness_srv};
		device_context->PSSetShaderResources(0U, sizeof(bind_srvs) / sizeof(bind_srvs[0]), bind_srvs);
		device_context->PSSetSamplers(0U, 1U, &this->m_sampler);

		device_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		device_context->DrawInstanced(3U, 1U, 0U, 0U);

		ID3D11ShaderResourceView *const unbind_srvs[2] = {NULL, NULL};
		device_context->PSSetShaderResources(0U, sizeof(unbind_srvs) / sizeof(unbind_srvs[0]), unbind_srvs);

		ID3D11RenderTargetView *const unbind_rtvs[1] = {NULL};
		device_context->OMSetRenderTargets(sizeof(unbind_rtvs) / sizeof(unbind_rtvs[0]), unbind_rtvs, NULL);
	}
}

void Demo::Destroy(ID3D11Device *device, ID3D11DeviceContext *device_context)
{
	this->m_sampler->Release();

	this->m_diffuse_srv->Release();

	this->m_specular_srv->Release();

	this->m_glossiness_srv->Release();

	this->m_base_color->Release();
	this->m_base_color_rtv->Release();
	this->m_base_color_srv->Release();

	this->m_metallic_roughness->Release();
	this->m_metallic_roughness_rtv->Release();
	this->m_metallic_roughness_srv->Release();

	this->m_full_screen_vs->Release();
	this->m_full_screen_rs->Release();
	this->m_full_screen_conversion_ps->Release();
	this->m_full_screen_shading_ps->Release();
}