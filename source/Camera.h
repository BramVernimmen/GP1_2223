#pragma once
#include <cassert>
#include <SDL_keyboard.h>
#include <SDL_mouse.h>

#include "Math.h"
#include "Timer.h"

#include <iostream>
namespace dae
{
	struct Camera
	{
		Camera() = default;

		Camera(const Vector3& _origin, float _fovAngle):
			origin{_origin},
			fovAngle{_fovAngle}
		{
			UpdateFOV();
		}


		Vector3 origin{};
		float fovAngle{90.f};
		float fov{ 0.f }; 

		Vector3 forward{Vector3::UnitZ};
		//Vector3 forward{0.266f, -0.453f, 0.860f};
		Vector3 up{Vector3::UnitY};
		Vector3 right{Vector3::UnitX};

		float totalPitch{0.f};
		float totalYaw{0.f};


		Matrix cameraToWorld{};


		Matrix CalculateCameraToWorld()
		{
			right = Vector3::Cross(Vector3::UnitY, forward).Normalized();
			up = Vector3::Cross(forward, right).Normalized();

			cameraToWorld = { {right},{up},{forward},{origin}};
			return cameraToWorld;
		}

		void Update(Timer* pTimer)
		{
			const float deltaTime = pTimer->GetElapsed();
			float movementSpeed{ 25.f };
			float rotationSpeed{ 10.f * TO_RADIANS };
			//Keyboard Input
			const uint8_t* pKeyboardState = SDL_GetKeyboardState(nullptr);


			//Mouse Input
			int mouseX{}, mouseY{};
			const uint32_t mouseState = SDL_GetRelativeMouseState(&mouseX, &mouseY);



			const float speedMultiplier{ 4.0f };

			// Speeding up all movement
			movementSpeed += movementSpeed * speedMultiplier * (pKeyboardState[SDL_SCANCODE_LSHIFT] || pKeyboardState[SDL_SCANCODE_RSHIFT]);
			rotationSpeed += rotationSpeed * speedMultiplier * (pKeyboardState[SDL_SCANCODE_LSHIFT] || pKeyboardState[SDL_SCANCODE_RSHIFT]);

			// In/decreasing FOV -> might not really be optimal, yet nesting if statements might be the easiest
			// currently disabled since not needed for assignment

			/*if (pKeyboardState[SDL_SCANCODE_O] || pKeyboardState[SDL_SCANCODE_P])
			{
				if (pKeyboardState[SDL_SCANCODE_O])
				{
					if (fovAngle > 0.5f)
					{
						fovAngle -= movementSpeed * deltaTime;
						UpdateFOV();
					}
				}
				else
				{
					if (fovAngle < 179.5f)
					{
						fovAngle += movementSpeed * deltaTime;
						UpdateFOV();
					}
				}
			}*/
			

			// Moving origin with "WASD"
			origin += forward * (movementSpeed * deltaTime) * (pKeyboardState[SDL_SCANCODE_W] || pKeyboardState[SDL_SCANCODE_UP]);
			origin -= forward * (movementSpeed * deltaTime) * (pKeyboardState[SDL_SCANCODE_S] || pKeyboardState[SDL_SCANCODE_DOWN]);
			origin += right * (movementSpeed * deltaTime) * (pKeyboardState[SDL_SCANCODE_D] || pKeyboardState[SDL_SCANCODE_RIGHT]);
			origin -= right * (movementSpeed * deltaTime) * (pKeyboardState[SDL_SCANCODE_A] || pKeyboardState[SDL_SCANCODE_LEFT]);
			


			switch (mouseState)
			{
			case SDL_BUTTON_LMASK:
				origin += forward * -(static_cast<float>(mouseY) * movementSpeed * deltaTime);
				totalYaw += static_cast<float>(mouseX) * rotationSpeed * deltaTime;
				break;
			case SDL_BUTTON_RMASK:
				totalYaw += static_cast<float>(mouseX) * rotationSpeed * deltaTime;
				totalPitch += -(static_cast<float>(mouseY)) * rotationSpeed * deltaTime;
				break;
			case SDL_BUTTON_LMASK | SDL_BUTTON_RMASK:
				origin += up * -((movementSpeed / 2.f) * static_cast<float>(mouseY) * deltaTime);
				break;
			}

			const Matrix finalRotation{Matrix::CreateRotationX(totalPitch) * Matrix::CreateRotationY(totalYaw)};

			forward = finalRotation.TransformVector(Vector3::UnitZ);
			forward.Normalize();
			
		}

		void UpdateFOV()
		{
			fov = tanf(fovAngle * TO_RADIANS / 2);
		}
	};
}
