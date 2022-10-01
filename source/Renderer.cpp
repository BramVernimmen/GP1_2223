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

using namespace dae;

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
	const Matrix cameraToWorld = camera.CalculateCameraToWorld();
	auto& materials = pScene->GetMaterials();
	auto& lights = pScene->GetLights();

	const float aspectRatio{ m_Width / static_cast<float>(m_Height) };

	for (int px{}; px < m_Width; ++px)
	{
		for (int py{}; py < m_Height; ++py)
		{
			float cx{ ((2.f * (px + 0.5f)) / m_Width - 1) * aspectRatio * camera.fovScaleFactor};
			float cy{ (1.f - ((2.f * (py + 0.5f)) / m_Height)) * camera.fovScaleFactor };
			
			//Vector3 rayDirection{ (cx * camera.right) + (cy * camera.up) + camera.forward };
			Vector3 rayDirection{ cx, cy , 1 };
			rayDirection = cameraToWorld.TransformVector(rayDirection);
			rayDirection.Normalize();
			
			Ray viewRay{ camera.origin, rayDirection };
			
			ColorRGB finalColor{};

			HitRecord closestHit{};
			pScene->GetClosestHit(viewRay, closestHit);


			if (closestHit.didHit)
			{
				finalColor = materials[closestHit.materialIndex]->Shade();
				closestHit.origin += closestHit.normal * 0.0001f; // Use small offset for the ray origin (self-shadowing)
				for (size_t i{0}; i < lights.size(); ++i)
				{
					Vector3 lightDirection{ LightUtils::GetDirectionToLight(lights[i], closestHit.origin)};
				
					Ray invLightRay{}; // W2 slide 25
					invLightRay.origin = closestHit.origin;
					invLightRay.direction = lightDirection;
					invLightRay.direction.Normalize();
					invLightRay.max = lightDirection.Magnitude();
				
					if (pScene->DoesHit(invLightRay))
					{
						finalColor *= 0.5f;
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
	}

	//@END
	//Update SDL Surface
	SDL_UpdateWindowSurface(m_pWindow);
}

bool Renderer::SaveBufferToImage() const
{
	return SDL_SaveBMP(m_pBuffer, "RayTracing_Buffer.bmp");
}
