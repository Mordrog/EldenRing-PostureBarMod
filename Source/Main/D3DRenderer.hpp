#pragma once
#include "../Common.hpp"

namespace ER
{
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
			IDXGISwapChain3* m_Swapchain{};
		};


		struct _FrameContext 
		{
			ID3D12CommandAllocator* CommandAllocator;
			ID3D12Resource* Resource;
			D3D12_CPU_DESCRIPTOR_HANDLE DescriptorHandle;
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


		bool Init(IDXGISwapChain3* swapChain);

		static HRESULT APIENTRY HookPresent(IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags);
		static void HookExecuteCommandLists(ID3D12CommandQueue* queue, UINT NumCommandLists, ID3D12CommandList* ppCommandLists);
		static HRESULT APIENTRY HookResizeTarget(IDXGISwapChain* _this, const DXGI_MODE_DESC* pNewTargetParameters);
		void ResetRenderState();

		bool InitHook();
		bool Hook();
		void Unhook();

		bool InitWindow();
		bool DeleteWindow();

		bool CreateHook(uint16_t Index, void** Original, void* Function);
		void DisableHook(uint16_t Index);
		void DisableAll();

		void Overlay(IDXGISwapChain* pSwapChain);

		static LRESULT WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

		uint64_t m_OldWndProc{};

	private:

		std::unique_ptr<ProgramData> programData;

		WNDCLASSEX WindowClass{};
		HWND WindowHwnd{};

		IDXGISwapChain3* m_Swapchain{};
		ID3D12Device* m_Device{};

		ID3D12DescriptorHeap* m_DescriptorHeap{};
		ID3D12DescriptorHeap* m_rtvDescriptorHeap{};
		ID3D12CommandAllocator** m_CommandAllocator;
		ID3D12GraphicsCommandList* m_CommandList{};
		ID3D12CommandQueue* m_CommandQueue{};
		ID3D12Resource** m_BackBuffer;
		std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> m_RenderTargets;

		uint64_t m_BuffersCounts = -1;
		_FrameContext* m_FrameContext{};
	};

	inline std::unique_ptr<D3DRenderer> g_D3DRenderer;
}