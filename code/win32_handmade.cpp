# include <windows.h>
# include <stdint.h>
# include <xinput.h>

// 3 different uses of static
// 1. a local function definition
# define internal static
// 2. a variable whose storage survives its scope
# define local_persist static global_variable
// 3. a variable available everywhere
# define global_variable static

typedef int8_t int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;

typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;


struct win32_offscreen_buffer
{
    // NOTE: Pixels are always 32 bits wide, Memory Order: BB GG RR xx
    BITMAPINFO Info;
    void *Memory;
    int Width;
    int Height;
    int Pitch;
};

// TODO: This is a global for now.
global_variable bool GlobalRunning;
global_variable win32_offscreen_buffer GlobalBackbuffer;

struct win32_window_dimension {
    int Width;
    int Height;
};

// NOTE: XInputGetState
#define X_INPUT_GET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_STATE *pState)
typedef X_INPUT_GET_STATE(x_input_get_state);
X_INPUT_GET_STATE(XInputGetStateStub)
{
    return(0);
}
global_variable x_input_get_state *XInputGetState_ = XInputGetStateStub;
#define XInputGetState XInputGetState_

// NOTE: XInputSetState
#define X_INPUT_SET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_VIBRATION *pVibration)
typedef X_INPUT_SET_STATE(x_input_set_state);

X_INPUT_SET_STATE(XInputSetStateStub)
{
    return(0);
}
global_variable x_input_set_state *XInputSetState_ = XInputSetStateStub;
#define XInputSetState XInputSetState_

internal void
Win32LoadXInput(void)
{
    // Replicate the steps the Windows loader goes through when it loads our code
    // We're doing this so that we can support the case where the lib doesn't exist
    // i.e. someone who just wants to play on their keyboard
    HMODULE XInputLibrary = LoadLibraryA("xinput1_3.dll");
    if(XInputLibrary)
    {
        XInputGetState = (x_input_get_state *)GetProcAddress(XInputLibrary, "XInputGetState");
        if(!XInputGetState) {XInputGetState = XInputGetStateStub;}

        XInputSetState = (x_input_set_state *)GetProcAddress(XInputLibrary, "XInputSetState");
        if(!XInputSetState) {XInputSetState = XInputSetStateStub;}
    }
}

internal win32_window_dimension
Win32GetWindowDimension(HWND Window)
{
    win32_window_dimension Result;

    RECT ClientRect;
    GetClientRect(Window, &ClientRect);
    Result.Width = ClientRect.right - ClientRect.left;
    Result.Height = ClientRect.bottom - ClientRect.top;

    return(Result);
}

internal void
RenderWeirdGradient(win32_offscreen_buffer *Buffer, int XOffset, int YOffset)
{
    uint8 *Row = (uint8 *)Buffer->Memory;
    for(int Y = 0;
        Y < Buffer->Height;
        ++Y)
    {
        uint32 *Pixel = (uint32 *)Row;
        for(int X = 0;
            X < Buffer->Width;
            ++X)
        {
            /*
             * Memory:      BB GG RR xx
             * Register:    xx RR GG BB
             */
            uint8 Blue = (X + XOffset);
            uint8 Green = (Y + YOffset);
            *Pixel++ = ((Green << 8) | Blue);
        }
        Row += Buffer->Pitch;
    }
}

// DIB = Device Independent Bitmap
internal void
Win32ResizeDIBSection(win32_offscreen_buffer *Buffer, int Width, int Height)
{
    // TODO: bulletproof this
    // Maybe don't free first, free after, and then free first if that fails
    //
    if(Buffer->Memory)
    {
        // MEM_RELEASE as opposed to MEM_DECOMMIT
        VirtualFree(Buffer->Memory, 0, MEM_RELEASE);
    }

    Buffer->Width = Width;
    Buffer->Height = Height;
    int BytesPerPixel = 4;
    
    Buffer->Info.bmiHeader.biSize = sizeof(Buffer->Info.bmiHeader);
    Buffer->Info.bmiHeader.biWidth = Buffer->Width;
    // Needs to be negative so we can create a top down DIB
    // i.e. the bitmap is stored in memory starting from the
    // top left pixel
    Buffer->Info.bmiHeader.biHeight = -Buffer->Height;
    Buffer->Info.bmiHeader.biPlanes = 1;
    Buffer->Info.bmiHeader.biBitCount = 32;
    Buffer->Info.bmiHeader.biCompression = BI_RGB;

    int BitmapMemorySize = (Buffer->Width*Buffer->Height)*BytesPerPixel;
    // Allocate me 4 bytes of memory per pixel
    // make sure it's actually available (MEM_COMMIT)
    // make sure I can read and write to it
    Buffer->Memory = VirtualAlloc(0, BitmapMemorySize, MEM_COMMIT, PAGE_READWRITE);

    Buffer->Pitch = Width*BytesPerPixel;
    // TODO: probably clear this to black
}

internal void
Win32DisplayBufferInWindow(win32_offscreen_buffer *Buffer,
                           HDC DeviceContext, int WindowWidth, int WindowHeight)
{
    // TODO: Aspect ration correction
    StretchDIBits(DeviceContext,
        0, 0, WindowWidth, WindowHeight,
        0, 0, Buffer->Width, Buffer->Height,
        Buffer->Memory,
        &Buffer->Info,
        DIB_RGB_COLORS, SRCCOPY);
}

internal LRESULT CALLBACK
Win32MainWindowCallback(HWND Window,
                        UINT Message,
                        WPARAM WParam,
                        LPARAM LParam)
{
    LRESULT Result = 0;

    switch(Message)
    {
        case WM_DESTROY:
        {
            // TODO: Handle this as an error, recreate window?
            GlobalRunning = false;
        } break;

        case WM_CLOSE:
        {
            // TODO: Handle this with a message to the user?
            GlobalRunning = false;
        } break;

        case WM_ACTIVATEAPP:
        {
            OutputDebugStringA("WM_ACTIVATEAPP\n");
        } break;

        case WM_SYSKEYDOWN:
        case WM_SYSKEYUP:
        case WM_KEYDOWN:
        case WM_KEYUP:
        {
            uint32 VKCode = WParam;
            bool WasDown = ((LParam & (1 << 30)) != 0);
            bool IsDown = ((LParam & (1 << 31)) == 0);

            if(WasDown != IsDown)
            {
                if(VKCode == 'W')
                {
                }
                else if(VKCode == 'A')
                {
                }
                else if(VKCode == 'S')
                {
                }
                else if(VKCode == 'D')
                {
                }
                else if(VKCode == 'Q')
                {
                }
                else if(VKCode == 'E')
                {
                }
                else if(VKCode == VK_UP)
                {
                }
                else if(VKCode == VK_DOWN)
                {
                }
                else if(VKCode == VK_LEFT)
                {
                }
                else if(VKCode == VK_RIGHT)
                {
                }
                else if(VKCode == VK_ESCAPE)
                {
                }
                else if(VKCode == VK_SPACE)
                {
                }
            }
        } break;

        case WM_PAINT:
        {
            PAINTSTRUCT Paint;
            HDC DeviceContext = BeginPaint(Window, &Paint);
            win32_window_dimension Dimension = Win32GetWindowDimension(Window);
            Win32DisplayBufferInWindow(&GlobalBackbuffer, DeviceContext,
                                       Dimension.Width, Dimension.Height);
            EndPaint(Window, &Paint);
        } break;

        default:
        {
            // OutputDebugStringA("default\n");
            Result = DefWindowProc(Window, Message, WParam, LParam); 
        } break;
    }

    return(Result);
}

int CALLBACK
WinMain(HINSTANCE Instance,
        HINSTANCE PrevInstance,
        LPSTR CommandLine,
        int ShowCode)
{
    Win32LoadXInput();
    WNDCLASSA WindowClass = {};
    
    Win32ResizeDIBSection(&GlobalBackbuffer, 1280, 720);

    WindowClass.style = CS_HREDRAW|CS_VREDRAW|CS_OWNDC;
    WindowClass.lpfnWndProc = Win32MainWindowCallback;
    WindowClass.hInstance = Instance;
    // WindowClass.hIcon;
    WindowClass.lpszClassName = "HandmadeHeroWindowClass";

    if(RegisterClassA(&WindowClass))
    {
        HWND Window =
            CreateWindowExA(
                0,
                WindowClass.lpszClassName,
                "Handmade Hero",
                WS_OVERLAPPEDWINDOW|WS_VISIBLE,
                CW_USEDEFAULT,
                CW_USEDEFAULT,
                CW_USEDEFAULT,
                CW_USEDEFAULT,
                0,
                0,
                Instance,
                0);

        if(Window)
        {
            HDC DeviceContext = GetDC(Window);
            int XOffset = 0;
            int YOffset = 0;

            GlobalRunning = true;
            while(GlobalRunning)
            {
                MSG Message;
                while(PeekMessage(&Message, 0, 0, 0, PM_REMOVE))
                {
                    if(Message.message == WM_QUIT) {
                        GlobalRunning = false;
                    }
                    
                    // TODO: Should we poll this more frequently?
                    for(DWORD ControllerIndex = 0;
                            ControllerIndex < XUSER_MAX_COUNT;
                            ++ControllerIndex)
                    {
                        XINPUT_STATE ControllerState;
                        if(XInputGetState(ControllerIndex, &ControllerState) == ERROR_SUCCESS)
                        {
                            // This controller is plugged in
                            // TODO: see if ControllerState.dwPacketNumer increments too rapidly
                            XINPUT_GAMEPAD *Pad = &ControllerState.Gamepad;
                            bool Up = (Pad->wButtons & XINPUT_GAMEPAD_DPAD_UP);
                            bool Down = (Pad->wButtons & XINPUT_GAMEPAD_DPAD_DOWN);
                            bool Left = (Pad->wButtons & XINPUT_GAMEPAD_DPAD_LEFT);
                            bool Right = (Pad->wButtons & XINPUT_GAMEPAD_DPAD_RIGHT);
                            bool Start = (Pad->wButtons & XINPUT_GAMEPAD_START);
                            bool Back = (Pad->wButtons & XINPUT_GAMEPAD_BACK);
                            bool LeftShoulder = (Pad->wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER);
                            bool RightShoulder = (Pad->wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER);
                            bool AButton = (Pad->wButtons & XINPUT_GAMEPAD_A);
                            bool BButton = (Pad->wButtons & XINPUT_GAMEPAD_B);
                            bool XButton = (Pad->wButtons & XINPUT_GAMEPAD_X);
                            bool YButton = (Pad->wButtons & XINPUT_GAMEPAD_Y);

                            int16 StickX = Pad->sThumbLX;
                            int16 StickY = Pad->sThumbLY;

                            if(AButton)
                            {
                                YOffset += 2;
                            }
                        }
                        else
                        {
                            // The controller is not available
                        }
                    }

                    TranslateMessage(&Message);
                    DispatchMessageA(&Message);
                }

                RenderWeirdGradient(&GlobalBackbuffer, XOffset, YOffset);

                win32_window_dimension Dimension = Win32GetWindowDimension(Window);
                Win32DisplayBufferInWindow(&GlobalBackbuffer, DeviceContext,
                                           Dimension.Width, Dimension.Height);
            }
        }
        else
        {
            // TODO: logging
        }
    }
    else
    {
        //TODO: logging
    }

    return(0);
}
