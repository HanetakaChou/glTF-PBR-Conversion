//--------------------------------------------------------------------------------------
// File: SDKMisc.h
//
// Various helper functionality that is shared between SDK samples
//
// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
//
// http://go.microsoft.com/fwlink/?LinkId=320437
//--------------------------------------------------------------------------------------
#pragma once

#include <d3d11.h>

#include <cstddef>
#include <cstdint>

//--------------------------------------------------------------------------------------
// Tries to finds a media file by searching in common locations
//--------------------------------------------------------------------------------------
HRESULT WINAPI DXUTFindDXSDKMediaFileCch(_Out_writes_(cchDest) WCHAR *strDestPath,
                                         _In_ int cchDest,
                                         _In_z_ LPCWSTR strFilename);
HRESULT WINAPI DXUTSetMediaSearchPath(_In_z_ LPCWSTR strPath);
LPCWSTR WINAPI DXUTGetMediaSearchPath();

//--------------------------------------------------------------------------------------
// Texture utilities
//--------------------------------------------------------------------------------------
HRESULT WINAPI DXUTCreateShaderResourceViewFromFile(_In_ ID3D11Device *d3dDevice, _In_z_ const wchar_t *szFileName, _Outptr_ ID3D11ShaderResourceView **textureView);
HRESULT WINAPI DXUTCreateTextureFromFile(_In_ ID3D11Device *d3dDevice, _In_z_ const wchar_t *szFileName, _Outptr_ ID3D11Resource **texture);
HRESULT WINAPI DXUTSaveTextureToFile(_In_ ID3D11DeviceContext *pContext, _In_ ID3D11Resource *pSource, _In_ bool usedds, _In_z_ const wchar_t *szFileName);
