#pragma once
#include <cassert>
#include <fstream>
#include "Math.h"
#include "DataTypes.h"

namespace dae
{
	namespace GeometryUtils
	{
#pragma region Sphere HitTest
		//SPHERE HIT-TESTS
		inline bool HitTest_Sphere(const Sphere& sphere, const Ray& ray, HitRecord& hitRecord, bool ignoreHitRecord = false)
		{
			// calculating this prior, reused a lot
			const Vector3 raySphereOriginVector{ ray.origin - sphere.origin };

			const float a{ Vector3::Dot(ray.direction, ray.direction) };
			const float b{ Vector3::Dot((2.f * ray.direction), raySphereOriginVector) };
			const float c{ Vector3::Dot(raySphereOriginVector, raySphereOriginVector) - (sphere.radius * sphere.radius) };

			float discriminant{ (b * b) - 4.f * a * c };

			if (discriminant < 0)
			{
				return false;
			}

			// take square root of discriminant
			discriminant = sqrt(discriminant);


			// calculating distance
			float t{ (- b - discriminant) / (2.f * a)};

			if (t <= ray.min || t > ray.max) // check if distance is outside of range
			{
				// first result is out of range if we get here
				t = ((- b + discriminant) / (2.f * a));
				if (t <= ray.min || t > ray.max)
				{
					return false; // both are out of range; nothing is visible
				}
			}


			// explanation:
			// if we take a look at { -b - discriminant / (2.f * a) } and { -b + discriminant / (2.f * a) }
			// the square root of a value will always be positive
			// if we subtract this value, the result will always be smaller than the addition
			// meaning that if we first check the subtraction and it is in range, it will be the smallest possible



			// code underneath just confuses me, currently keeping it out
			//// if need to be ignored, just return
			//if (ignoreHitRecord)
			//{
			//	return true; // false removes shading
			//}


			// calculate the hitRecord info
			hitRecord.t = t;
			hitRecord.origin = ray.origin + (ray.direction * hitRecord.t);
			hitRecord.normal = (hitRecord.origin - sphere.origin).Normalized();
			hitRecord.materialIndex = sphere.materialIndex;
			hitRecord.didHit = true;

			return true;
		}

		inline bool HitTest_Sphere(const Sphere& sphere, const Ray& ray)
		{
			HitRecord temp{};
			return HitTest_Sphere(sphere, ray, temp, true);
		}
#pragma endregion
#pragma region Plane HitTest
		//PLANE HIT-TESTS
		inline bool HitTest_Plane(const Plane& plane, const Ray& ray, HitRecord& hitRecord, bool ignoreHitRecord = false)
		{
			// code underneath just confuses me, currently keeping it out
			//// if need to be ignored, just return
			//if (ignoreHitRecord)
			//{
			//	return false; // using true makes whole screen shaded
			//}
			float t{ Vector3::Dot((plane.origin - ray.origin), plane.normal) / Vector3::Dot(ray.direction, plane.normal) };
			if (t > ray.min && t <= ray.max)
			{
				hitRecord.normal = plane.normal;
				hitRecord.origin = ray.origin + (ray.direction * t);
				hitRecord.t = t;
				hitRecord.materialIndex = plane.materialIndex;
				hitRecord.didHit = true;
			}


			return hitRecord.didHit;
		}

		inline bool HitTest_Plane(const Plane& plane, const Ray& ray)
		{
			HitRecord temp{};
			return HitTest_Plane(plane, ray, temp, true);
		}
#pragma endregion
#pragma region Triangle HitTest
		//TRIANGLE HIT-TESTS
		inline bool HitTest_Triangle(const Triangle& triangle, const Ray& ray, HitRecord& hitRecord, bool ignoreHitRecord = false)
		{
			//todo W5
			assert(false && "No Implemented Yet!");
			return false;
		}

		inline bool HitTest_Triangle(const Triangle& triangle, const Ray& ray)
		{
			HitRecord temp{};
			return HitTest_Triangle(triangle, ray, temp, true);
		}
#pragma endregion
#pragma region TriangeMesh HitTest
		inline bool HitTest_TriangleMesh(const TriangleMesh& mesh, const Ray& ray, HitRecord& hitRecord, bool ignoreHitRecord = false)
		{
			//todo W5
			assert(false && "No Implemented Yet!");
			return false;
		}

		inline bool HitTest_TriangleMesh(const TriangleMesh& mesh, const Ray& ray)
		{
			HitRecord temp{};
			return HitTest_TriangleMesh(mesh, ray, temp, true);
		}
#pragma endregion
	}

	namespace LightUtils
	{
		//Direction from target to light
		inline Vector3 GetDirectionToLight(const Light& light, const Vector3 origin)
		{
			switch (light.type)
			{
			case LightType::Point:
				return { light.origin - origin }; // direction of the light
				break; // safety break I guess
			case LightType::Directional:
				return -(light.direction);
				break; // safety break I guess
			}

			return {};
		}

		inline ColorRGB GetRadiance(const Light& light, const Vector3& target)
		{
			switch (light.type)
			{
			case LightType::Point:
			{
				// making seperate variable to keep it less confusing
				const Vector3 lightToPoint{ GetDirectionToLight(light, target) };
				return ( light.color * ( light.intensity / lightToPoint.SqrMagnitude() ) );
			}
				break;
			case LightType::Directional:
				return (light.color * light.intensity);
				break;
			}
			return {};
		}
	}

	namespace Utils
	{
		//Just parses vertices and indices
#pragma warning(push)
#pragma warning(disable : 4505) //Warning unreferenced local function
		static bool ParseOBJ(const std::string& filename, std::vector<Vector3>& positions, std::vector<Vector3>& normals, std::vector<int>& indices)
		{
			std::ifstream file(filename);
			if (!file)
				return false;

			std::string sCommand;
			// start a while iteration ending when the end of file is reached (ios::eof)
			while (!file.eof())
			{
				//read the first word of the string, use the >> operator (istream::operator>>) 
				file >> sCommand;
				//use conditional statements to process the different commands	
				if (sCommand == "#")
				{
					// Ignore Comment
				}
				else if (sCommand == "v")
				{
					//Vertex
					float x, y, z;
					file >> x >> y >> z;
					positions.push_back({ x, y, z });
				}
				else if (sCommand == "f")
				{
					float i0, i1, i2;
					file >> i0 >> i1 >> i2;

					indices.push_back((int)i0 - 1);
					indices.push_back((int)i1 - 1);
					indices.push_back((int)i2 - 1);
				}
				//read till end of line and ignore all remaining chars
				file.ignore(1000, '\n');

				if (file.eof()) 
					break;
			}

			//Precompute normals
			for (uint64_t index = 0; index < indices.size(); index += 3)
			{
				uint32_t i0 = indices[index];
				uint32_t i1 = indices[index + 1];
				uint32_t i2 = indices[index + 2];

				Vector3 edgeV0V1 = positions[i1] - positions[i0];
				Vector3 edgeV0V2 = positions[i2] - positions[i0];
				Vector3 normal = Vector3::Cross(edgeV0V1, edgeV0V2);

				if(isnan(normal.x))
				{
					int k = 0;
				}

				normal.Normalize();
				if (isnan(normal.x))
				{
					int k = 0;
				}

				normals.push_back(normal);
			}

			return true;
		}
#pragma warning(pop)
	}
}