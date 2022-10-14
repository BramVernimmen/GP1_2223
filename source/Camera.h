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
			UpdateFOVScaleFactor();
		}


		Vector3 origin{};
		float fovAngle{90.f};
		float fovScaleFactor{ 0.f }; 

		Vector3 forward{Vector3::UnitZ};
		//Vector3 forward{0.266f, -0.453f, 0.860f};
		Vector3 up{Vector3::UnitY};
		Vector3 right{Vector3::UnitX};

		float totalPitch{0.f};
		float totalYaw{0.f};


		Matrix cameraToWorld{};


		Matrix CalculateCameraToWorld()
		{
			Vector3 tempRight{ (Vector3::Cross(Vector3::UnitY, forward)) };
			tempRight.Normalize();
			Vector3 tempUp{ (Vector3::Cross(forward, tempRight)) };
			tempUp.Normalize();
			return { {tempRight},{tempUp},{forward},{origin} };
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


			// using a lot of if statements might not be optimal


			// Speeding up all movement
			if (pKeyboardState[SDL_SCANCODE_LSHIFT] || pKeyboardState[SDL_SCANCODE_RSHIFT])
			{
				movementSpeed *= 4.f;
				rotationSpeed *= 4.f;
			}

			// In/decreasing FOV
			if (pKeyboardState[SDL_SCANCODE_LEFT])
			{
				if (fovAngle > 0.5f)
				{
					fovAngle -= movementSpeed * deltaTime;
					UpdateFOVScaleFactor();
				}
			}
			else if (pKeyboardState[SDL_SCANCODE_RIGHT])
			{
				if (fovAngle < 179.5f)
				{
					fovAngle += movementSpeed * deltaTime;
					UpdateFOVScaleFactor();
				}
			}

			// Moving origin with "WASD"
			// updates with no more if statements

			origin += forward * (movementSpeed * deltaTime) * (pKeyboardState[SDL_SCANCODE_W]);
			origin -= forward * (movementSpeed * deltaTime) * (pKeyboardState[SDL_SCANCODE_S]);
			origin += right * (movementSpeed * deltaTime) * (pKeyboardState[SDL_SCANCODE_D]);
			origin -= right * (movementSpeed * deltaTime) * (pKeyboardState[SDL_SCANCODE_A]);
			
			
			if (mouseState == SDL_BUTTON_LMASK)
			{
				
				origin += forward * -(static_cast<float>(mouseY) * movementSpeed * deltaTime) ;
				totalYaw += static_cast<float>(mouseX) * rotationSpeed * deltaTime;
			}
			else if (mouseState == SDL_BUTTON_RMASK)
			{
				totalYaw += static_cast<float>(mouseX) * rotationSpeed * deltaTime;
				totalPitch += -(static_cast<float>(mouseY)) * rotationSpeed * deltaTime;

			}
			else if (mouseState == (SDL_BUTTON_LMASK | SDL_BUTTON_RMASK))
			{
				origin += up * -((movementSpeed / 2.f ) * static_cast<float>(mouseY) * deltaTime);
			}

			Matrix finalRotation = Matrix::CreateRotation(totalPitch, totalYaw, 0.f);

			forward = finalRotation.TransformVector(Vector3::UnitZ);
			forward.Normalize();
			
		}

		void UpdateFOVScaleFactor()
		{
			fovScaleFactor = tanf(fovAngle * TO_RADIANS / 2);
		}
	};
}
