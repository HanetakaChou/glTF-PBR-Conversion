#define NOMINMAX 1
#define WIN32_LEAN_AND_MEAN 1
#include <sdkddkver.h>
#include <Windows.h>
#include <windowsx.h>
#include <dxgi1_4.h>
#include <d3d11.h>
#include <stdint.h>
#include <stddef.h>
#include <assert.h>
#include "demo.h"

static int g_window_width;
static int g_window_height;
static ID3D11Device *g_device;
static ID3D11DeviceContext *g_device_context;
static IDXGISwapChain *g_swap_chain;
static ID3D11RenderTargetView *g_swap_chain_back_buffer_rtv;
static class Demo g_demo;

static LRESULT CALLBACK wnd_proc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

int APIENTRY wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE, _In_ LPWSTR lpCmdLine, _In_ int nCmdShow)
{
	g_window_width = 512;
	g_window_height = 512;

	ATOM hWndCls;
	{
		WNDCLASSEXW Desc = {
			sizeof(WNDCLASSEX),
			CS_OWNDC,
			wnd_proc,
			0,
			0,
			hInstance,
			LoadIconW(NULL, IDI_APPLICATION),
			LoadCursorW(NULL, IDC_ARROW),
			(HBRUSH)(COLOR_WINDOW + 1),
			NULL,
			L"glTF-PBR-Conversion:0XFFFFFFFF",
			LoadIconW(NULL, IDI_APPLICATION),
		};
		hWndCls = RegisterClassExW(&Desc);
	}

	HWND hWnd;
	{
		HWND hDesktop = GetDesktopWindow();
		HMONITOR hMonitor = MonitorFromWindow(hDesktop, MONITOR_DEFAULTTONEAREST);
		MONITORINFOEXW MonitorInfo;
		MonitorInfo.cbSize = sizeof(MONITORINFOEXW);
		GetMonitorInfoW(hMonitor, &MonitorInfo);

		constexpr DWORD const dw_style = WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_THICKFRAME;
		constexpr DWORD const dw_ex_style = WS_EX_APPWINDOW;

		RECT rect = {(MonitorInfo.rcWork.left + MonitorInfo.rcWork.right) / 2 - g_window_width / 2,
					 (MonitorInfo.rcWork.bottom + MonitorInfo.rcWork.top) / 2 - g_window_height / 2,
					 (MonitorInfo.rcWork.left + MonitorInfo.rcWork.right) / 2 + g_window_width / 2,
					 (MonitorInfo.rcWork.bottom + MonitorInfo.rcWork.top) / 2 + g_window_height / 2};
		AdjustWindowRectEx(&rect, dw_style, FALSE, dw_ex_style);

		hWnd = CreateWindowExW(dw_ex_style, MAKEINTATOM(hWndCls), L"glTF PBR Conversion", dw_style, rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top, hDesktop, NULL, hInstance, NULL);
	}

	g_device = NULL;
	g_device_context = NULL;
	g_swap_chain = NULL;
	g_swap_chain_back_buffer_rtv = NULL;
	{
		IDXGIFactory *factory = NULL;
		{
			HRESULT hr_create_factory = CreateDXGIFactory2(
#ifndef NDEBUG
				DXGI_CREATE_FACTORY_DEBUG,
#else
				0U,
#endif
				IID_PPV_ARGS(&factory));
			assert(SUCCEEDED(hr_create_factory));
		}

		IDXGIAdapter *adapter = NULL;
		{
			IDXGIAdapter *first_discrete_gpu_adapter = NULL;
			IDXGIAdapter *first_non_discrete_gpu_adapter = NULL;
			for (UINT adapter_index = 0U; adapter_index < 7U; ++adapter_index)
			{
				IDXGIAdapter *new_adapter = NULL;
				HRESULT hr_enum_adapters = SUCCEEDED(factory->EnumAdapters(adapter_index, &new_adapter));
				if (!(SUCCEEDED(hr_enum_adapters)))
				{
					break;
				}

				IDXGIAdapter3 *new_adapter_3 = NULL;
				HRESULT hr_query_interface = new_adapter->QueryInterface(IID_PPV_ARGS(&new_adapter_3));
				assert(SUCCEEDED(hr_query_interface));

				DXGI_QUERY_VIDEO_MEMORY_INFO local_video_memory_info;
				HRESULT hr_query_local_video_memory_info = new_adapter_3->QueryVideoMemoryInfo(0U, DXGI_MEMORY_SEGMENT_GROUP_LOCAL, &local_video_memory_info);
				assert(SUCCEEDED(hr_query_local_video_memory_info));

				DXGI_QUERY_VIDEO_MEMORY_INFO non_local_video_memory_info;
				HRESULT hr_query_non_local_video_memory_info = new_adapter_3->QueryVideoMemoryInfo(0U, DXGI_MEMORY_SEGMENT_GROUP_NON_LOCAL, &non_local_video_memory_info);
				assert(SUCCEEDED(hr_query_non_local_video_memory_info));

				new_adapter_3->Release();

				if (local_video_memory_info.Budget > 512U && non_local_video_memory_info.Budget > 512U)
				{
					assert(NULL == first_discrete_gpu_adapter);
					first_discrete_gpu_adapter = new_adapter;
					break;
				}
				else
				{
					// we should already "break" and exit the loop
					assert(NULL == first_discrete_gpu_adapter);

					if (NULL == first_non_discrete_gpu_adapter)
					{
						first_non_discrete_gpu_adapter = new_adapter;
					}
					else
					{
						new_adapter->Release();
					}
				}
			}

			assert(NULL == adapter);
			if (NULL != first_discrete_gpu_adapter)
			{
				adapter = first_discrete_gpu_adapter;
				first_discrete_gpu_adapter = NULL;

				if (NULL != first_non_discrete_gpu_adapter)
				{
					first_non_discrete_gpu_adapter->Release();
					first_non_discrete_gpu_adapter = NULL;
				}
			}
			else if (NULL != first_non_discrete_gpu_adapter)
			{
				// The discrete gpu is preferred
				assert(NULL == first_discrete_gpu_adapter);

				adapter = first_non_discrete_gpu_adapter;
				first_non_discrete_gpu_adapter = NULL;
			}
			else
			{
				assert(false);
			}
		}

		D3D_FEATURE_LEVEL pFeatureLevels[1] = {D3D_FEATURE_LEVEL_11_0};
		HRESULT res_create_device = D3D11CreateDevice(
			adapter,
			D3D_DRIVER_TYPE_UNKNOWN,
			NULL,
#ifndef NDEBUG
			D3D11_CREATE_DEVICE_SINGLETHREADED | D3D11_CREATE_DEVICE_DEBUG,
#else
			D3D11_CREATE_DEVICE_SINGLETHREADED,
#endif
			pFeatureLevels,
			sizeof(pFeatureLevels) / sizeof(pFeatureLevels[0]),
			D3D11_SDK_VERSION,
			&g_device,
			NULL,
			&g_device_context);
		assert(SUCCEEDED(res_create_device));

		uint32_t new_swap_chain_image_width = -1;
		uint32_t new_swap_chain_image_height = -1;
		{
			RECT rect;

			BOOL res_get_clent_rect = GetClientRect(hWnd, &rect);
			assert(FALSE != res_get_clent_rect);

			new_swap_chain_image_width = rect.right - rect.left;
			new_swap_chain_image_height = rect.bottom - rect.top;
		}

		DXGI_SWAP_CHAIN_DESC swap_chain_desc;
		swap_chain_desc.BufferDesc.Width = new_swap_chain_image_width;
		swap_chain_desc.BufferDesc.Height = new_swap_chain_image_height;
		swap_chain_desc.BufferDesc.RefreshRate.Numerator = 60U;
		swap_chain_desc.BufferDesc.RefreshRate.Denominator = 1U;
		swap_chain_desc.BufferDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
		swap_chain_desc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
		swap_chain_desc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
		swap_chain_desc.SampleDesc.Count = 1U;
		swap_chain_desc.SampleDesc.Quality = 0U;
		swap_chain_desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		swap_chain_desc.BufferCount = 2U;
		swap_chain_desc.OutputWindow = hWnd;
		swap_chain_desc.Windowed = TRUE;
		swap_chain_desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
		swap_chain_desc.Flags = 0U;
		HRESULT res_factory_create_swap_chain = factory->CreateSwapChain(g_device, &swap_chain_desc, &g_swap_chain);
		assert(SUCCEEDED(res_factory_create_swap_chain));

		ID3D11Texture2D *swap_chain_back_buffer = NULL;
		HRESULT res_swap_chain_get_buffer = g_swap_chain->GetBuffer(0U, IID_PPV_ARGS(&swap_chain_back_buffer));
		assert(SUCCEEDED(res_swap_chain_get_buffer));

		D3D11_RENDER_TARGET_VIEW_DESC render_target_view_desc;
		render_target_view_desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
		render_target_view_desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
		render_target_view_desc.Texture2D.MipSlice = 0U;

		HRESULT res_device_create_render_target_view = g_device->CreateRenderTargetView(swap_chain_back_buffer, &render_target_view_desc, &g_swap_chain_back_buffer_rtv);
		assert(SUCCEEDED(res_device_create_render_target_view));

		swap_chain_back_buffer->Release();
		swap_chain_back_buffer = NULL;

		adapter->Release();
		factory->Release();
	}

	g_demo.Init(g_device, g_device_context);

	ShowWindow(hWnd, SW_SHOWDEFAULT);

	{
		BOOL result_update_window = UpdateWindow(hWnd);
		assert(FALSE != result_update_window);
	}

	MSG msg;
	while (GetMessageW(&msg, NULL, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessageW(&msg);
	}

	g_demo.Destroy(g_device, g_device_context);

	g_swap_chain_back_buffer_rtv->Release();
	g_swap_chain->Release();
	g_device_context->Release();
	g_device->Release();

	return (int)msg.wParam;
}

static LRESULT CALLBACK wnd_proc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	static float g_SpeedTranslate = 0.5f;

	switch (message)
	{
	case WM_DESTROY:
	{
		PostQuitMessage(0);
	}
		return 0;
	case WM_PAINT:
	{
		g_demo.Tick(g_device, g_device_context, g_swap_chain_back_buffer_rtv, g_window_width, g_window_height);

		HRESULT res_swap_chain_present = g_swap_chain->Present(1U, 0U);
		assert(SUCCEEDED(res_swap_chain_present));
	}
		return 0;
	case WM_SIZE:
	{
		WORD const new_width = LOWORD(lParam);
		WORD const new_height = HIWORD(lParam);

		if (g_window_width != new_width || g_window_height != new_height)
		{
			g_swap_chain_back_buffer_rtv->Release();
			g_swap_chain_back_buffer_rtv = NULL;

			g_swap_chain->ResizeBuffers(2U, new_width, new_height, DXGI_FORMAT_B8G8R8A8_UNORM, 0U);

			ID3D11Texture2D *swap_chain_back_buffer = NULL;
			HRESULT res_swap_chain_get_buffer = g_swap_chain->GetBuffer(0U, IID_PPV_ARGS(&swap_chain_back_buffer));
			assert(SUCCEEDED(res_swap_chain_get_buffer));

			D3D11_RENDER_TARGET_VIEW_DESC render_target_view_desc;
			render_target_view_desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
			render_target_view_desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
			render_target_view_desc.Texture2D.MipSlice = 0U;

			HRESULT res_device_create_render_target_view = g_device->CreateRenderTargetView(swap_chain_back_buffer, &render_target_view_desc, &g_swap_chain_back_buffer_rtv);
			assert(SUCCEEDED(res_device_create_render_target_view));

			swap_chain_back_buffer->Release();
			swap_chain_back_buffer = NULL;

			g_window_width = new_width;
			g_window_height = new_height;
		}
	}
		return 0;
	case WM_ERASEBKGND:
		return 1;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
}