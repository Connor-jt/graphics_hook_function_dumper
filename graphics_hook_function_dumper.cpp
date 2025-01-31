#include <iostream>
#include <string>

#include <iostream>
#include <fstream>

#include <windows.h>
#include <psapi.h>
#include <d3d11_1.h>
#pragma comment(lib, "d3d11.lib")
#include <d3dcompiler.h>
#pragma comment(lib, "d3dcompiler.lib")
#include <intrin.h>

unsigned long long calculateChecksum(const std::string& filename) {
    std::ifstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "Unable to open file for checksum'ing: " << filename << std::endl;
        return 0;}
    UINT64 checksum = 0;
    UINT64 buffer;
    while (file.read((char*)(&buffer), 8))
        checksum ^= buffer;
    return checksum;
}

typedef HRESULT(__stdcall* dynload_D3D11CreateDevice)(
    _In_opt_ IDXGIAdapter* pAdapter,
    D3D_DRIVER_TYPE DriverType,
    HMODULE Software,
    UINT Flags,
    _In_reads_opt_(FeatureLevels) CONST D3D_FEATURE_LEVEL* pFeatureLevels,
    UINT FeatureLevels,
    UINT SDKVersion,
    _COM_Outptr_opt_ ID3D11Device** ppDevice,
    _Out_opt_ D3D_FEATURE_LEVEL* pFeatureLevel,
    _COM_Outptr_opt_ ID3D11DeviceContext** ppImmediateContext);
dynload_D3D11CreateDevice __D3D11CreateDevice;

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam){
    switch (msg){
    case WM_KEYDOWN:
    case WM_KEYUP:
    case WM_SIZE:
        return 0;
    case WM_DESTROY:{
        PostQuitMessage(0);
        return 0;}
    default:
        return DefWindowProcW(hwnd, msg, wparam, lparam);
    }
}

void generate_dx11_junk(ID3D11Device1*& d3d11Device, ID3D11DeviceContext1*& d3d11DeviceContext, IDXGISwapChain1*& d3d11SwapChain) {
    // Open a window
    HWND hwnd;
    {
        WNDCLASSEXW winClass = {};
        winClass.cbSize = sizeof(WNDCLASSEXW);
        winClass.style = CS_HREDRAW | CS_VREDRAW;
        winClass.lpfnWndProc = &WndProc;
        winClass.hInstance = GetModuleHandleW(0);
        winClass.hIcon = LoadIconW(0, IDI_APPLICATION);
        winClass.hCursor = LoadCursorW(0, IDC_ARROW);
        winClass.lpszClassName = L"MyWindowClass";
        winClass.hIconSm = LoadIconW(0, IDI_APPLICATION);

        if (!RegisterClassExW(&winClass)) return;

        RECT initialRect = { 0, 0, 1024, 768 };
        AdjustWindowRectEx(&initialRect, WS_OVERLAPPEDWINDOW, FALSE, WS_EX_OVERLAPPEDWINDOW);
        LONG initialWidth = initialRect.right - initialRect.left;
        LONG initialHeight = initialRect.bottom - initialRect.top;

        hwnd = CreateWindowExW(WS_EX_OVERLAPPEDWINDOW,
            winClass.lpszClassName,
            L"sample",
            WS_OVERLAPPEDWINDOW | WS_VISIBLE,
            CW_USEDEFAULT, CW_USEDEFAULT,
            initialWidth,
            initialHeight,
            0, 0, winClass.hInstance, 0);

        if (!hwnd) return;
    }

    // Create D3D11 Device and Context
    {
        ID3D11Device* baseDevice;
        ID3D11DeviceContext* baseDeviceContext;
        D3D_FEATURE_LEVEL featureLevels[] = { D3D_FEATURE_LEVEL_11_0 };
        UINT creationFlags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;

        HRESULT hResult = __D3D11CreateDevice(0, D3D_DRIVER_TYPE_HARDWARE,
            0, creationFlags,
            featureLevels, ARRAYSIZE(featureLevels),
            D3D11_SDK_VERSION, &baseDevice,
            0, &baseDeviceContext);
        if (FAILED(hResult)) return; 
        

        // Get 1.1 interface of D3D11 Device and Context
        hResult = baseDevice->QueryInterface(__uuidof(ID3D11Device1), (void**)&d3d11Device);
        //assert(SUCCEEDED(hResult));
        baseDevice->Release();

        hResult = baseDeviceContext->QueryInterface(__uuidof(ID3D11DeviceContext1), (void**)&d3d11DeviceContext);
        //assert(SUCCEEDED(hResult));
        baseDeviceContext->Release();
    }

    // Create Swap Chain
    {
        // Get DXGI Factory (needed to create Swap Chain)
        IDXGIFactory2* dxgiFactory;
        {
            IDXGIDevice1* dxgiDevice;
            HRESULT hResult = d3d11Device->QueryInterface(__uuidof(IDXGIDevice1), (void**)&dxgiDevice);

            IDXGIAdapter* dxgiAdapter;
            hResult = dxgiDevice->GetAdapter(&dxgiAdapter);
            dxgiDevice->Release();

            DXGI_ADAPTER_DESC adapterDesc;
            dxgiAdapter->GetDesc(&adapterDesc);

            hResult = dxgiAdapter->GetParent(__uuidof(IDXGIFactory2), (void**)&dxgiFactory);
            dxgiAdapter->Release();
        }

        DXGI_SWAP_CHAIN_DESC1 d3d11SwapChainDesc = {};
        d3d11SwapChainDesc.Width = 0; // use window width
        d3d11SwapChainDesc.Height = 0; // use window height
        d3d11SwapChainDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM_SRGB;
        d3d11SwapChainDesc.SampleDesc.Count = 1;
        d3d11SwapChainDesc.SampleDesc.Quality = 0;
        d3d11SwapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        d3d11SwapChainDesc.BufferCount = 2;
        d3d11SwapChainDesc.Scaling = DXGI_SCALING_STRETCH;
        d3d11SwapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
        d3d11SwapChainDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
        d3d11SwapChainDesc.Flags = 0;

        HRESULT hResult = dxgiFactory->CreateSwapChainForHwnd(d3d11Device, hwnd, &d3d11SwapChainDesc, 0, 0, &d3d11SwapChain);

        dxgiFactory->Release();
    }

}


int main()
{
    std::cout << "Hello World!\n\n";

    // iterate all running windows processes

    // check loaded modules for either DirectX, vulkan, or whatever
    std::string target_dll = "C:\\Windows\\System32\\d3d11.dll";
    
    HMODULE hGetProcIDDLL = LoadLibraryA(target_dll.c_str());

    if (!hGetProcIDDLL) {
        std::cout << "could not load the dynamic library" << std::endl;
        return EXIT_FAILURE;}

    // resolve function address here
    __D3D11CreateDevice = (dynload_D3D11CreateDevice)GetProcAddress(hGetProcIDDLL, "D3D11CreateDevice");
    if (!__D3D11CreateDevice) {
        std::cout << "could not locate the function" << std::endl;
        return EXIT_FAILURE;}


    // get dll base address
    MODULEINFO mod_info = {}; 
    if (!GetModuleInformation(GetCurrentProcess(), hGetProcIDDLL, &mod_info, sizeof(MODULEINFO))) {
        std::cout << "could not get module information" << std::endl;
        auto v = GetLastError();
        return EXIT_FAILURE;}


    // get other dll base address
    HMODULE dxgi_module = GetModuleHandleA("dxgi.dll");
    if (!dxgi_module) {
        std::cout << "could not get dxgi module handle" << std::endl;
        return EXIT_FAILURE;}

    MODULEINFO other_mod_info = {};
    if (!GetModuleInformation(GetCurrentProcess(), dxgi_module, &other_mod_info, sizeof(MODULEINFO))) {
        std::cout << "could not get module information" << std::endl;
        return EXIT_FAILURE;}

    std::cout << std::hex;

    std::cout << "d3d11 base address: " << mod_info.lpBaseOfDll << std::endl;
    std::cout << "dxgi base address: " << other_mod_info.lpBaseOfDll << std::endl << std::endl;


    std::cout << "d3d11.dll Checksum: " << calculateChecksum(target_dll) << std::endl;

    char filename[256];
    if (!GetModuleFileNameA(dxgi_module, filename, 256)) {
        std::cout << "could not get dxgi module filepath" << std::endl;
        return EXIT_FAILURE;}
    std::cout << "dxgi.dll Checksum: " << calculateChecksum(filename) << std::endl << std::endl;

    ID3D11Device1* d3d11Device;
    ID3D11DeviceContext1* d3d11DeviceContext;
    IDXGISwapChain1* d3d11SwapChain;
    generate_dx11_junk(d3d11Device, d3d11DeviceContext, d3d11SwapChain);

    


    // device_ptr->vtable_ptr->function_ptr
    UINT64 draw_indexed_ptr = (UINT64)*((*(void***)d3d11DeviceContext) + 12);
    UINT64 set_shader_ptr   = (UINT64)*((*(void***)d3d11DeviceContext) + 11);
    UINT64 set_consts_ptr   = (UINT64)*((*(void***)d3d11DeviceContext) + 7 );
    UINT64 present_ptr      = (UINT64)*((*(void***)d3d11SwapChain    ) + 8 );
    std::cout << "DrawIndexed: "            << "d3d11.dll+" << (draw_indexed_ptr - (UINT64)mod_info.lpBaseOfDll) << " (" << draw_indexed_ptr << ")" << std::endl;
    std::cout << "VSSetShader: "            << "d3d11.dll+" << (set_shader_ptr   - (UINT64)mod_info.lpBaseOfDll) << " (" << set_shader_ptr   << ")" << std::endl;
    std::cout << "VSSetConstantBuffers: "   << "d3d11.dll+" << (set_consts_ptr   - (UINT64)mod_info.lpBaseOfDll) << " (" << set_consts_ptr   << ")" << std::endl;
    std::cout << "Present: "                << "dxgi.dll+"  << (present_ptr      - (UINT64)other_mod_info.lpBaseOfDll) << " (" << present_ptr      << ")" << std::endl;


    //UINT64 debugvar = 0x1020304050607080;
    //debugvar += draw_indexed_ptr;
    //std::cout << debugvar;

    std::string test;
    std::cin >> test;

    // turns out C++ doesn't let you get the pointer to virtual functions??? thanks bill gates
    // so we just have to do this the hard way
    __debugbreak();
    d3d11DeviceContext->DrawIndexed(0, 0, 0);                 // +60h vtable[12]
    d3d11DeviceContext->VSSetShader(nullptr, nullptr, 0);     // +58h vtable[11]
    d3d11DeviceContext->VSSetConstantBuffers(0, 1, nullptr);  // +38h vtable[7]
    d3d11SwapChain->Present(1, 0);                            // +40h vtable[8]
    return 0;
}

// Run program: Ctrl + F5 or Debug > Start Without Debugging menu
// Debug program: F5 or Debug > Start Debugging menu

// Tips for Getting Started: 
//   1. Use the Solution Explorer window to add/manage files
//   2. Use the Team Explorer window to connect to source control
//   3. Use the Output window to see build output and other messages
//   4. Use the Error List window to view errors
//   5. Go to Project > Add New Item to create new code files, or Project > Add Existing Item to add existing code files to the project
//   6. In the future, to open this project again, go to File > Open > Project and select the .sln file
