# include <windows.h>
# include <stdint.h>

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

// TODO: This is a global for now.
global_variable bool Running;

global_variable BITMAPINFO BitmapInfo;
global_variable void *BitmapMemory;
global_variable int BitmapWidth;
global_variable int BitmapHeight;

// DIB = Device Independent Bitmap
internal void
Win32ResizeDIBSection(int Width, int Height)
{
    // TODO: bulletproof this
    // Maybe don't free first, free after, and then free first if that fails
    //
    if(BitmapMemory)
    {
            // MEM_RELEASE as opposed to MEM_DECOMMIT
            VirtualFree(BitmapMemory, 0, MEM_RELEASE);
    }

    BitmapWidth = Width;
    BitmapHeight = Height;
    
    BitmapInfo.bmiHeader.biSize = sizeof(BitmapInfo.bmiHeader);
    BitmapInfo.bmiHeader.biWidth = BitmapWidth;
    // Needs to be negative so we can create a top down DIB
    // i.e. the bitmap is stored in memory starting from the
    // top left pixel
    BitmapInfo.bmiHeader.biHeight = -BitmapHeight;
    BitmapInfo.bmiHeader.biPlanes = 1;
    BitmapInfo.bmiHeader.biBitCount = 32;
    BitmapInfo.bmiHeader.biCompression = BI_RGB;

    int BytesPerPixel = 4;
    int BitmapMemorySize = (Width*Height)*BytesPerPixel;
    // Allocate me 4 bytes of memory per pixel
    // make sure it's actually available (MEM_COMMIT)
    // make sure I can read and write to it
    BitmapMemory = VirtualAlloc(0, BitmapMemorySize, MEM_COMMIT, PAGE_READWRITE);

    int Pitch = Width*BytesPerPixel;
    uint8 *Row = (uint8 *)BitmapMemory;
    for(int Y = 0;
            Y < BitmapHeight;
            ++Y)
    {
            uint8 *Pixel = (uint8 *)Row;
            for(int X = 0;
                    X < BitmapWidth;
                    ++X)
            {
                    /*
                     * Pixel in memory:
                     * 0x BB GG RR XX
                     */
                    *Pixel = (uint8)X;
                    ++Pixel;

                    *Pixel = 0;
                    ++Pixel;

                    *Pixel = (uint8)Y;
                    ++Pixel;

                    *Pixel = 0;
                    ++Pixel;
            }
            Row += Pitch;
    }
}

internal void
Win32UpdateWindow(HDC DeviceContext, RECT *WindowRect, int X, int Y, int Width, int Height)
{
        int WindowWidth = WindowRect->right - WindowRect->left;
        int WindowHeight = WindowRect->bottom - WindowRect->top;
        StretchDIBits(DeviceContext,
                0, 0, BitmapWidth, BitmapHeight,
                0, 0, WindowWidth, WindowHeight,
                BitmapMemory,
                &BitmapInfo,
                DIB_RGB_COLORS, SRCCOPY);
}

LRESULT CALLBACK
Win32MainWindowCallback(HWND Window,
                        UINT Message,
                        WPARAM WParam,
                        LPARAM LParam)
{
    LRESULT Result = 0;

    switch(Message)
    {
        case WM_SIZE:
        {
            // The part of the window you can actually draw into, i.e. not the border
            RECT ClientRect;
            GetClientRect(Window, &ClientRect);
            int Width = ClientRect.right - ClientRect.left;
            int Height = ClientRect.bottom - ClientRect.top;
            Win32ResizeDIBSection(Width, Height);
        } break;

        case WM_DESTROY:
        {
            // TODO: Handle this as an error, recreate window?
            Running = false;
        } break;

        case WM_CLOSE:
        {
            // TODO: Handle this with a message to the user?
            Running = false;
        } break;

        case WM_ACTIVATEAPP:
        {
            OutputDebugStringA("WM_ACTIVATEAPP\n");
        } break;

        case WM_PAINT:
        {
            PAINTSTRUCT Paint;
            HDC DeviceContext = BeginPaint(Window, &Paint);
            int X = Paint.rcPaint.left;
            int Y = Paint.rcPaint.top;
            int Width = Paint.rcPaint.right - Paint.rcPaint.left;
            int Height = Paint.rcPaint.bottom - Paint.rcPaint.top;

            RECT ClientRect;
            GetClientRect(Window, &ClientRect);

            Win32UpdateWindow(DeviceContext, &ClientRect, X, Y, Width, Height);
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
    WNDCLASS WindowClass = {};
    
    // TODO(casey): Check if HREDRAW/VREDRAW/OWNDC still matter
    WindowClass.lpfnWndProc = Win32MainWindowCallback;
    WindowClass.hInstance = Instance;
    // WindowClass.hIcon;
    WindowClass.lpszClassName = "HandmadeHeroWindowClass";

    if(RegisterClassA(&WindowClass))
    {
        HWND WindowHandle =
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

        if(WindowHandle)
        {
            Running = true;
            while(Running)
            {
                MSG Message;
                BOOL MessageResult = GetMessageA(&Message, 0, 0, 0);
                if(MessageResult > 0)
                {
                    TranslateMessage(&Message);
                    DispatchMessageA(&Message);
                }
                else
                {
                    break;
                }
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
