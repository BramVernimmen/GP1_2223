#pragma once
#include <cassert>
#include "Math.h"

namespace dae
{
	namespace BRDF
	{
		/**
		 * \param kd Diffuse Reflection Coefficient
		 * \param cd Diffuse Color
		 * \return Lambert Diffuse Color
		 */
		static ColorRGB Lambert(float kd, const ColorRGB& cd)
		{
			return ((cd * kd) * INV_PI);
		}

		static ColorRGB Lambert(const ColorRGB& kd, const ColorRGB& cd)
		{
			return ((cd * kd) * INV_PI);
		}

		/**
		 * \brief todo
		 * \param ks Specular Reflection Coefficient
		 * \param exp Phong Exponent
		 * \param l Incoming (incident) Light Direction
		 * \param v View Direction
		 * \param n Normal of the Surface
		 * \return Phong Specular Color
		 */
		static ColorRGB Phong(float ks, float exp, const Vector3& l, const Vector3& v, const Vector3& n)
		{
			//const Vector3 reflect{ l - 2 * (Vector3::Dot(n, l)) * n };
			const Vector3 reflect{ Vector3::Reflect(l,n)};
			float angleViewReflect{ std::max(Vector3::Dot(reflect, v), 0.0f) }; // angle between the reflect and view
			// -> max out angleViewReflect to be at least 0, this will spare an if statement
			const float phongValue{ ks * powf(angleViewReflect, exp) };

			return ColorRGB{ phongValue, phongValue, phongValue };
		}

		/**
		 * \brief BRDF Fresnel Function >> Schlick >> F
		 * \param h Normalized Halfvector between View and Light directions
		 * \param v Normalized View direction
		 * \param f0 Base reflectivity of a surface based on IOR (Indices Of Refrection), this is different for Dielectrics (Non-Metal) and Conductors (Metal)
		 * \return
		 */
		static ColorRGB FresnelFunction_Schlick(const Vector3& h, const Vector3& v, const ColorRGB& f0)
		{
			return (f0 + (ColorRGB{1.f, 1.f, 1.f} - f0) * powf((1 - (std::max(Vector3::Dot(h, v), 0.0f))), 5));
		}

		/**
		 * \brief BRDF NormalDistribution >> Trowbridge-Reitz GGX (UE4 implemetation - squared(roughness)) >> D
		 * \param n Surface normal
		 * \param h Normalized half vector
		 * \param roughness Roughness of the material
		 * \return BRDF Normal Distribution Term using Trowbridge-Reitz GGX
		 */
		static float NormalDistribution_GGX(const Vector3& n, const Vector3& h, float roughness)
		{
			const float alpha{ roughness * roughness };
			const float alphaSqrd{ alpha * alpha };
			const float dot{ std::max(Vector3::Dot(n,h), 0.0f) };
			const float dotSqrd{ dot * dot };

			return alphaSqrd / (PI * Square(dotSqrd * (alphaSqrd - 1) + 1));
		}


		/**
		 * \brief BRDF Geometry Function >> Schlick GGX (Direct Lighting + UE4 implementation - squared(roughness)) >> G
		 * \param n Normal of the surface
		 * \param v Normalized view direction
		 * \param roughness Roughness of the material
		 * \return BRDF Geometry Term using SchlickGGX
		 */
		static float GeometryFunction_SchlickGGX(const Vector3& n, const Vector3& v, float roughness)
		{
			const float alpha{ roughness * roughness };
			const float kDirect{ (Square(alpha + 1.f) / 8.f) }; // kIndirect would be { (Square(alpha)/2)}
			const float dotResult{ std::max(Vector3::Dot(n,v), 0.0f) }; // result of dot, not negative

			return (dotResult / ((dotResult * (1.f - kDirect) + kDirect)) );
		}

		/**
		 * \brief BRDF Geometry Function >> Smith (Direct Lighting)
		 * \param n Normal of the surface
		 * \param v Normalized view direction
		 * \param l Normalized light direction
		 * \param roughness Roughness of the material
		 * \return BRDF Geometry Term using Smith (> SchlickGGX(n,v,roughness) * SchlickGGX(n,l,roughness))
		 */
		static float GeometryFunction_Smith(const Vector3& n, const Vector3& v, const Vector3& l, float roughness)
		{
			return (GeometryFunction_SchlickGGX(n, v, roughness) * GeometryFunction_SchlickGGX(n, l, roughness));
		}

	}
}