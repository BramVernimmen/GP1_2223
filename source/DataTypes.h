#pragma once
#include <cassert>

#include "Math.h"
#include "vector"

namespace dae
{
#pragma region GEOMETRY
	struct Sphere
	{
		Vector3 origin{};
		float radius{};

		unsigned char materialIndex{ 0 };
	};

	struct Plane
	{
		Vector3 origin{};
		Vector3 normal{};

		unsigned char materialIndex{ 0 };
	};

	enum class TriangleCullMode
	{
		FrontFaceCulling,
		BackFaceCulling,
		NoCulling
	};

	struct Triangle
	{
		Triangle() = default;
		Triangle(const Vector3& _v0, const Vector3& _v1, const Vector3& _v2, const Vector3& _normal):
			v0{_v0}, v1{_v1}, v2{_v2}, normal{_normal.Normalized()}{}

		Triangle(const Vector3& _v0, const Vector3& _v1, const Vector3& _v2) :
			v0{ _v0 }, v1{ _v1 }, v2{ _v2 }
		{
			const Vector3 edgeV0V1 = v1 - v0;
			const Vector3 edgeV0V2 = v2 - v0;
			normal = Vector3::Cross(edgeV0V1, edgeV0V2).Normalized();
		}

		Vector3 v0{};
		Vector3 v1{};
		Vector3 v2{};

		Vector3 normal{};

		TriangleCullMode cullMode{};
		unsigned char materialIndex{};
	};

	struct BVHNode
	{
		// most of the info around this comes from: https://jacco.ompf2.com/2022/04/13/how-to-build-a-bvh-part-1-basics/ 
		Vector3 minAABB{};
		Vector3 maxAABB{};
		unsigned int leftNode{};
		unsigned int firstTriIdx{};
		unsigned int triCount{};
		bool IsLeaf() { return triCount > 0; };
	};
	

	struct aabb
	{
		Vector3 bMin{ Vector3::MaxFloat };
		Vector3 bMax{ Vector3::MinFloat };
		void Grow(const Vector3& p)
		{
			bMin = Vector3::Min(bMin, p);
			bMax = Vector3::Max(bMax, p);
		}
		void Grow(const aabb& bounds)
		{
			bMin = Vector3::Min(bMin, bounds.bMin);
			bMax = Vector3::Max(bMax, bounds.bMax);
		}
		float Area()
		{
			Vector3 extent{ bMax - bMin }; // box extent
			return extent.x * extent.y + extent.y * extent.z + extent.z * extent.x;
		}

	};

	struct Bin
	{

		aabb bounds;
		int triCount{ 0 };

	};

	struct TriangleMesh
	{
		TriangleMesh() = default;
		TriangleMesh(const std::vector<Vector3>& _positions, const std::vector<int>& _indices, TriangleCullMode _cullMode):
		positions(_positions), indices(_indices), cullMode(_cullMode)
		{
			//Calculate Normals
			CalculateNormals();

			//Update Transforms
			UpdateTransforms();
		}

		TriangleMesh(const std::vector<Vector3>& _positions, const std::vector<int>& _indices, const std::vector<Vector3>& _normals, TriangleCullMode _cullMode) :
			positions(_positions), indices(_indices), normals(_normals), cullMode(_cullMode)
		{
			UpdateTransforms();
		}

		~TriangleMesh()
		{
			delete[] pBvhNodes;
		}

		std::vector<Vector3> positions{};
		std::vector<Vector3> normals{};
		std::vector<int> indices{};
		unsigned char materialIndex{};

		TriangleCullMode cullMode{TriangleCullMode::BackFaceCulling};

		Matrix rotationTransform{};
		Matrix translationTransform{};
		Matrix scaleTransform{};

		Vector3 minAABB;
		Vector3 maxAABB;

		Vector3 transformedMinAABB;
		Vector3 transformedMaxAABB;

		std::vector<Vector3> transformedPositions{};
		std::vector<Vector3> transformedNormals{};



		BVHNode* pBvhNodes{};
		unsigned int rootNodeIdx{};
		unsigned int nodesUsed{};


		void Translate(const Vector3& translation)
		{
			translationTransform = Matrix::CreateTranslation(translation);
		}

		void RotateY(float yaw)
		{
			rotationTransform = Matrix::CreateRotationY(yaw);
		}

		void Scale(const Vector3& scale)
		{
			scaleTransform = Matrix::CreateScale(scale);
		}

		void AppendTriangle(const Triangle& triangle, bool ignoreTransformUpdate = false)
		{
			int startIndex = static_cast<int>(positions.size());

			positions.push_back(triangle.v0);
			positions.push_back(triangle.v1);
			positions.push_back(triangle.v2);

			indices.push_back(startIndex);
			indices.push_back(++startIndex);
			indices.push_back(++startIndex);

			normals.push_back(triangle.normal);

			//Not ideal, but making sure all vertices are updated
			if(!ignoreTransformUpdate)
				UpdateTransforms();
		}

		void CalculateNormals()
		{
			normals.clear();
			normals.reserve(static_cast<int>(indices.size() / 3));

			// used to do i < indices.size() / 3
			// and v0{positions[indiced[i*3]]}, v1.....
			// noticed that this is a bit slower than doing it like now
			// changed this everywhere to keep consistent
			for (int i{0}; i < static_cast<int>(indices.size() ); i += 3) // for each triangle
			{
				const Vector3 v0{ positions[indices[i]] }; // vertex 1
				const Vector3 v1{ positions[indices[i + 1]] }; // vertex 2
				const Vector3 v2{ positions[indices[i + 2]] }; // vertex 3

				const Vector3 a{ v1 - v0 };
				const Vector3 b{ v2 - v0 };

				normals.emplace_back(Vector3::Cross(a, b).Normalized());
			}
		}


		void UpdateTransforms()
		{
			//Calculate Final Transform 
			const auto finalTransform = scaleTransform * rotationTransform * translationTransform; // TRS matrix

			// preps the vector 
			transformedNormals.clear();
			transformedPositions.clear();
			transformedPositions.reserve(positions.size());
			transformedNormals.reserve(normals.size());
			
			// transform both and store them at the right index
			//for (int i{0}; i < static_cast<int>(positions.size()); ++i)
			for (const auto& position: positions)
			{
				transformedPositions.emplace_back(finalTransform.TransformPoint(position));
			}
			//for (int i{0}; i < static_cast<int>(normals.size()); ++i)
			for (const auto& normal : normals)
			{
				transformedNormals.emplace_back(finalTransform.TransformVector(normal).Normalized());
			}

			//UpdateTransformedAABB(finalTransform);

			nodesUsed = 0;
			BuildBVH();
		}

		void UpdateAABB()
		{
			if (positions.size() > 0)
			{
				minAABB = positions[0];
				maxAABB = positions[0];
				for (auto& p : positions)
				{
					minAABB = Vector3::Min(p, minAABB);
					maxAABB = Vector3::Max(p, maxAABB);
				}
			}
		}

		void UpdateTransformedAABB(const Matrix& finalTransform)
		{
			// AABB update: be careful -> transform the 8 vertices of the aabb
			// and calculate new min and max.
			Vector3 tMinAABB = finalTransform.TransformPoint(minAABB);
			Vector3 tMaxAABB = tMinAABB;
			// (xmax, ymin, zmin)
			Vector3 tAABB = finalTransform.TransformPoint(maxAABB.x, minAABB.y, minAABB.z);
			tMinAABB = Vector3::Min(tAABB, tMinAABB);
			tMaxAABB = Vector3::Max(tAABB, tMaxAABB);
			// (xmax, ymin, zmax)
			tAABB = finalTransform.TransformPoint(maxAABB.x, minAABB.y, maxAABB.z);
			tMinAABB = Vector3::Min(tAABB, tMinAABB);
			tMaxAABB = Vector3::Max(tAABB, tMaxAABB);
			// (xmin, ymin, zmax)
			tAABB = finalTransform.TransformPoint(minAABB.x, minAABB.y, maxAABB.z);
			tMinAABB = Vector3::Min(tAABB, tMinAABB);
			tMaxAABB = Vector3::Max(tAABB, tMaxAABB);
			// (xmin, ymax, zmin)
			tAABB = finalTransform.TransformPoint(minAABB.x, maxAABB.y, minAABB.z);
			tMinAABB = Vector3::Min(tAABB, tMinAABB);
			tMaxAABB = Vector3::Max(tAABB, tMaxAABB);
			// (xmax, ymax, zmin)
			tAABB = finalTransform.TransformPoint(maxAABB.x, maxAABB.y, minAABB.z);
			tMinAABB = Vector3::Min(tAABB, tMinAABB);
			tMaxAABB = Vector3::Max(tAABB, tMaxAABB);
			// (xmax, ymax, zmax)
			tAABB = finalTransform.TransformPoint(maxAABB);
			tMinAABB = Vector3::Min(tAABB, tMinAABB);
			tMaxAABB = Vector3::Max(tAABB, tMaxAABB);
			// (xmin, ymax, zmax)
			tAABB = finalTransform.TransformPoint(minAABB.x, maxAABB.y, maxAABB.z);
			tMinAABB = Vector3::Min(tAABB, tMinAABB);
			tMaxAABB = Vector3::Max(tAABB, tMaxAABB);

			transformedMinAABB = tMinAABB;
			transformedMaxAABB = tMaxAABB;

		}

		


		void UpdateNodeBounds(unsigned int nodeIdx)
		{
			BVHNode& node = pBvhNodes[nodeIdx];

			//node.minAABB = Vector3(FLT_MAX, FLT_MAX, FLT_MAX);
			node.minAABB = Vector3::MaxFloat;
			//node.maxAABB = Vector3(FLT_MIN, FLT_MIN, FLT_MIN);
			node.maxAABB = Vector3::MinFloat;

			// for each vertex, take min/max
			for (unsigned int i{ 0 }; i < node.triCount; ++i) 
			{
				node.minAABB = Vector3::Min(node.minAABB, transformedPositions[indices[node.firstTriIdx + i]]);
				node.maxAABB = Vector3::Max(node.maxAABB, transformedPositions[indices[node.firstTriIdx + i]]);
			}
		}

		void BuildBVH()
		{
			BVHNode& root = pBvhNodes[rootNodeIdx];
			root.leftNode = 0;
			root.firstTriIdx = 0;
			root.triCount = static_cast<unsigned int>(indices.size());

			UpdateNodeBounds(rootNodeIdx);

			Subdivide(rootNodeIdx);

		}


		void Subdivide(unsigned int nodeIdx)
		{
			// terminate recursion
			BVHNode& node = pBvhNodes[nodeIdx];
			if (node.triCount <= 8) return; // test number
			
			// the tutorial I followed had 3 different parts to this solution, with each part giving more performance
			// uncomment 1 part while keeping the rest commented should let you work with the old stuff

			//// determine split axis and position -> part 1
			/*const Vector3 extent{ node.maxAABB - node.minAABB };
			int axis{ 0 };
			if (extent.y > extent.x) axis = 1;
			if (extent.z > extent[axis]) axis = 2;
			float splitPos = node.minAABB[axis] + extent[axis] * 0.5f;*/



			//// determine split axis using SAH -> part 2 (part 2 has lower performance than part 1)
			//int bestAxis{ -1 };
			//float bestPos{ 0 };
			/*float bestCost{ FLT_MAX };
			for (int axis{0}; axis < 3; ++axis)
			{
				for (unsigned int i{0}; i < node.triCount / 3; ++i)
				{
					const Vector3 centroid{ (transformedPositions[indices[i * 3]] + transformedPositions[indices[i * 3 + 1]] + transformedPositions[indices[i * 3 + 2]]) * 0.3333f };

					const float candidatePos{ centroid[axis] };
					const float cost{ EvaluateSAH(node, axis, candidatePos) };
					if (cost < bestCost)
					{
						bestPos = candidatePos;
						bestAxis = axis;
						bestCost = cost;
					}
				}
			}*/
			//const int axis{ bestAxis };
			//const float splitPos{ bestPos };
			
			// part 3
			int axis{ };
			float splitPos{ };
			float splitCost{ FindBestSplitPlane(node, axis, splitPos) };
			const float noSplitCost{ CalculateNodeCost(node) };
			if (splitCost >= noSplitCost) return;



			// end parts



			// in-place partition
			int i{ static_cast<int>(node.firstTriIdx)};
			int j{ i + static_cast<int>(node.triCount) - 1 };
			while (i <= j)
			{
				//calculate centroid of current triangle
				const Vector3 centroid{ (transformedPositions[indices[i]] + transformedPositions[indices[i + 1]] + transformedPositions[indices[i + 2]]) * 0.3333f };
				if (centroid[axis] < splitPos)
				{
					i += 3;
				}
				else
				{
					std::swap(indices[i], indices[j - 2]);
					std::swap(indices[i + 1], indices[j - 1]);
					std::swap(indices[i + 2], indices[j]);

					std::swap(normals[i / 3], normals[(j - 2) / 3]);
					std::swap(transformedNormals[i / 3], transformedNormals[(j - 2) / 3]);

					j -= 3;
				}
			}
			// abort split of one of the sides is empty
			int leftCount{ i - static_cast<int>(node.firstTriIdx) };
			if (leftCount == 0 || leftCount == node.triCount) return;
			// create child nodes
			int leftChildIdx{ static_cast<int>(++nodesUsed)};
			int rightChildIdx{ static_cast<int>(++nodesUsed)};
			node.leftNode = leftChildIdx;
			pBvhNodes[leftChildIdx].firstTriIdx = node.firstTriIdx;
			pBvhNodes[leftChildIdx].triCount = leftCount;
			pBvhNodes[rightChildIdx].firstTriIdx = i;
			pBvhNodes[rightChildIdx].triCount = node.triCount - leftCount;
			node.triCount = 0;
			UpdateNodeBounds(leftChildIdx);
			UpdateNodeBounds(rightChildIdx);
			// recurse
			Subdivide(leftChildIdx);
			Subdivide(rightChildIdx);

		}


		float EvaluateSAH(const BVHNode& node, int axis, float pos)
		{
			// determine triangle counts and bounds for this split candidate
			aabb leftBox{};
			aabb rightBox{};
			int leftCount{ 0 };
			int rightCount{ 0 };

			for (unsigned int i{ 0 }; i < node.triCount; i += 3)
			{
				const Vector3 v0{ transformedPositions[indices[i]] };
				const Vector3 v1{ transformedPositions[indices[i + 1]] };
				const Vector3 v2{ transformedPositions[indices[i + 2]] };
				const Vector3 centroid{ (v0 + v1 + v2) * 0.3333f };

				if (centroid[axis] < pos)
				{
					leftCount++;
					leftBox.Grow(v0);
					leftBox.Grow(v1);
					leftBox.Grow(v2);
				}
				else
				{
					rightCount++;
					rightBox.Grow(v0);
					rightBox.Grow(v1);
					rightBox.Grow(v2);
				}
			}

			const float cost{ leftCount * leftBox.Area() + rightCount * rightBox.Area() };
			return cost > 0 ? cost : FLT_MAX;
		}



		float FindBestSplitPlane(const BVHNode& node, int& axis, float& splitPos)
		{
			float bestCost{ FLT_MAX };
			for (int currAxis{ 0 }; currAxis < 3; ++currAxis)
			{
				float boundsMin{FLT_MAX};
				float boundsMax{FLT_MIN};
				for (unsigned int i{0}; i < node.triCount; i += 3)
				{
					const Vector3 centroid{ (transformedPositions[indices[node.firstTriIdx + i]] + transformedPositions[indices[node.firstTriIdx + i + 1]] + transformedPositions[indices[node.firstTriIdx + i + 2]]) / 3.0f };
					boundsMin = std::min(boundsMin, centroid[currAxis]);
					boundsMax = std::max(boundsMax, centroid[currAxis]);
				}
				

				if (boundsMin == boundsMax) continue;
				// populate the bins
				const int nrOfBins{ 8 };
				Bin bin[nrOfBins];
				float scale{ nrOfBins / (boundsMax - boundsMin)  };



				for (unsigned int i{0}; i < node.triCount; i += 3)
				{
					const Vector3& v0{ transformedPositions[indices[node.firstTriIdx + i]] };
					const Vector3& v1{ transformedPositions[indices[node.firstTriIdx + i + 1]] };
					const Vector3& v2{ transformedPositions[indices[node.firstTriIdx + i + 2]] };
					const Vector3 centroid{ (v0 + v1 + v2) / 3.0f };

					const int binIdx{ std::min(nrOfBins - 1, static_cast<int>((centroid[currAxis] - boundsMin) * scale)) };

					bin[binIdx].triCount += 3;
					bin[binIdx].bounds.Grow(v0);
					bin[binIdx].bounds.Grow(v1);
					bin[binIdx].bounds.Grow(v2);
				}

				// gather data for the 7 planes between the 8 bins
				float leftArea[nrOfBins - 1]{};
				float rightArea[nrOfBins - 1]{};
				int leftCount[nrOfBins - 1]{};
				int rightCount[nrOfBins - 1]{};


				aabb leftBox;
				aabb rightBox;
				int leftSum{ 0 };
				int rightSum{ 0 };
				for (int i{0}; i < nrOfBins - 1; ++i)
				{
					leftSum += bin[i].triCount;
					leftCount[i] = leftSum;
					leftBox.Grow(bin[i].bounds);
					leftArea[i] = leftBox.Area();


					rightSum += bin[nrOfBins - 1 - i].triCount;
					rightCount[nrOfBins - 2 - i] = rightSum;
					rightBox.Grow(bin[nrOfBins - 1 - i].bounds);
					rightArea[nrOfBins - 2 - i] = rightBox.Area();
				}

				scale = (boundsMax - boundsMin) / nrOfBins;
				for (int i{0}; i < nrOfBins - 1; ++i)
				{
					const float planeCost{ leftCount[i] * leftArea[i] + rightCount[i] * rightArea[i] };
					if (planeCost < bestCost)
					{
						axis = currAxis;
						splitPos = boundsMin + scale * (i + 1);
						bestCost = planeCost;
					}
				}
			}

			return bestCost;
		}


		float CalculateNodeCost(const BVHNode& node)
		{
			const Vector3 extent{ node.maxAABB - node.minAABB };
			const float surfaceArea{ extent.x * extent.y + extent.y * extent.z + extent.z * extent.x };
			return static_cast<float>(node.triCount) * surfaceArea;
		}

	};
#pragma endregion
#pragma region LIGHT
	enum class LightType
	{
		Point,
		Directional
	};

	struct Light
	{
		Vector3 origin{};
		Vector3 direction{};
		ColorRGB color{};
		float intensity{};

		LightType type{};
	};
#pragma endregion
#pragma region MISC
	struct Ray
	{
		Vector3 origin{};
		Vector3 direction{};
		Vector3 inverseDirection{};

		float min{ 0.0001f };
		float max{ FLT_MAX };
	};

	struct HitRecord
	{
		Vector3 origin{};
		Vector3 normal{};
		float t = FLT_MAX;

		bool didHit{ false };
		unsigned char materialIndex{ 0 };
	};
#pragma endregion
}