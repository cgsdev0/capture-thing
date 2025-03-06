//*********************************************************
//
// Copyright (c) Microsoft. All rights reserved.
// This code is licensed under the MIT License (MIT).
// THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, 
// INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. 
// IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, 
// DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, 
// TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH 
// THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
//
//*********************************************************

#include "pch.h"
#include "App.h"
#include "SimpleCapture.h"
#include <ShObjIdl.h>
#include "Win32WindowEnumeration.h"

using namespace winrt;
using namespace Windows::UI;
using namespace Windows::UI::Composition;
using namespace Windows::UI::Composition::Desktop;

// Direct3D11CaptureFramePool requires a DispatcherQueue
auto CreateDispatcherQueueController()
{
    namespace abi = ABI::Windows::System;

    DispatcherQueueOptions options
    {
        sizeof(DispatcherQueueOptions),
        DQTYPE_THREAD_CURRENT,
        DQTAT_COM_STA
    };

    Windows::System::DispatcherQueueController controller{ nullptr };
    check_hresult(CreateDispatcherQueueController(options, reinterpret_cast<abi::IDispatcherQueueController**>(put_abi(controller))));
    return controller;
}

int CALLBACK WinMain(
    HINSTANCE instance,
    HINSTANCE previousInstance,
    LPSTR     cmdLine,
    int       cmdShow);

void attachConsole()
{
    AllocConsole();
    HWND consoleWindow = GetConsoleWindow();
    //if (consoleWindow)
    //{
    //    ShowWindow(consoleWindow, SW_HIDE);  // Hide console window
    //}
    FILE* file = nullptr;
    //freopen_s(&file, "CONOUT$", "w", stdout);
    //freopen_s(&file, "CONOUT$", "w", stderr);
    //std::cerr << "hello world" << std::endl;
    //std::cout << "Console attached!\n";
}

// Get monitor by index
HMONITOR GetMonitorByIndex(int monitorIndex)
{
    int index = 0;
    EnumDisplayMonitors(nullptr, nullptr, [](HMONITOR hMonitor, HDC, LPRECT, LPARAM lParam) -> BOOL
        {
            int& index = *reinterpret_cast<int*>(lParam);
    if (index-- == 0)
    {
        g_targetMonitor = hMonitor;
        return FALSE;
    }
    return TRUE; }, reinterpret_cast<LPARAM>(&monitorIndex));

    return g_targetMonitor;
}

int CALLBACK WinMain(
    HINSTANCE instance,
    HINSTANCE previousInstance,
    LPSTR     cmdLine,
    int       cmdShow)
{
	// Init COM
	init_apartment(apartment_type::single_threaded);

    attachConsole();

    int monitorIndex = 2;
    g_targetMonitor = GetMonitorByIndex(monitorIndex);

    if (!g_targetMonitor)
    {
        std::cerr << "Failed to find the specified monitor!" << std::endl;
        return 1;
    }

    App app;

    
    // Create a DispatcherQueue for our thread
    auto controller = CreateDispatcherQueueController();

    // Enqueue our capture work on the dispatcher
    auto queue = controller.DispatcherQueue();
    auto success = queue.TryEnqueue([&]() -> void
        {
            app.Start();
        });
    WINRT_VERIFY(success);

    // Message pump
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0))
    {
        HANDLE hStdIn = GetStdHandle(STD_INPUT_HANDLE);
        if (hStdIn == INVALID_HANDLE_VALUE) {
            break;
        }
        // Prepare to check for available data
        DWORD dwAvailable;
        CHAR buffer[1];

        // Check for data in the pipe without blocking
        BOOL result = PeekNamedPipe(hStdIn, buffer, sizeof(buffer), NULL, &dwAvailable, NULL);
        if (result && dwAvailable > 0) {
            break;
        }
        TranslateMessage(&msg);
        DispatchMessage(&msg);
        auto success = queue.TryEnqueue([&]() -> void
        {
            app.Process(EnumerateWindows());
        });
        WINRT_VERIFY(success);
    }

    return (int)msg.wParam;
}