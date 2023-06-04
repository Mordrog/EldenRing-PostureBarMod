#include "D3DRenderer.hpp"
#include "../PostureBarMod.hpp"
#include "Logger.hpp"
#include "Hooking.hpp"
#include "PostureBarUI.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include "../Stb/stb_image.h"

IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

namespace ER 
{
	HWND FindERWindow()
	{
		HWND window = FindWindowExW(NULL, NULL, L"ELDEN RING™", NULL);

		if (window == NULL)
		{
			Logger::log("Failed to find \"ELDEN RING™\" window, trying to get via GetForegroundWindow", LogLevel::Warning);
			window = GetForegroundWindow();
		}

		return window;
	}

	D3DRenderer::ProgramData::ProgramData()
	{
		Logger::log("Acquiring program data");

		m_GamePid = GetCurrentProcessId();
		m_GameHandle = GetCurrentProcess();
		m_GameWindow = FindERWindow();

		m_ModuleBase = (uintptr_t)GetModuleHandle(NULL);

		RECT TempRect;
		GetWindowRect(m_GameWindow, &TempRect);
		m_GameWidth = TempRect.right - TempRect.left;
		m_GameHeight = TempRect.bottom - TempRect.top;

		char TempTitle[MAX_PATH];
		GetWindowText(m_GameWindow, TempTitle, sizeof(TempTitle));
		m_GameTitle = TempTitle;

		char TempClassName[MAX_PATH];
		GetClassName(m_GameWindow, TempClassName, sizeof(TempClassName));
		m_ClassName = TempClassName;

		char TempPath[MAX_PATH];
		GetModuleFileNameEx(m_GameHandle, NULL, TempPath, sizeof(TempPath));
		m_GamePath = TempPath;

		Logger::log("Program Data:");
		Logger::log("\tGame Pid: " + std::to_string(m_GamePid));
		Logger::log("\tGame Module Base: " + std::to_string(m_ModuleBase));
		Logger::log("\tGame Title: " + std::string(m_GameTitle));
		Logger::log("\tGame Class Name: " + std::string(m_ClassName));
		Logger::log("\tGame Game Path: " + std::string(m_GamePath));
		Logger::log("\tGame Window Width: " + std::to_string(m_GameWidth));
		Logger::log("\tGame Window Height: " + std::to_string(m_GameHeight));
		Logger::log("\tIs Window Class Unicode: " + std::to_string(IsWindowUnicode(m_GameWindow)));
	}

	//-----------------------------------------------------------------------------------
	//									    D3DWindow
	//-----------------------------------------------------------------------------------
	static uint64_t* MethodsTable = NULL;
	
	bool D3DRenderer::loadTextureFileData(const std::string& filename, TextureFileData* textureFileData)
	{
		// Load from disk into a raw RGBA buffer
		int imageWidth = 0;
		int imageHeight = 0;
		unsigned char* imageData = stbi_load(filename.c_str(), &imageWidth, &imageHeight, NULL, 4);
		if (imageData == NULL)
			return false;

		textureFileData->width = imageWidth;
		textureFileData->height = imageHeight;
		textureFileData->data = imageData;

		return true;
	}

	void D3DRenderer::loadBarTextures()
	{
		if (!TextureData::useTextures)
			return;

		if (!loadTextureFileData(TextureFileData::bossBarFile, &bossBarFileData))
		{
			Logger::log("Failed to load \"" + TextureFileData::bossBarFile + "\" texture file", LogLevel::Warning);
			return;
		}

		if (!loadTextureFileData(TextureFileData::bossBorderFile, &bossBarBorderFileData))
		{
			Logger::log("Failed to load \"" + TextureFileData::bossBorderFile + "\" texture file", LogLevel::Warning);
			return;
		}

		if (!loadTextureFileData(TextureFileData::entityBarFile, &entityBarFileData))
		{
			Logger::log("Failed to load \"" + TextureFileData::entityBarFile + "\" texture file", LogLevel::Warning);
			return;
		}

		if (!loadTextureFileData(TextureFileData::entityBorderFile, &entityBarBorderFileData))
		{
			Logger::log("Failed to load \"" + TextureFileData::entityBorderFile + "\" texture file", LogLevel::Warning);
			return;
		}

		textureFileDataLoaded = true;
		Logger::log("All texture files loaded successfully");
	}

	bool D3DRenderer::Hook()
	{
		Logger::log("Hooking D3DWindow");
		if (InitHook()) 
		{
			Logger::log("Hooking ExecuteCommandLists");
			CreateHook(54, (void**)&oExecuteCommandLists, HookExecuteCommandLists);
			Logger::log("Hooking Present");
			CreateHook(140, (void**)&oPresent, HookPresent);
			Logger::log("Hooking ResizeTarget");
			CreateHook(146, (void**)&oResizeTarget, HookResizeTarget);
			return 1;
		}

		Logger::log("Failed to init D3DWindow hooking", LogLevel::Error);
		return 0;
	}

	void D3DRenderer::Unhook()
	{
		Logger::log("Unhooking D3DWindow");
		SetWindowLongPtr(programData->m_GameWindow, GWLP_WNDPROC, (LONG_PTR)m_OldWndProc);
		DisableAll();
	}

	void D3DRenderer::EnableDebugLayer()
	{
#if defined(DEBUGLOG)
			// Always enable the debug layer before doing anything DX12 related
			// so all possible errors generated while creating DX12 objects
			// are caught by the debug layer.
			ID3D12Debug* debugInterface;
			if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugInterface))))
			{
				Logger::log("Enabled debug layer");
				debugInterface->EnableDebugLayer();
			}
#endif
	}

    bool D3DRenderer::InitHook()
    {
#ifdef DEBUGLOG
		try
		{
#endif // DEBUGLOG
		Logger::log("Init D3DWindow");
		if (!InitWindow()) 
		{
			Logger::log("Failed to init D3DWindow", LogLevel::Error);
			return false;
		}

		Logger::log("Getting D3D12 and DXGI modules");
		HMODULE D3D12Module = GetModuleHandle("d3d12.dll");
		HMODULE DXGIModule = GetModuleHandle("dxgi.dll");
		if (D3D12Module == NULL || DXGIModule == NULL) 
		{
			Logger::log("Failed to get DX modules: D3D12Module - " + std::to_string(D3D12Module != NULL) + "DXGIModule - " + std::to_string(DXGIModule != NULL), LogLevel::Error);
			DeleteWindow();
			return false;
		}

		Logger::log("Getting DXGI factory");
		void* CreateDXGIFactory = GetProcAddress(DXGIModule, "CreateDXGIFactory");
		if (CreateDXGIFactory == NULL) 
		{
			Logger::log("Failed to get DXGI factory", LogLevel::Error);
			DeleteWindow();
			return false;
		}

		Logger::log("Init DXGI factory");
		IDXGIFactory* Factory;
		if (((long(__stdcall*)(const IID&, void**))(CreateDXGIFactory))(IID_PPV_ARGS(&Factory)) < 0)
		{
			Logger::log("Failed to init DXGI factory", LogLevel::Error);
			DeleteWindow();
			return false;
		}

		Logger::log("Getting IDXGI adapter");
		IDXGIAdapter* Adapter;
		if (Factory->EnumAdapters(0, &Adapter) == DXGI_ERROR_NOT_FOUND) 
		{
			Logger::log("Failed to get IDXGI adapter", LogLevel::Error);
			DeleteWindow();
			return false;
		}

		Logger::log("Getting D3D12CreateDevice process");
		void* D3D12CreateDevice = GetProcAddress(D3D12Module, "D3D12CreateDevice");
		if (D3D12CreateDevice == NULL) 
		{
			Logger::log("Failed to get D3D12CreateDevice process", LogLevel::Error);
			DeleteWindow();
			return false;
		}

		Logger::log("Init ID3D12Device");
		ID3D12Device* Device;
		auto test = ((HRESULT(__stdcall*)(IUnknown*, D3D_FEATURE_LEVEL, const IID&, void**))(D3D12CreateDevice))(Adapter, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&Device));
		if (test < 0)
		{
			Logger::log("Failed to init ID3D12Device", LogLevel::Error);
			DeleteWindow();
			return false;
		}

		D3D12_COMMAND_QUEUE_DESC QueueDesc;
		QueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
		QueueDesc.Priority = 0;
		QueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
		QueueDesc.NodeMask = 0;

		Logger::log("Init ID3D12Device command queue");
		ID3D12CommandQueue* CommandQueue;
		if (Device->CreateCommandQueue(&QueueDesc, IID_PPV_ARGS(&CommandQueue)) < 0)
		{
			Logger::log("Failed to init ID3D12Device command queue", LogLevel::Error);
			DeleteWindow();
			return false;
		}

		Logger::log("Init ID3D12Device command allocator");
		ID3D12CommandAllocator* CommandAllocator;
		if (Device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&CommandAllocator)) < 0)
		{
			Logger::log("Failed to init ID3D12Device command allocator", LogLevel::Error);
			DeleteWindow();
			return false;
		}

		Logger::log("Init ID3D12Device command list");
		ID3D12GraphicsCommandList* CommandList;
		if (Device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, CommandAllocator, NULL, IID_PPV_ARGS(&CommandList)) < 0)
		{
			Logger::log("Failed to init ID3D12Device command list", LogLevel::Error);
			DeleteWindow();
			return false;
		}

		DXGI_RATIONAL RefreshRate;
		RefreshRate.Numerator = 60;
		RefreshRate.Denominator = 1;

		DXGI_MODE_DESC BufferDesc;
		BufferDesc.Width = 100;
		BufferDesc.Height = 100;
		BufferDesc.RefreshRate = RefreshRate;
		BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
		BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;

		DXGI_SAMPLE_DESC SampleDesc;
		SampleDesc.Count = 1;
		SampleDesc.Quality = 0;

		DXGI_SWAP_CHAIN_DESC SwapChainDesc = {};
		SwapChainDesc.BufferDesc = BufferDesc;
		SwapChainDesc.SampleDesc = SampleDesc;
		SwapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		SwapChainDesc.BufferCount = 2;
		SwapChainDesc.OutputWindow = WindowHwnd;
		SwapChainDesc.Windowed = 1;
		SwapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
		SwapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

		Logger::log("Init ID3D12Device swap chain");
		IDXGISwapChain* SwapChain;
		if (Factory->CreateSwapChain(CommandQueue, &SwapChainDesc, &SwapChain) < 0) 
		{
			Logger::log("Failed to init ID3D12Device swap chain", LogLevel::Error);
			DeleteWindow();
			return false;
		}

		Logger::log("Allock D3DWindow method tables");
		MethodsTable = (uint64_t*)::calloc(150, sizeof(uint64_t));
		memcpy(MethodsTable, *(uint64_t**)Device, 44 * sizeof(uint64_t));
		memcpy(MethodsTable + 44, *(uint64_t**)CommandQueue, 19 * sizeof(uint64_t));
		memcpy(MethodsTable + 44 + 19, *(uint64_t**)CommandAllocator, 9 * sizeof(uint64_t));
		memcpy(MethodsTable + 44 + 19 + 9, *(uint64_t**)CommandList, 60 * sizeof(uint64_t));
		memcpy(MethodsTable + 44 + 19 + 9 + 60, *(uint64_t**)SwapChain, 18 * sizeof(uint64_t));
		Sleep(1000);

		MH_Initialize();
		Logger::log("Releasing D3DWindow");
		Device->Release();
		Device = NULL;
		CommandQueue->Release();
		CommandQueue = NULL;
		CommandAllocator->Release();
		CommandAllocator = NULL;
		CommandList->Release();
		CommandList = NULL;
		SwapChain->Release();
		SwapChain = NULL;
		DeleteWindow();
#ifdef DEBUGLOG
		}
		catch (const std::exception& e)
		{
			Logger::useLogger = true;
			Logger::log(e.what(), LogLevel::Error);
			throw;
		}
		catch (...)
		{
			Logger::useLogger = true;
			Logger::log("Unknown exception during D3DRenderer::InitHook", LogLevel::Error);
			throw;
		}
#endif // DEBUGLOG
		return true;
    }

	// Simple helper function to init an image into a DX12 texture with common settings
	D3D12TextureData initD3D12Texture(const TextureFileData& textureFileData, ID3D12Device* device, ID3D12DescriptorHeap* descriptor, int descriptor_index)
	{
		// We need to pass a D3D12_CPU_DESCRIPTOR_HANDLE in ImTextureID, so make sure it will fit
		static_assert(sizeof(ImTextureID) >= sizeof(D3D12_CPU_DESCRIPTOR_HANDLE), "D3D12_CPU_DESCRIPTOR_HANDLE is too large to fit in an ImTextureID");

		// We presume here that we have our D3D device pointer in g_pd3dDevice
		D3D12TextureData textureData;

		// Get CPU/GPU handles for the shader resource view
		// Normally your engine will have some sort of allocator for these - here we assume that there's an SRV descriptor heap in
		// g_pd3dSrvDescHeap with at least two descriptors allocated, and descriptor 1 is unused
		UINT handle_increment = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		textureData.cpuHandle = descriptor->GetCPUDescriptorHandleForHeapStart();
		textureData.cpuHandle.ptr += (handle_increment * descriptor_index);
		textureData.gpuHandle = descriptor->GetGPUDescriptorHandleForHeapStart();
		textureData.gpuHandle.ptr += (handle_increment * descriptor_index);

		int image_width = textureFileData.width;
		int image_height = textureFileData.height;
		unsigned char* image_data = textureFileData.data;

		// Create texture resource
		D3D12_HEAP_PROPERTIES props;
		memset(&props, 0, sizeof(D3D12_HEAP_PROPERTIES));
		props.Type = D3D12_HEAP_TYPE_DEFAULT;
		props.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
		props.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;

		D3D12_RESOURCE_DESC desc;
		ZeroMemory(&desc, sizeof(desc));
		desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		desc.Alignment = 0;
		desc.Width = image_width;
		desc.Height = image_height;
		desc.DepthOrArraySize = 1;
		desc.MipLevels = 1;
		desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		desc.SampleDesc.Count = 1;
		desc.SampleDesc.Quality = 0;
		desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
		desc.Flags = D3D12_RESOURCE_FLAG_NONE;

		ID3D12Resource* pTexture = NULL;
		device->CreateCommittedResource(&props, D3D12_HEAP_FLAG_NONE, &desc, D3D12_RESOURCE_STATE_COPY_DEST, NULL, IID_PPV_ARGS(&pTexture));

		// Create a temporary upload resource to move the data in
		UINT uploadPitch = (image_width * 4 + D3D12_TEXTURE_DATA_PITCH_ALIGNMENT - 1u) & ~(D3D12_TEXTURE_DATA_PITCH_ALIGNMENT - 1u);
		UINT uploadSize = image_height * uploadPitch;
		desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
		desc.Alignment = 0;
		desc.Width = uploadSize;
		desc.Height = 1;
		desc.DepthOrArraySize = 1;
		desc.MipLevels = 1;
		desc.Format = DXGI_FORMAT_UNKNOWN;
		desc.SampleDesc.Count = 1;
		desc.SampleDesc.Quality = 0;
		desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
		desc.Flags = D3D12_RESOURCE_FLAG_NONE;

		props.Type = D3D12_HEAP_TYPE_UPLOAD;
		props.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
		props.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;

		ID3D12Resource* uploadBuffer = NULL;
		HRESULT hr = device->CreateCommittedResource(&props, D3D12_HEAP_FLAG_NONE, &desc, D3D12_RESOURCE_STATE_GENERIC_READ, NULL, IID_PPV_ARGS(&uploadBuffer));
		IM_ASSERT(SUCCEEDED(hr));

		// Write pixels into the upload resource
		void* mapped = NULL;
		D3D12_RANGE range = { 0, uploadSize };
		hr = uploadBuffer->Map(0, &range, &mapped);
		IM_ASSERT(SUCCEEDED(hr));
		for (int y = 0; y < image_height; y++)
			memcpy((void*)((uintptr_t)mapped + y * uploadPitch), image_data + y * image_width * 4, image_width * 4);
		uploadBuffer->Unmap(0, &range);

		// Copy the upload resource content into the real resource
		D3D12_TEXTURE_COPY_LOCATION srcLocation = {};
		srcLocation.pResource = uploadBuffer;
		srcLocation.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
		srcLocation.PlacedFootprint.Footprint.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		srcLocation.PlacedFootprint.Footprint.Width = image_width;
		srcLocation.PlacedFootprint.Footprint.Height = image_height;
		srcLocation.PlacedFootprint.Footprint.Depth = 1;
		srcLocation.PlacedFootprint.Footprint.RowPitch = uploadPitch;

		D3D12_TEXTURE_COPY_LOCATION dstLocation = {};
		dstLocation.pResource = pTexture;
		dstLocation.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
		dstLocation.SubresourceIndex = 0;

		D3D12_RESOURCE_BARRIER barrier = {};
		barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		barrier.Transition.pResource = pTexture;
		barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
		barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
		barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;

		// Create a temporary command queue to do the copy with
		ID3D12Fence* fence = NULL;
		hr = device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence));
		IM_ASSERT(SUCCEEDED(hr));

		HANDLE event = CreateEvent(0, 0, 0, 0);
		IM_ASSERT(event != NULL);

		D3D12_COMMAND_QUEUE_DESC queueDesc = {};
		queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
		queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
		queueDesc.NodeMask = 1;

		ID3D12CommandQueue* cmdQueue = NULL;
		hr = device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&cmdQueue));
		IM_ASSERT(SUCCEEDED(hr));

		ID3D12CommandAllocator* cmdAlloc = NULL;
		hr = device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&cmdAlloc));
		IM_ASSERT(SUCCEEDED(hr));

		ID3D12GraphicsCommandList* cmdList = NULL;
		hr = device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, cmdAlloc, NULL, IID_PPV_ARGS(&cmdList));
		IM_ASSERT(SUCCEEDED(hr));

		cmdList->CopyTextureRegion(&dstLocation, 0, 0, 0, &srcLocation, NULL);
		cmdList->ResourceBarrier(1, &barrier);

		hr = cmdList->Close();
		IM_ASSERT(SUCCEEDED(hr));

		// Execute the copy
		cmdQueue->ExecuteCommandLists(1, (ID3D12CommandList* const*)&cmdList);
		hr = cmdQueue->Signal(fence, 1);
		IM_ASSERT(SUCCEEDED(hr));

		// Wait for everything to complete
		fence->SetEventOnCompletion(1, event);
		WaitForSingleObject(event, INFINITE);

		// Tear down our temporary command queue and release the upload resource
		cmdList->Release();
		cmdAlloc->Release();
		cmdQueue->Release();
		CloseHandle(event);
		fence->Release();
		uploadBuffer->Release();

		// Create a shader resource view for the texture
		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc;
		ZeroMemory(&srvDesc, sizeof(srvDesc));
		srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Texture2D.MipLevels = desc.MipLevels;
		srvDesc.Texture2D.MostDetailedMip = 0;
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		device->CreateShaderResourceView(pTexture, &srvDesc, textureData.cpuHandle);

		// Return results
		textureData.dx12Resource = pTexture;
		textureData.width = image_width;
		textureData.height = image_height;

		return textureData;
	}

	void D3DRenderer::Overlay(IDXGISwapChain* pSwapChain)
	{
#ifdef DEBUGLOG
		try
		{
#endif // DEBUGLOG
		if (m_CommandQueue == nullptr)
			return;

		ID3D12CommandQueue* pCmdQueue = this->m_CommandQueue;

		IDXGISwapChain3* pSwapChain3 = nullptr;
		DXGI_SWAP_CHAIN_DESC sc_desc;
		pSwapChain->QueryInterface(IID_PPV_ARGS(&pSwapChain3));
		if (pSwapChain3 == nullptr)
		{
			Logger::log("Failed to query interface for IDXGISwapChain3", LogLevel::Debug);
			return;
		}

		pSwapChain3->GetDesc(&sc_desc);

		if (!m_Init)
		{
			UINT bufferIndex = pSwapChain3->GetCurrentBackBufferIndex();
			ID3D12Device* pDevice;
			if (pSwapChain3->GetDevice(IID_PPV_ARGS(&pDevice)) != S_OK)
			{
				Logger::log("Failed to get device for ID3D12Device", LogLevel::Warning);
				return;
			}

			m_RenderTargets.clear();
			m_BuffersCounts = sc_desc.BufferCount;

			{
				D3D12_DESCRIPTOR_HEAP_DESC desc = {};
				desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
				// 1 + 8 from 4 textures (cpu + gpu handles)
				desc.NumDescriptors = textureFileDataLoaded ? 9 : 1;
				desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
				if (pDevice->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&m_srvDescriptorHeap)) != S_OK)
				{
					pDevice->Release();
					pSwapChain3->Release();
					Logger::log("Failed to create descriptor heap type D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV", LogLevel::Warning);
					return;
				}

				if (textureFileDataLoaded)
				{
					Logger::log("Init texture files into DirectX12");
					auto&& bossBarBorderTextureData = initD3D12Texture(bossBarBorderFileData, pDevice, m_srvDescriptorHeap, 2);
					auto&& bossBarTextureData = initD3D12Texture(bossBarFileData, pDevice, m_srvDescriptorHeap, 3);
					auto&& entityBarBorderTextureData = initD3D12Texture(entityBarBorderFileData, pDevice, m_srvDescriptorHeap, 4);
					auto&& entityBarTextureData = initD3D12Texture(entityBarFileData, pDevice, m_srvDescriptorHeap, 5);

					if (!(bossBarBorderTextureData.dx12Resource && bossBarTextureData.dx12Resource && entityBarBorderTextureData.dx12Resource && entityBarTextureData.dx12Resource))
					{
						pDevice->Release();
						pSwapChain3->Release();
						Logger::log("Failed to init textures into DirectX12", LogLevel::Warning);
						return;
					}

					g_postureUI->bossBarTexture = TextureBar(
						TextureData((ImTextureID)bossBarBorderTextureData.gpuHandle.ptr, (float)bossBarBorderTextureData.width, (float)bossBarBorderTextureData.height), 
						TextureData((ImTextureID)bossBarTextureData.gpuHandle.ptr, (float)bossBarTextureData.width, (float)bossBarTextureData.height)
					);
					
					g_postureUI->entityBarTexture = TextureBar(
						TextureData((ImTextureID)entityBarBorderTextureData.gpuHandle.ptr, (float)entityBarBorderTextureData.width, (float)entityBarBorderTextureData.height), 
						TextureData((ImTextureID)entityBarTextureData.gpuHandle.ptr, (float)entityBarTextureData.width, (float)entityBarTextureData.height)
					);
					g_postureUI->textureBarInit = true;
					Logger::log("All texture init successfully");
				}
			}
			{
				D3D12_DESCRIPTOR_HEAP_DESC desc = {};
				desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
				desc.NumDescriptors = sc_desc.BufferCount;
				desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
				desc.NodeMask = 1;
				if (pDevice->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&m_rtvDescriptorHeap)) != S_OK)
				{
					pDevice->Release();
					pSwapChain3->Release();
					m_srvDescriptorHeap->Release();
					Logger::log("Failed to create descriptor heap type D3D12_DESCRIPTOR_HEAP_TYPE_RTV", LogLevel::Warning);
					return;
				}

				SIZE_T rtvDescriptorSize = pDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
				D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = m_rtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
				m_CommandAllocators = new ID3D12CommandAllocator * [sc_desc.BufferCount];
				for (IndexType i = 0; i < sc_desc.BufferCount; ++i)
				{
					m_RenderTargets.push_back(rtvHandle);
					rtvHandle.ptr += rtvDescriptorSize;
				}
			}

			for (IndexType i = 0; i < sc_desc.BufferCount; ++i)
			{
				if (pDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_CommandAllocators[i])) != S_OK)
				{
					pDevice->Release();
					pSwapChain3->Release();
					m_srvDescriptorHeap->Release();
					for (UINT j = 0; j < i; ++j)
					{
						m_CommandAllocators[j]->Release();
					}
					m_rtvDescriptorHeap->Release();
					delete[]m_CommandAllocators;
					Logger::log("Failed to create command allocator D3D12_COMMAND_LIST_TYPE_DIRECT buffer idx: " + std::to_string(i), LogLevel::Warning);
					return;
				}
			}

			if (pDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_CommandAllocators[0], NULL, IID_PPV_ARGS(&m_CommandList)) != S_OK ||
				m_CommandList->Close() != S_OK)
			{
				pDevice->Release();
				pSwapChain3->Release();
				m_srvDescriptorHeap->Release();
				for (IndexType i = 0; i < sc_desc.BufferCount; ++i)
					m_CommandAllocators[i]->Release();
				m_rtvDescriptorHeap->Release();
				delete[]m_CommandAllocators;
				Logger::log("Failed to create command list D3D12_COMMAND_LIST_TYPE_DIRECT", LogLevel::Warning);
				return;
			}

			m_BackBuffers = new ID3D12Resource * [sc_desc.BufferCount];
			for (IndexType i = 0; i < sc_desc.BufferCount; i++)
			{
				pSwapChain3->GetBuffer(i, IID_PPV_ARGS(&m_BackBuffers[i]));
				pDevice->CreateRenderTargetView(m_BackBuffers[i], NULL, m_RenderTargets[i]);
			}

			Logger::log("ImGui CreateContext");
			ImGui::CreateContext();
			ImGuiIO& io = ImGui::GetIO();
			io.IniFilename = NULL;

			programData->m_GameWindow = FindERWindow();

			Logger::log("Init ImGui for found window");
			if (!ImGui_ImplWin32_Init(programData->m_GameWindow))
			{
				Logger::log("Failed to init ImGui for found window", LogLevel::Warning);
				return;
			}

			Logger::log("Init ImGui for DX12");
			if (!ImGui_ImplDX12_Init(pDevice, sc_desc.BufferCount, DXGI_FORMAT_R8G8B8A8_UNORM, NULL, m_srvDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), m_srvDescriptorHeap->GetGPUDescriptorHandleForHeapStart()))
			{
				Logger::log("Failed to init ImGui for DX12", LogLevel::Warning);
				return;
			}

			Logger::log("ImGui DX12 create device object");
			if (!ImGui_ImplDX12_CreateDeviceObjects())
			{
				Logger::log("Failed to create ImGui DX12 device object", LogLevel::Warning);
				return;
			}

			ImGui::GetIO().ImeWindowHandle = programData->m_GameWindow;
			m_OldWndProc = SetWindowLongPtr(programData->m_GameWindow, GWLP_WNDPROC, (LONG_PTR)WndProc);

			m_Init = true;

			pDevice->Release();

			Logger::log("ImGui init successful!");
		}

		Logger::log("ImGui create new farme", LogLevel::Debug);
		ImGui_ImplDX12_NewFrame();
		ImGui_ImplWin32_NewFrame();
		ImGui::NewFrame();

		Logger::log("Draw posture bar UI", LogLevel::Debug);
		g_postureUI->Draw();

		Logger::log("ImGui end frame", LogLevel::Debug);
		ImGui::EndFrame();

		UINT bufferIndex = pSwapChain3->GetCurrentBackBufferIndex();
		Logger::log("ImGui swap chain buffer index: " + std::to_string(bufferIndex), LogLevel::Debug);

		D3D12_RESOURCE_BARRIER barrier = {};
		barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		barrier.Transition.pResource = m_BackBuffers[bufferIndex];
		barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
		barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
		barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;

		m_CommandAllocators[bufferIndex]->Reset();
		m_CommandList->Reset(m_CommandAllocators[bufferIndex], NULL);
		m_CommandList->ResourceBarrier(1, &barrier);
		m_CommandList->OMSetRenderTargets(1, &m_RenderTargets[bufferIndex], FALSE, NULL);
		m_CommandList->SetDescriptorHeaps(1, &m_srvDescriptorHeap);

		Logger::log("ImGui render", LogLevel::Debug);
		ImGui::Render();
		ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), m_CommandList);

		barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
		barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
		m_CommandList->ResourceBarrier(1, &barrier);
		m_CommandList->Close();

		Logger::log("ID3D12CommandList execute command list", LogLevel::Debug);
		pCmdQueue->ExecuteCommandLists(1, (ID3D12CommandList**)&m_CommandList);

		Logger::log("IDXGISwapChain3 release", LogLevel::Debug);
		pSwapChain3->Release();

#ifdef DEBUGLOG
		}
		catch (const std::exception& e)
		{
			Logger::useLogger = true;
			Logger::log(e.what(), LogLevel::Error);
			throw;
		}
		catch (...)
		{
			Logger::useLogger = true;
			Logger::log("Unknown exception during D3DRenderer::Overlay", LogLevel::Error);
			throw;
		}
#endif // DEBUGLOG
	}

	LRESULT D3DRenderer::WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
	{
		if (ImGui::GetCurrentContext())
			ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam);

		return CallWindowProc((WNDPROC)g_D3DRenderer->m_OldWndProc, hWnd, msg, wParam, lParam);
	}

	HRESULT APIENTRY D3DRenderer::HookResizeTarget(IDXGISwapChain* _this, const DXGI_MODE_DESC* pNewTargetParameters)
	{
		Logger::log("Enter HookResizeTarget", LogLevel::Debug);

		if (g_D3DRenderer->m_Init)
			g_D3DRenderer->ResetRenderState();

		return g_D3DRenderer->oResizeTarget(_this, pNewTargetParameters);
	}

    HRESULT APIENTRY D3DRenderer::HookPresent(IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags)
    {
		Logger::log("Enter HookPresent", LogLevel::Debug);
		if (g_KillSwitch) 
		{
			Logger::log("Kill switch");
			g_Hooking->Unhook();
			g_D3DRenderer->oPresent(pSwapChain, SyncInterval, Flags);
			g_Running = FALSE;
			return 0;

		}
		g_D3DRenderer->Overlay(pSwapChain);
		return g_D3DRenderer->oPresent(pSwapChain, SyncInterval, Flags);

    }

	void D3DRenderer::HookExecuteCommandLists(ID3D12CommandQueue* queue, UINT NumCommandLists, ID3D12CommandList* ppCommandLists) 
	{
		if (!g_D3DRenderer->m_CommandQueue)
			g_D3DRenderer->m_CommandQueue = queue;

		g_D3DRenderer->oExecuteCommandLists(queue, NumCommandLists, ppCommandLists);
	}

	void D3DRenderer::ResetRenderState(IDXGISwapChain3* swapChain /*= nullptr*/)
	{
		if (swapChain)
			swapChain->Release();

		if (m_Device)
			m_Device->Release();

		if (m_CommandList)
			m_CommandList->Release();

		if (m_srvDescriptorHeap)
			m_srvDescriptorHeap->Release();

		if (m_rtvDescriptorHeap)
			m_rtvDescriptorHeap->Release();

		for (IndexType i = 0; i < m_BuffersCounts; i++)
		{
			m_CommandAllocators[i]->Release();
			m_BackBuffers[i]->Release();
		}

		m_RenderTargets.clear();
		delete[] m_CommandAllocators;
		delete[] m_BackBuffers;

		ImGui_ImplDX12_Shutdown();
		SetWindowLongPtr(programData->m_GameWindow, GWLP_WNDPROC, (LONG_PTR)m_OldWndProc);
		ImGui_ImplWin32_Shutdown();
		ImGui::DestroyContext();

		g_postureUI->textureBarInit = false;
		m_Init = false;
	}

	bool D3DRenderer::InitWindow() 
	{

		WindowClass.cbSize = sizeof(WNDCLASSEX);
		WindowClass.style = CS_HREDRAW | CS_VREDRAW;
		WindowClass.lpfnWndProc = DefWindowProc;
		WindowClass.cbClsExtra = 0;
		WindowClass.cbWndExtra = 0;
		WindowClass.hInstance = GetModuleHandle(NULL);
		WindowClass.hIcon = NULL;
		WindowClass.hCursor = NULL;
		WindowClass.hbrBackground = NULL;
		WindowClass.lpszMenuName = NULL;
		WindowClass.lpszClassName = "MJ";
		WindowClass.hIconSm = NULL;
		RegisterClassEx(&WindowClass);
		WindowHwnd = CreateWindow(WindowClass.lpszClassName, "DirectX Window", WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_THICKFRAME, 0, 0, 100, 100, NULL, NULL, WindowClass.hInstance, NULL);
		if (WindowHwnd == NULL)
			return false;

		return true;
	}

	bool D3DRenderer::DeleteWindow() 
	{
		DestroyWindow(WindowHwnd);
		UnregisterClass(WindowClass.lpszClassName, WindowClass.hInstance);
		if (WindowHwnd != NULL) {
			return false;
		}
		return true;
	}

    bool D3DRenderer::CreateHook(uint16_t Index, void** Original, void* Function) 
	{
        void* target = (void*)MethodsTable[Index];
        if (MH_CreateHook(target, Function, Original) != MH_OK || MH_EnableHook(target) != MH_OK) {
            return false;
        }
        return true;
    }

    void D3DRenderer::DisableHook(uint16_t Index) 
	{
        MH_DisableHook((void*)MethodsTable[Index]);
    }

    void D3DRenderer::DisableAll() 
	{
		DisableHook(MethodsTable[54]);
		DisableHook(MethodsTable[140]);
		DisableHook(MethodsTable[146]);
    }

	D3DRenderer::~D3DRenderer() noexcept
	{
		g_Running = false;
		Unhook();
		stbi_image_free(entityBarBorderFileData.data);
		stbi_image_free(entityBarFileData.data);
		stbi_image_free(bossBarBorderFileData.data);
		stbi_image_free(bossBarFileData.data);
	}
}