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
			// ----------- OLD CODE -------------------------------------------------
			// calculating this prior, reused a lot
			//const Vector3 raySphereOriginVector{ ray.origin - sphere.origin };

			//const float a{ Vector3::Dot(ray.direction, ray.direction) };
			//const float b{ Vector3::Dot((2.f * ray.direction), raySphereOriginVector) };
			//const float c{ Vector3::Dot(raySphereOriginVector, raySphereOriginVector) - (sphere.radius * sphere.radius) };

			//float discriminant{ (b * b) - 4.f * a * c };

			//if (discriminant < 0)
			//{
			//	return false;
			//}

			//// take square root of discriminant
			//discriminant = sqrt(discriminant);


			//// calculating distance
			//float t{ (- b - discriminant) / (2.f * a)};

			//if (t <= ray.min || t > ray.max) // check if distance is outside of range
			//{
			//	// first result is out of range if we get here
			//	t = ((- b + discriminant) / (2.f * a));
			//	if (t <= ray.min || t > ray.max)
			//	{
			//		return false; // both are out of range; nothing is visible
			//	}
			//}


			//// explanation:
			//// if we take a look at { -b - discriminant / (2.f * a) } and { -b + discriminant / (2.f * a) }
			//// the square root of a value will always be positive
			//// if we subtract this value, the result will always be smaller than the addition
			//// meaning that if we first check the subtraction and it is in range, it will be the smallest possible



			//// code underneath just confuses me, currently keeping it out
			////// if need to be ignored, just return
			////if (ignoreHitRecord)
			////{
			////	return true; // false removes shading
			////}


			//// calculate the hitRecord info
			//hitRecord.t = t;
			//hitRecord.origin = ray.origin + (ray.direction * hitRecord.t);
			//hitRecord.normal = (hitRecord.origin - sphere.origin).Normalized();
			//hitRecord.materialIndex = sphere.materialIndex;
			//hitRecord.didHit = true;

			//return true;

			//  ----------- NEW CODE -------------------------------------------------
			// using the info from fxMath - week 01: Ray sphere intersection 2D
			//const float sphereRadSqr{ sphere.radius * sphere.radius }; // used multiple times, saves a little bit -> apperently doesn't work
			//
			const Vector3 tc{ sphere.origin - ray.origin }; // vector from ray origin to center of sphere
			const float dp{ Vector3::Dot(tc, ray.direction) }; // this will give us the lenght of the TP side of the triangle;
			// P is the perpendicular projection of point C on the ray
			const float odSqr{ tc.SqrMagnitude() - (dp * dp) }; // calculate the size of odSquared using pythagorean formula
			// early exit; if odSqr if outside of the sphere radius squared, the point is not in the sphere and we don't hit
			if (odSqr > (sphere.radius * sphere.radius))
				return false;

			// we now want to calculate intersecting point
			// first we need to calculate the distance between the point and P (in triangle of: intersecting point, P, C)
			const float tca{ sqrtf((sphere.radius * sphere.radius) - odSqr) };

			// now we can calculate t
			const float t{ dp - tca };

			// now we can calculate the intersecting point, but first check if t is withing range
			if (t <= ray.min || t > ray.max)
				return false;

			if (ignoreHitRecord) return true;

			const Vector3 intersectPoint{ ray.origin + t * ray.direction };
			hitRecord.t = t;
			hitRecord.origin = intersectPoint;
			hitRecord.normal = (intersectPoint - sphere.origin).Normalized();
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
			float t{ Vector3::Dot((plane.origin - ray.origin), plane.normal) / Vector3::Dot(ray.direction, plane.normal) };
			if (t > ray.min && t <= ray.max)
			{
				if (ignoreHitRecord)
					return true;

				hitRecord.t = t;
				hitRecord.origin = ray.origin + (ray.direction * t);
				hitRecord.normal = plane.normal;
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
			// check if viewray is intersecting with the triangle
			const float dotProductNormalViewray = Vector3::Dot(triangle.normal, ray.direction);
			if (dotProductNormalViewray == 0)
				return false;
			

			// switch the cullmode because of shadows
			TriangleCullMode currentCullMode = triangle.cullMode;
			if (ignoreHitRecord)
			{
				switch (currentCullMode)
				{
				case TriangleCullMode::FrontFaceCulling:
					currentCullMode = TriangleCullMode::BackFaceCulling;
					break;
				case TriangleCullMode::BackFaceCulling:
					currentCullMode = TriangleCullMode::FrontFaceCulling;
					break;
				}
			}


			// return depending on the culling mode situations
			switch (currentCullMode)
			{
			case TriangleCullMode::FrontFaceCulling:
				if (dotProductNormalViewray < 0)
					return false;
				break;
			case TriangleCullMode::BackFaceCulling:
				if (dotProductNormalViewray > 0)
					return false;
				break;
			}

			//--------------OLD CODE------------------------------------------------------------------

			//const Vector3 length = ((triangle.v0 + triangle.v1 + triangle.v2) / 3) - ray.origin; // length between triangle center and viewray
			//const float t = Vector3::Dot(length, triangle.normal) / Vector3::Dot(ray.direction, triangle.normal);


			// check if t is in range
			//if (t < ray.min || t >= ray.max)
			//	return false;

			//const Vector3 p = ray.origin + t * ray.direction;
			//
			//const Vector3 edgeA = triangle.v1 - triangle.v0;
			//const Vector3 edgeB = triangle.v2 - triangle.v1;
			//const Vector3 edgeC = triangle.v0 - triangle.v2;
			//const Vector3 pointToSideA = p - triangle.v0;
			//const Vector3 pointToSideB = p - triangle.v1;
			//const Vector3 pointToSideC = p - triangle.v2;
			//if (Vector3::Dot(triangle.normal, Vector3::Cross(edgeA, pointToSideA)) < 0)
			//	return false;
			//if (Vector3::Dot(triangle.normal, Vector3::Cross(edgeB, pointToSideB)) < 0)
			//	return false;
			//if (Vector3::Dot(triangle.normal, Vector3::Cross(edgeC, pointToSideC)) < 0)
			//	return false;




			//--------------NEW CODE------------------------------------------------------------------

			// Code was made possible thanks to: https://www.youtube.com/watch?v=fK1RPmF_zjQ - watch part from 6.30 min - 13.00 min

			const Vector3 edge1{ triangle.v1 - triangle.v0};
			const Vector3 edge2{ triangle.v2 - triangle.v0};
			const Vector3 cross_rayDir_edge2{Vector3::Cross(ray.direction, edge2)};
			
			
			const float det{Vector3::Dot(edge1, cross_rayDir_edge2)}; // scalar triple product

			if (det > -FLT_EPSILON && det < FLT_EPSILON)
				return false;

			const float inv_det{ 1.0f / det}; // calculate after the det, might return false so no need to waste time to calculate before

			const Vector3 orig_minus_vert0{ray.origin - triangle.v0};
			const float baryU{ Vector3::Dot(orig_minus_vert0, cross_rayDir_edge2) * inv_det};
			if (baryU < 0.0f || baryU > 1.0f)
				return false;

			const Vector3 cross_oriMinusVert0_edge1{Vector3::Cross(orig_minus_vert0, edge1)};
			const float baryV{ Vector3::Dot(ray.direction, cross_oriMinusVert0_edge1) * inv_det};
			if (baryV < 0.0f || baryU + baryV > 1.0f)
				return false;

			
			const float rayT{Vector3::Dot(edge2, cross_oriMinusVert0_edge1) * inv_det};

			if (rayT < ray.min || rayT >= ray.max)
				return false;

			if (ignoreHitRecord)
				return true;

			// if we get here, time to fill in the hitrecord and return true
			hitRecord.t = rayT;
			hitRecord.origin = ray.origin + (ray.direction * rayT);
			hitRecord.normal = triangle.normal;
			hitRecord.materialIndex = triangle.materialIndex;
			hitRecord.didHit = true;
			

			//hitRecord.origin = ray.origin + (ray.direction * t);
			//hitRecord.t = t;


			return true;

		}

		inline bool HitTest_Triangle(const Triangle& triangle, const Ray& ray)
		{
			HitRecord temp{};
			return HitTest_Triangle(triangle, ray, temp, true);
		}
#pragma endregion
#pragma region TriangeMesh HitTest
		// OLD SLABTEST  --> basically the same as IntersectAABB, but this one uses mesh, the other will get the min and max as parameters
		//inline bool SlabTest_TriangleMesh(const TriangleMesh& mesh, const Ray& ray)
		//{
		//	float tx1 = (mesh.transformedMinAABB.x - ray.origin.x) / ray.direction.x;
		//	float tx2 = (mesh.transformedMaxAABB.x - ray.origin.x) / ray.direction.x;
		//
		//	float tmin = std::min(tx1, tx2);
		//	float tmax = std::max(tx1, tx2);
		//
		//	float ty1 = (mesh.transformedMinAABB.y - ray.origin.y) / ray.direction.y;
		//	float ty2 = (mesh.transformedMaxAABB.y - ray.origin.y) / ray.direction.y;
		//
		//	tmin = std::max(tmin, std::min(ty1, ty2));
		//	tmax = std::min(tmin, std::max(ty1, ty2));
		//
		//	float tz1 = (mesh.transformedMinAABB.z - ray.origin.z) / ray.direction.z;
		//	float tz2 = (mesh.transformedMaxAABB.z - ray.origin.z) / ray.direction.z;
		//
		//	tmin = std::max(tmin, std::min(tz1, tz2));
		//	tmax = std::min(tmin, std::max(tz1, tz2));
		//
		//	return tmax > 0 && tmax >= tmin;
		//}
		// 
		
		inline bool IntersectAABB(const Ray& ray, const Vector3& minAABB, const Vector3& maxAABB)
		{
			// const Vector3 inversedDirection{ 1.0f / ray.direction.x, 1.0f / ray.direction.y, 1.0f / ray.direction.z }; -> more fps?

			const float tx1 = (minAABB.x - ray.origin.x) / ray.direction.x;
			const float tx2 = (maxAABB.x - ray.origin.x) / ray.direction.x;

			float tmin = std::min(tx1, tx2);
			float tmax = std::max(tx1, tx2);

			const float ty1 = (minAABB.y - ray.origin.y) / ray.direction.y;
			const float ty2 = (maxAABB.y - ray.origin.y) / ray.direction.y;

			tmin = std::max(tmin, std::min(ty1, ty2));
			tmax = std::min(tmax, std::max(ty1, ty2));

			const float tz1 = (minAABB.z - ray.origin.z) / ray.direction.z;
			const float tz2 = (maxAABB.z - ray.origin.z) / ray.direction.z;

			tmin = std::max(tmin, std::min(tz1, tz2));
			tmax = std::min(tmax, std::max(tz1, tz2));

			return tmax > 0 && tmax >= tmin;
		}

		inline void IntersectBVH(const TriangleMesh& mesh, const Ray& ray, HitRecord& hitRecord, bool& hasHit, HitRecord& curClosestHit, unsigned int bvhNodeIdx, bool ignoreHitRecord)
		{
			BVHNode& node = mesh.pBvhNodes[bvhNodeIdx];

			// Slabtest
			if (!IntersectAABB(ray, node.minAABB, node.maxAABB)) return;

			if (node.IsLeaf())
			{
				// Create temp triangle
				Triangle triangle{};
				triangle.cullMode = mesh.cullMode;
				triangle.materialIndex = mesh.materialIndex;

				// For each triangle
				for (int i{}; i < static_cast<int>(node.triCount); i+=3)
				{
					// Set the position and normal of the current triangle to the triangle object
					triangle.v0 = mesh.transformedPositions[mesh.indices[node.firstTriIdx + i]];
					triangle.v1 = mesh.transformedPositions[mesh.indices[node.firstTriIdx + i + 1]];
					triangle.v2 = mesh.transformedPositions[mesh.indices[node.firstTriIdx + i + 2]];
					triangle.normal = mesh.transformedNormals[(node.firstTriIdx + i ) / 3];

					// If the ray hits a triangle in the mesh, check if it is closer then the previous hit triangle
					if (HitTest_Triangle(triangle, ray, curClosestHit, ignoreHitRecord))
					{
						hasHit = true;

						if (ignoreHitRecord)
							return;

						// Check if the current hit is closer then the previous hit
						if (hitRecord.t > curClosestHit.t)
						{
							hitRecord = curClosestHit;
						}
					}
				}
			}
			else
			{
				IntersectBVH(mesh, ray, hitRecord, hasHit, curClosestHit, node.leftNode, ignoreHitRecord);
				IntersectBVH(mesh, ray, hitRecord, hasHit, curClosestHit, node.leftNode + 1, ignoreHitRecord);
			}
		}

		inline bool HitTest_TriangleMesh(const TriangleMesh& mesh, const Ray& ray, HitRecord& hitRecord, bool ignoreHitRecord = false)
		{
			// slabtest
			//if (!SlabTest_TriangleMesh(mesh, ray))
			//{
			//	return false;
			//}

			 
			// Create a triangle to test on
			//Triangle triangle{};
			//
			//// Give triangle the same cullMode and material
			//triangle.cullMode = mesh.cullMode;
			//triangle.materialIndex = mesh.materialIndex;

			HitRecord tempHit{};
			bool hasHit{};

			// BVH intersect
			IntersectBVH(mesh, ray, hitRecord, hasHit, tempHit, mesh.rootNodeIdx, ignoreHitRecord);

			//// loop each triangle
			//for (int i{ 0 }; i < static_cast<int>(mesh.indices.size() / 3); ++i) // for each triangle
			//{
			//	// give the correct position and normal to triangle
			//	triangle.v0 = mesh.transformedPositions[mesh.indices[i * 3]] ; // vertex 1
			//	triangle.v1 = mesh.transformedPositions[mesh.indices[i * 3 + 1]] ; // vertex 2
			//	triangle.v2 = mesh.transformedPositions[mesh.indices[i * 3 + 2]] ; // vertex 3
			//	triangle.normal = mesh.transformedNormals[i];
			//	if (GeometryUtils::HitTest_Triangle(triangle, ray, tempHit, ignoreHitRecord)) // same as in Scene.cpp -> GetClosestHit
			//	{
			//		if (tempHit.t < hitRecord.t)
			//		{
			//			hitRecord = tempHit;
			//		}
			//
			//	}
			//
			//}


			return hasHit;
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