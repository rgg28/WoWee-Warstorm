#ifndef SDL3_BRIDGE_H
#define SDL3_BRIDGE_H

typedef struct SDL_Window SDL_Window;
typedef union SDL_Event SDL_Event;
typedef struct SDL_Gamepad SDL_Gamepad;
typedef int SDL_GamepadButton;
typedef uint32_t SDL_JoystickID;
typedef int SDL_Scancode;

#define SDL_GAMEPAD_BUTTON_COUNT 15
#define SDL_SCANCODE_COUNT 512

#endif // SDL3_BRIDGE_H
