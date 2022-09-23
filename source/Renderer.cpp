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
	// W1 - todo 2
	{
		//// testing dot product Vector3
		//float dotResult{};
		//dotResult = Vector3::Dot(Vector3::UnitX, Vector3::UnitX); // (1) Same Direction
		//dotResult = Vector3::Dot(Vector3::UnitX, -Vector3::UnitX); // (-1) Opposite Direction
		//dotResult = Vector3::Dot(Vector3::UnitX, Vector3::UnitY); // (0) Perpendicular
		//
		//// testing cross product Vector3
		//Vector3 crossResult{};
		//crossResult = Vector3::Cross(Vector3::UnitZ, Vector3::UnitX); // (0,1,0) UnitY
		//crossResult = Vector3::Cross(Vector3::UnitX, Vector3::UnitZ); // (0,-1,0) -UnitY
	}

	Camera& camera = pScene->GetCamera();
	auto& materials = pScene->GetMaterials();
	auto& lights = pScene->GetLights();

	const float aspectRatio{ m_Width / static_cast<float>(m_Height) };

	for (int px{}; px < m_Width; ++px)
	{
		for (int py{}; py < m_Height; ++py)
		{
			// W1 - todo 3
			{
				//float cx{ ((2.f * (px + 0.5f)) / m_Width - 1) * aspectRatio };
				//float cy{ 1.f - ((2.f * (py + 0.5f)) / m_Height) };
				//
				//Vector3 rayDirection{ (cx * camera.right) + (cy * camera.up) + camera.forward };
				//rayDirection.Normalize();
				//
				//
				//Ray hitRay{ {0,0,0}, rayDirection };
				//
				//ColorRGB finalColor{ rayDirection.x, rayDirection.y, rayDirection.z };
				//
				////Update Color in Buffer
				//finalColor.MaxToOne();
				//
				//m_pBufferPixels[px + (py * m_Width)] = SDL_MapRGB(m_pBuffer->format,
				//	static_cast<uint8_t>(finalColor.r * 255),
				//	static_cast<uint8_t>(finalColor.g * 255),
				//	static_cast<uint8_t>(finalColor.b * 255));
			}

			float cx{ ((2.f * (px + 0.5f)) / m_Width - 1) * aspectRatio };
			float cy{ 1.f - ((2.f * (py + 0.5f)) / m_Height) };
			
			Vector3 rayDirection{ (cx * camera.right) + (cy * camera.up) + camera.forward };
			rayDirection.Normalize();
			// For each pixel...
			// ... Ray Direction calculations above...
			// Ray we are casting from the camera towards each pixel
			Ray viewRay{ {0,0,0}, rayDirection };
			
			// Color to write to the color buffer (default=black)
			ColorRGB finalColor{};

			// HitRecord containing more information about a potential hit
			HitRecord closestHit{};
			pScene->GetClosestHit(viewRay, closestHit);


			// W1: todo 4
			{
				//// TEMP SPHERE (default material = solid red - id=0)
				//Sphere testSphere{ {0.f,0.f,100.f}, 50.f, 0 };
				//
				//// Perform Sphere HitTest
				//GeometryUtils::HitTest_Sphere(testSphere, viewRay, closestHit);
			}

			// W1: todo 9
			{
				//Plane testPlane{ {0.f,-50.f,0.f}, {0.f, 1.f, 0.f}, 0 };
				//GeometryUtils::HitTest_Plane(testPlane, viewRay, closestHit);
			}

			if (closestHit.didHit)
			{
				// If we hit something, set finalColor to material color, else keep black
				// Use HitRecord::materialIndex to find the corresponding material
				finalColor = materials[closestHit.materialIndex]->Shade();

				// W1: todo 5
				{
					// Verify t-values
					// Remap t-value to [0,1] (should lay between ~[50,90])
					//const float scaled_t = (closestHit.t - 50.f) / 40.f;
					//finalColor = { scaled_t, scaled_t, scaled_t };
				}

				// W1: todo 10
				{
					// T-Value visualization
					//const float scaled_t = closestHit.t / 500.f;
					//finalColor = { scaled_t, scaled_t, scaled_t };
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
