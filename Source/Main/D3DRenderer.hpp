#pragma once
#include "../Common.hpp"

namespace ER
{
	struct TextureFileData
	{
		unsigned char* data = nullptr;
		int width = 0;
		int height = 0;

		static inline std::string bossBarFile = "mods\\PostureBarResources\\BossBar.png";
		static inline std::string bossBorderFile = "mods\\PostureBarResources\\BossBarBorder.png";
		static inline std::string entityBarFile = "mods\\PostureBarResources\\EntityBar.png";
		static inline std::string entityBorderFile = "mods\\PostureBarResources\\EntityBarBorder.png";
	};

	struct D3D12TextureData
	{
		D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle{};
		D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle{};
		ID3D12Resource* dx12Resource = nullptr;
		int width = 0;
		int height = 0;
	};

	class D3DRenderer
	{
		class ProgramData
		{
		public:
			explicit ProgramData();
			~ProgramData() noexcept = default;
			ProgramData(ProgramData const&) = delete;
			ProgramData(ProgramData&&) = delete;
			ProgramData& operator=(ProgramData const&) = delete;
			ProgramData& operator=(ProgramData&&) = delete;

			//	Dx & ImGui
			int m_GamePid{};
			HANDLE m_GameHandle{};
			HWND m_GameWindow{};
			int m_GameWidth;
			int m_GameHeight;
			LPCSTR m_GameTitle;
			LPCSTR m_ClassName;
			LPCSTR m_GamePath;
			uintptr_t m_ModuleBase;		// OBTAIN MODULE BASE
		};

		typedef HRESULT(APIENTRY* Present12) (IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags);
		Present12 oPresent = NULL;

		typedef void(APIENTRY* ExecuteCommandLists)(ID3D12CommandQueue* queue, UINT NumCommandLists, ID3D12CommandList* ppCommandLists);
		ExecuteCommandLists oExecuteCommandLists = NULL;

		typedef HRESULT(APIENTRY* ResizeTarget)(IDXGISwapChain* _this, const DXGI_MODE_DESC* pNewTargetParameters);
		ResizeTarget oResizeTarget = NULL;

	public:
		explicit D3DRenderer()
			: programData(std::make_unique<ProgramData>()) {};
		~D3DRenderer() noexcept;
		D3DRenderer(D3DRenderer const&) = delete;
		D3DRenderer(D3DRenderer&&) = delete;
		D3DRenderer& operator=(D3DRenderer const&) = delete;
		D3DRenderer& operator=(D3DRenderer&&) = delete;

		bool m_Init = false;


		void Overlay(IDXGISwapChain* pSwapChain);

		static HRESULT APIENTRY HookPresent(IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags);
		static void HookExecuteCommandLists(ID3D12CommandQueue* queue, UINT NumCommandLists, ID3D12CommandList* ppCommandLists);
		static HRESULT APIENTRY HookResizeTarget(IDXGISwapChain* _this, const DXGI_MODE_DESC* pNewTargetParameters);
		void ResetRenderState(IDXGISwapChain3* swapChain = nullptr);

		bool loadTextureFileData(const std::string& filename, TextureFileData* textureFileData);
		void loadBarTextures();

		void EnableDebugLayer();
		bool InitHook();
		bool Hook();
		void Unhook();

		bool InitWindow();
		bool DeleteWindow();

		bool CreateHook(uint16_t Index, void** Original, void* Function);
		void DisableHook(uint16_t Index);
		void DisableAll();

		static LRESULT WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

		uint64_t m_OldWndProc{};

	private:

		std::unique_ptr<ProgramData> programData;

		WNDCLASSEX WindowClass{};
		HWND WindowHwnd{};

		IDXGISwapChain3* m_Swapchain{};
		ID3D12Device* m_Device{};

		TextureFileData entityBarFileData;
		TextureFileData entityBarBorderFileData;
		TextureFileData bossBarFileData;
		TextureFileData bossBarBorderFileData;
		bool textureFileDataLoaded = false;

		ID3D12DescriptorHeap* m_srvDescriptorHeap{};
		ID3D12DescriptorHeap* m_rtvDescriptorHeap{};
		ID3D12CommandAllocator** m_CommandAllocators;
		ID3D12GraphicsCommandList* m_CommandList{};
		ID3D12CommandQueue* m_CommandQueue{};
		ID3D12Resource** m_BackBuffers;
		std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> m_RenderTargets;

		uint64_t m_BuffersCounts = -1;
		// 1 + number_of_textures * 2(cpu + gpu handles)
		static inline const int srvDescriptorsNumWithTextures = 9;
	};

	inline std::unique_ptr<D3DRenderer> g_D3DRenderer;
}