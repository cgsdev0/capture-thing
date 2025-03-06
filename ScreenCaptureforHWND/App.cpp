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
#include <xmemory>
#include <Winuser.h>

using namespace winrt;
using namespace Windows::System;
using namespace Windows::Security;
using namespace Windows::Foundation;
using namespace Windows::UI;
using namespace Windows::UI::Composition;
using namespace Windows::Graphics::Capture;

void App::Start()
{
    auto queue = DispatcherQueue::GetForCurrentThread();
    auto d3dDevice = CreateD3DDevice();
    auto dxgiDevice = d3dDevice.as<IDXGIDevice>();
    m_device = CreateDirect3DDevice(dxgiDevice.get());

    auto result = GraphicsCaptureAccess::RequestAccessAsync(GraphicsCaptureAccessKind::Borderless);
    result.Completed([](auto a, auto b) { 
        if (a.GetResults() != Authorization::AppCapabilityAccess::AppCapabilityAccessStatus::Allowed)
        {
            std::cerr << "Borderless capture permission denied." << std::endl;
        }
    });
}

void App::Process(const std::vector<Window>& w) {
    HWND active = GetForegroundWindow();
    for (auto& [hwnd, capture] : activeCaptures)
    {
        capture.dirty = true;
        capture.was_active = capture.active;
        capture.active = false;
    }
    for (const auto& window : w) {

        StartCapture(window, active == window.Hwnd());
    }
    std::erase_if(activeCaptures, [](const auto& a) { return a.second.dirty; });
}

void App::StartCapture(const Window& window, bool active)
{
    auto hwnd = window.Hwnd();
    auto it = activeCaptures.find(hwnd);
    if (it == activeCaptures.end()) {
        // start a new capture

        try {
	        auto item = CreateCaptureItemForWindow(hwnd);
            auto name = "Window_" + std::to_string(reinterpret_cast<uintptr_t>(hwnd));
            auto title = window.Title();
            activeCaptures[hwnd] = ActiveCapture{
                .active = active,
                .was_active = active,
                .capture = std::make_unique<SimpleCapture>(m_device, item, name, title)
            };
            if (active) {
                std::cout << "focus_change|Window_" << std::to_string(reinterpret_cast<uintptr_t>(hwnd)) << std::endl;
            }
        }
        catch (...) {
            std::wcerr << "Failed to capture " << window.Title() << std::endl;
            activeCaptures[hwnd] = ActiveCapture{
                .active = active,
                .was_active = active,
                .capture = nullptr
            };
        }
    }
    else {
        it->second.active = active;
        if (active && !it->second.was_active) {
            std::cout << "focus_change|Window_" << std::to_string(reinterpret_cast<uintptr_t>(hwnd)) << std::endl;
        }
        it->second.dirty = false;
    }
}