//External includes
#include "SDL.h"
#include "SDL_surface.h"


//Project includes
#include "Renderer.h"
#include "Math.h"
#include "Matrix.h"
#include "Material.h"
#include "Scene.h"
#include "Utils.h"



#include <thread>
#include <future> // Async Stuff
#include <ppl.h> //Parallel Stuff

using namespace dae;

//#define ASYNC
#define PARALLEL_FOR

Renderer::Renderer(SDL_Window * pWindow) :
	m_pWindow(pWindow),
	m_pBuffer(SDL_GetWindowSurface(pWindow))
{
	//Initialize
	SDL_GetWindowSize(pWindow, &m_Width, &m_Height);
	m_pBufferPixels = static_cast<uint32_t*>(m_pBuffer->pixels);
}

void Renderer::Render(Scene* pScene) const
{
	Camera& camera = pScene->GetCamera();
	camera.CalculateCameraToWorld();

	const float fov{ camera.fovScaleFactor };

	auto& materials = pScene->GetMaterials();
	auto& lights = pScene->GetLights();

	const float aspectRatio{ m_Width / static_cast<float>(m_Height) };

	const uint32_t numPixels = m_Width * m_Height;

#if defined(ASYNC)

	// Async Logic
	const uint32_t numCores = std::thread::hardware_concurrency();
	std::vector<std::future<void>> async_futures{};

	const uint32_t numPixelsPerTask = numPixels / numCores;
	uint32_t numUnassignedPixels = numPixels % numCores;
	uint32_t currPixelIndex{ 0 };

	//Create Tasks
	for (uint32_t coreId{ 0 }; coreId < numCores; ++coreId)
	{
		uint32_t taskSize{ numPixelsPerTask };
		if (numUnassignedPixels > 0)
		{
			++taskSize;
			--numUnassignedPixels;
		}

		async_futures.push_back(
			std::async(std::launch::async, [=,this] 
				{
					const uint32_t pixelIndexEnd = currPixelIndex + taskSize;
					for (uint32_t pixelIndex{ currPixelIndex }; pixelIndex < pixelIndexEnd; ++pixelIndex)
					{
						RenderPixel(pScene, pixelIndex, fov, aspectRatio, camera, lights, materials);
					}
				})
		);

		currPixelIndex += taskSize;
	}

	// Wait for all tasks
	for (const std::future<void>& f : async_futures)
	{
		f.wait();
	}


#elif defined(PARALLEL_FOR)
	// Parallel For Logic
	concurrency::parallel_for(0u, numPixels,
		[=, this](int i)
		{
			RenderPixel(pScene, i, fov, aspectRatio, camera, lights, materials);
		});

#else 
	
	//Synchronous Logic
	for (uint32_t i{0}; i < numPixels; ++i)
	{
		RenderPixel(pScene, i, fov, aspectRatio, camera, lights, materials);
	}

#endif




	//@END
	//Update SDL Surface
	SDL_UpdateWindowSurface(m_pWindow);
}

void dae::Renderer::RenderPixel(Scene* pScene, uint32_t pixelIndex, float fov, float aspectRatio, const Camera& camera, const std::vector<Light>& lights, const std::vector<Material*>& materials) const
{
	const int px = pixelIndex % m_Width;
	const int py = pixelIndex / m_Width;

	float cx{ ((2.f * (px + 0.5f)) / m_Width - 1) * aspectRatio * camera.fovScaleFactor };
	float cy{ (1.f - ((2.f * (py + 0.5f)) / m_Height)) * camera.fovScaleFactor };

	//Vector3 rayDirection{ (cx * camera.right) + (cy * camera.up) + camera.forward };
	Vector3 rayDirection{ cx, cy , 1 };
	rayDirection = camera.cameraToWorld.TransformVector(rayDirection);
	rayDirection.Normalize();

	Ray viewRay{ camera.origin, rayDirection };

	ColorRGB finalColor{};

	HitRecord closestHit{};
	pScene->GetClosestHit(viewRay, closestHit);


	if (closestHit.didHit)
	{
		//finalColor = materials[closestHit.materialIndex]->Shade();
		const Vector3 originOffset{ closestHit.origin + closestHit.normal * 0.0001f }; // Use small offset for the ray origin (self-shadowing)
		for (size_t i{ 0 }; i < lights.size(); ++i)
		{
			Vector3 lightDirection{ LightUtils::GetDirectionToLight(lights[i], originOffset) };
			const float lightDistance{ lightDirection.Normalize() }; // normalizing the vector returns the distance
			float normalLightAngle{ Vector3::Dot(closestHit.normal, lightDirection) }; // angle between normal and light direction (cosine theta)


			if (m_ShadowsEnabled)
			{
				Ray invLightRay{}; // W2 slide 25
				invLightRay.origin = originOffset;
				invLightRay.direction = lightDirection;
				invLightRay.max = lightDistance;

				if (pScene->DoesHit(invLightRay))
					continue;

			}

			switch (m_CurrentLightingMode)
			{
			case dae::Renderer::LightingMode::ObservedArea:
				if (normalLightAngle < 0)
					continue;
				//normalLightAngle *= 0.5f;
				// only multiply if normalLightAngle is bigger then 0, replacement of if statement
				finalColor += ColorRGB{ normalLightAngle, normalLightAngle, normalLightAngle };
				break;
			case dae::Renderer::LightingMode::Radiance:
				finalColor += LightUtils::GetRadiance(lights[i], closestHit.origin);
				break;
			case dae::Renderer::LightingMode::BRDF:
				finalColor += materials[closestHit.materialIndex]->Shade(closestHit, lightDirection, rayDirection);
				break;
			case dae::Renderer::LightingMode::Combined:
			{
				// formula getting too long, making variables...
				if (normalLightAngle < 0)
					continue;

				const ColorRGB radiance{ LightUtils::GetRadiance(lights[i], closestHit.origin) };
				const ColorRGB brdf{ materials[closestHit.materialIndex]->Shade(closestHit, lightDirection, rayDirection) };

				finalColor += radiance * brdf * normalLightAngle;
			}
			break;
			}






		}
	}



	//Update Color in Buffer
	finalColor.MaxToOne();

	m_pBufferPixels[px + (py * m_Width)] = SDL_MapRGB(m_pBuffer->format,
		static_cast<uint8_t>(finalColor.r * 255),
		static_cast<uint8_t>(finalColor.g * 255),
		static_cast<uint8_t>(finalColor.b * 255));
}

bool Renderer::SaveBufferToImage() const
{
	return SDL_SaveBMP(m_pBuffer, "RayTracing_Buffer.bmp");
}

void dae::Renderer::CycleLightingMode()
{
	m_CurrentLightingMode = LightingMode((static_cast<int>(m_CurrentLightingMode) + 1) % 4 ); // add one to current value, if it is 4, will reset to 0
}
