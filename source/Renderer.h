#pragma once

#include <cstdint>
#include <vector>

struct SDL_Window;
struct SDL_Surface;


namespace dae
{
	class Scene;
	struct Camera;
	struct Light;
	class Material;

	class Renderer final
	{
	public:
		Renderer(SDL_Window* pWindow);
		~Renderer() = default;

		Renderer(const Renderer&) = delete;
		Renderer(Renderer&&) noexcept = delete;
		Renderer& operator=(const Renderer&) = delete;
		Renderer& operator=(Renderer&&) noexcept = delete;

		void Update() { ++m_Counter; }

		void Render(Scene* pScene) const;

		void RenderPixel(Scene* pScene, uint32_t pixelIndex, const Camera& camera, const std::vector<Light>& lights, const std::vector<Material*>& materials) const;

		bool SaveBufferToImage() const;


		void CycleLightingMode();
		void ToggleShadows() { m_ShadowsEnabled = !m_ShadowsEnabled; }
	private:

		enum class LightingMode
		{
			ObservedArea = 0, // Lambert Cosine Law
			Radiance = 1, // Incident Radiance
			BRDF = 2, // Scattering of the light
			Combined = 3 // ObservedArea * Radiance * BRDF -> default
		};

		LightingMode m_CurrentLightingMode{ LightingMode::Combined };
		bool m_ShadowsEnabled{ true };

		SDL_Window* m_pWindow{};

		SDL_Surface* m_pBuffer{};
		uint32_t* m_pBufferPixels{};

		int m_Width{};
		int m_Height{};

		float m_AspectRatio{};

		unsigned int m_Counter{};
	};
}
