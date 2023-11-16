//
// Copyright (C) YuqiaoZhang(HanetakaChou)
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published
// by the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.
//

Texture2D g_base_color : register(t0);
Texture2D g_metallic_roughness : register(t1);
SamplerState g_sampler : register(s0);

void main(
	in float4 in_position : SV_POSITION,
	in float2 in_uv : TEXCOORD0,
	out float4 out_lighting : SV_TARGET0)
{
	float3 base_color = g_base_color.Sample(g_sampler, in_uv).rgb;
	float3 packed_metallic_roughness = g_metallic_roughness.Sample(g_sampler, in_uv).rgb;
	float roughness = packed_metallic_roughness.g;
	float metallic = packed_metallic_roughness.b;

	const float dielectric_specular = 0.04;

	float3 specular = saturate(lerp(dielectric_specular, base_color, metallic));
	float3 diffuse = saturate(base_color - specular);

	const float PI = 3.141592653589793;

	const float3 light_intensity = float3(10.0, 10.0, 10.0);
	const float3 N = float3(0.0, 1.0, 0.0);
	const float3 V = normalize(float3(1.0, 1.0, 0.0));
	const float3 L = normalize(float3(0.0, 1.0, 1.0));

	float3 H = normalize(L + V);
	float NdotL = saturate(dot(N, L));
	float NdotH = saturate(dot(N, H));
	float NdotV = saturate(dot(N, V));
	float VdotH = saturate(dot(V, H));

	float3 brdf_diffuse;
	{
		// Lambert

		brdf_diffuse = (1.0 / PI) * diffuse;
	}

	float3 brdf_specular;
	{
		// Trowbridge Reitz		

		// 
		float alpha = roughness;
	
		// Equation 9.41 of Real-Time Rendering Fourth Edition: "Although ��Trowbridge-Reitz distribution�� is technically the correct name"
		// Equation 8.11 of PBR Book: https://pbr-book.org/3ed-2018/Reflection_Models/Microfacet_Models#MicrofacetDistributionFunctions
		float alpha2 = alpha * alpha;
		float denominator = 1.0 + NdotH * (NdotH * alpha2 - NdotH);
		float D = (1.0 / PI) * (alpha2 / (denominator * denominator));

		// Lambda:
		// Equation 8.13 of PBR Book: https://pbr-book.org/3ed-2018/Reflection_Models/Microfacet_Models#MaskingandShadowing
		// Equation 9.42 of Real-Time Rendering Fourth Edition
		// Figure 8.18 of PBR Book: https://pbr-book.org/3ed-2018/Reflection_Models/Microfacet_Models#MaskingandShadowing
		// ��(V) = 0.5*(-1.0 + (1.0/NoV)*sqrt(alpha^2 + (1.0 - alpha^2)*NoV^2))
		// ��(L) = 0.5*(-1.0 + (1.0/NoL)*sqrt(alpha^2 + (1.0 - alpha^2)*NoL^2))

		// G2
		// Equation 9.31 of Real-Time Rendering Fourth Edition
		// PBR Book / 8.4.3 Masking and Shadowing: "A more accurate model can be derived assuming that microfacet visibility is more likely the higher up a given point on a microface"
		// G2 = 1.0/(1.0 + ��(V) + ��(L)) = (2.0*NoV*NoL)/(NoL*sqrt(alpha^2 + (1.0 - alpha^2)*NoV^2) + NoV*sqrt(alpha^2 + (1.0 - alpha^2)*NoL^2))

		// V = G2/(4.0*NoV*NoL) = 0.5/(NoL*sqrt(alpha^2 + (1.0 - alpha^2)*NoV^2) + NoV*sqrt(alpha^2 + (1.0 - alpha^2)*NoL^2))

		// float alpha2 = alpha * alpha;
		// float term_v = NdotL * sqrt(alpha2 + (1.0 - alpha2) * NdotV * NdotV);
		// float term_l = NdotV * sqrt(alpha2 + (1.0 - alpha2) * NdotL * NdotL);
		// UE: [Vis_SmithJointApprox](https://github.com/EpicGames/UnrealEngine/blob/4.27/Engine/Shaders/Private/BRDF.ush#L380)
		float term_v = NdotL * (alpha + (1.0 - alpha) * NdotV);
		float term_l = NdotV * (alpha + (1.0 - alpha) * NdotL);
		float V = (0.5 / (term_v + term_l));

		// glTF Sample Renderer: [F_Schlick](https://github.com/KhronosGroup/glTF-Sample-Renderer/blob/e5646a2bf87b0871ba3f826fc2335fe117a11411/source/Renderer/shaders/brdf.glsl#L24)
		float3 f0 = specular;
		const float3 f90 = float3(1.0, 1.0, 1.0);

		float x = saturate(1.0 - VdotH);
		float x2 = x * x;
		float x5 = x * x2 * x2;
		float3 F = f0 + (f90 - f0) * x5;

		brdf_specular = D * V * F;
	}

	float3 lighting = (brdf_diffuse + brdf_specular) * (NdotL * light_intensity);

	out_lighting = float4(lighting, 1.0);
}