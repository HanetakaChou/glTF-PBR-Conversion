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

Texture2D g_diffuse : register(t0);
Texture2D g_specular : register(t1);
Texture2D g_glossiness : register(t2);
SamplerState g_sampler : register(s0);

void main(
	in float4 in_position : SV_POSITION,
	in float2 in_uv : TEXCOORD0,
	out float4 out_base_color : SV_TARGET0,
	out float4 out_metallic_roughness : SV_TARGET1)
{
	float3 diffuse = g_diffuse.Sample(g_sampler, in_uv).rgb;
	float3 specular = g_specular.Sample(g_sampler, in_uv).rgb;
	float glossiness = g_glossiness.Sample(g_sampler, in_uv).r;

	// [glTF PBR Conversion](https://github.com/KhronosGroup/glTF/blob/main/extensions/2.0/Archived/KHR_materials_pbrSpecularGlossiness/examples/convert-between-workflows/js/three.pbrUtilities.js#L33)
	// [glTF PBR Conversion](https://github.com/KhronosGroup/glTF/blob/main/extensions/2.0/Archived/KHR_materials_pbrSpecularGlossiness/examples/convert-between-workflows-bjs/js/babylon.pbrUtilities.js#L33)	
	
	float one_minus_specular_strength = 1.0 -  max(max(specular.r, specular.g), specular.b);

	float diffuse_perceived_brightness = sqrt(0.299 * diffuse.r * diffuse.r + 0.587 * diffuse.g * diffuse.g + 0.114 * diffuse.b * diffuse.b);
	float specular_perceived_brightness = sqrt(0.299 * specular.r * specular.r + 0.587 * specular.g * specular.g + 0.114 * specular.b * specular.b);

	const float dielectric_specular = 0.04;
	const float epsilon = 1e-6;

	float metallic;
	[branch]
	if(specular_perceived_brightness < dielectric_specular)
	{
		metallic = 0.0;
	}
	else
	{
		// diffuse_perceived_brightness = base_color * ((1.0 - dielectric_specular) * (1.0 - metallic) / one_minus_specular_strength)
		// specular_perceived_brightness = base_color * metallic + dielectric_specular * (1.0 - metallic)
		//
		// diffuse_perceived_brightness * one_minus_specular_strength / (1.0 - dielectric_specular) / (1.0 - metallic) = (specular_perceived_brightness - dielectric_specular * (1.0 - metallic)) / metallic
		//
		// diffuse_perceived_brightness * one_minus_specular_strength / (1.0 - dielectric_specular) * metallic = (specular_perceived_brightness - dielectric_specular * (1.0 - metallic)) * (1.0 - metallic)
		//
		float a = dielectric_specular;
		float b = diffuse_perceived_brightness * one_minus_specular_strength / (1.0 - dielectric_specular) + specular_perceived_brightness - 2.0 * dielectric_specular;
        float c = dielectric_specular - specular_perceived_brightness;
        float D = max(b * b - 4.0 * a * c, 0.0);
		metallic = clamp((-b + sqrt(D)) / (2 * a), 0.0, 1.0);
	}

	float3 base_color;
	{
		float3 base_color_from_diffuse = diffuse * (one_minus_specular_strength / (1.0 - dielectric_specular) / max(1.0 - metallic, epsilon));
		float3 base_color_from_specular = (specular - float3(dielectric_specular, dielectric_specular, dielectric_specular) * (1.0 - metallic)) * (1.0 / max(metallic, epsilon));
		base_color = saturate(lerp(base_color_from_diffuse, base_color_from_specular, metallic * metallic));
	}

	float roughness = 1.0 - glossiness;

	// https://github.com/KhronosGroup/glTF/blob/main/specification/2.0/schema/material.pbrMetallicRoughness.schema.json
	out_base_color = float4(base_color, 1.0);
	out_metallic_roughness = float4(0.0, roughness, metallic, 1.0);
}