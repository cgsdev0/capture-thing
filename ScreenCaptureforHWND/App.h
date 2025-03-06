#pragma once

class SimpleCapture;


struct ActiveCapture {
    bool dirty = false;
    bool active = false;
    bool was_active = false;
    std::unique_ptr<SimpleCapture> capture;
};

class App
{
public:
    App() {};
    ~App() {}
    void StartCapture(const Window&, bool active);
    void Process(const std::vector<Window>&);
    void Start();

private:

    winrt::Windows::Graphics::DirectX::Direct3D11::IDirect3DDevice m_device{ nullptr };
    std::unordered_map<HWND, ActiveCapture> activeCaptures;
};
