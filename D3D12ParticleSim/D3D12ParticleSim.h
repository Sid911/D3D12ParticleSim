#include "DXWindow.h"
#include "Camera.h"
#include "MyTimer.h"
#include <array>

using Microsoft::WRL::ComPtr;

class D3D12ParticleSim : public DXWindow
{
public:
	D3D12ParticleSim(UINT width, UINT height, std::wstring name);

	virtual void OnInit();
	virtual void OnUpdate();
	virtual void OnRender();
	virtual void OnDestroy();
	virtual void OnKeyDown(UINT8 key);
	virtual void OnKeyUp(UINT8 key);


private:
	static const int FrameCount = 2;
	static const float ParticleSpread;
	static const int ParticleCount = 5000;		// The number of particles in the n-body simulation.

	static const bool AsynchronousComputeEnabled = true;

	struct ParticleVertex
	{
		XMFLOAT4 color;
	};

	struct Particle
	{
		XMFLOAT4 position;
		XMFLOAT4 velocity;
	};

	struct ConstantBufferGS
	{
		XMFLOAT4X4 worldViewProjection;
		XMFLOAT4X4 inverseView;

		// Constant buffers are 256-byte aligned in GPU memory. Padding is added
		// for convenience when computing the struct's size.
		float padding[32];
	};

	struct ConstantBufferCS
	{
		UINT param[4];
		float paramf[4];
	};

	// Pipeline objects.
	D3D12_VIEWPORT m_viewport;
	D3D12_RECT m_scissorRect;
	ComPtr<IDXGISwapChain3> m_swapChain;
	ComPtr<ID3D12Device> m_device;
	ComPtr<ID3D12Resource> m_renderTargets[FrameCount];
	UINT m_frameIndex;
	UINT m_lastFrameIndex;
	ComPtr<ID3D12CommandAllocator> m_graphicsAllocators[FrameCount];
	ComPtr<ID3D12GraphicsCommandList> m_graphicsCommandLists[FrameCount];
	ComPtr<ID3D12CommandAllocator> m_graphicsCopyAllocators[FrameCount];
	ComPtr<ID3D12GraphicsCommandList> m_graphicsCopyCommandLists[FrameCount];

	ComPtr<ID3D12GraphicsCommandList> m_uploadCommandList;
	ComPtr<ID3D12CommandAllocator> m_uploadCommandAllocator;
	ComPtr<ID3D12Fence> m_uploadFence;
	HANDLE m_uploadEvent;
	UINT64 m_uploadFenceValue;

	ComPtr<ID3D12CommandQueue> m_graphicsCommandQueue;
	ComPtr<ID3D12RootSignature> m_rootSignature;
	ComPtr<ID3D12RootSignature> m_computeRootSignature;
	ComPtr<ID3D12DescriptorHeap> m_rtvHeap;
	//ComPtr<ID3D12DescriptorHeap> m_srvImGUIHeap;
	ComPtr<ID3D12DescriptorHeap> m_srvUavHeap;
	UINT m_rtvDescriptorSize;
	UINT m_srvUavDescriptorSize;
	//UINT m_srvImGUIDescriptorSize;

	// Timing queries
	ComPtr<ID3D12QueryHeap> m_timeQueryHeap;
	ComPtr<ID3D12Resource> m_timeQueryReadbackBuffer[FrameCount];
	UINT64 m_queryResults[FrameCount];
	int m_queryReadbackIndex;
	UINT64 m_frequency;

	// Asset objects.
	ComPtr<ID3D12PipelineState> m_pipelineState;
	ComPtr<ID3D12PipelineState> m_computeState;
	ComPtr<ID3D12Resource> m_vertexBuffer;
	ComPtr<ID3D12Resource> m_vertexBufferUpload;
	D3D12_VERTEX_BUFFER_VIEW m_vertexBufferView;
	ComPtr<ID3D12Resource> m_particleBuffer0;
	ComPtr<ID3D12Resource> m_particleBuffer1;
	ComPtr<ID3D12Resource> m_particleBufferForDraw;
	ComPtr<ID3D12Resource> m_particleBuffer0Upload;
	ComPtr<ID3D12Resource> m_particleBuffer1Upload;
	ComPtr<ID3D12Resource> m_constantBufferGS;
	UINT8* m_pConstantBufferGSData;
	ComPtr<ID3D12Resource> m_constantBufferCS;

	UINT m_srvIndex;		// Denotes which of the particle buffer resource views is the SRV (0 or 1). The UAV is 1 - srvIndex.
	UINT GetParticleBufferSrvIndex() const
	{
		return m_srvIndex;
	}

	UINT GetParticleBufferUavIndex() const
	{
		return 1 - m_srvIndex;
	}

	SimpleCamera m_camera;
	StepTimer m_timer;

	// Compute objects.
	ComPtr<ID3D12CommandAllocator> m_computeAllocators[FrameCount];
	ComPtr<ID3D12CommandQueue> m_computeCommandQueue;
	ComPtr<ID3D12GraphicsCommandList> m_computeCommandLists[FrameCount];

	// Synchronization objects.
	ComPtr<ID3D12Fence>	m_computeFences[FrameCount];
	ComPtr<ID3D12Fence> m_graphicsFences[FrameCount];
	ComPtr<ID3D12Fence> m_graphicsCopyFences[FrameCount];

	UINT64 m_computeFenceValue;
	UINT64 m_graphicsFenceValue;
	UINT64 m_graphicsCopyFenceValue;
	UINT64 m_computeFenceValues[FrameCount];
	UINT64 m_graphicsFenceValues[FrameCount];
	UINT64 m_graphicsCopyFenceValues[FrameCount];

	HANDLE m_computeFenceEvents[FrameCount];
	HANDLE m_graphicsFenceEvents[FrameCount];
	HANDLE m_graphicsCopyFenceEvents[FrameCount];

	UINT64 m_frameFenceValue;
	UINT64 m_frameFenceValues[FrameCount];
	ComPtr<ID3D12Fence> m_frameFences[FrameCount];
	HANDLE m_frameFenceEvents[FrameCount];

	int m_frameTimeEntryCount;
	int m_frameTimeNextEntry;
	std::array<double, 64> m_frameTimes;

	// Indices in the root parameter table.
	enum RootParameters : UINT32
	{
		RootParameterCB = 0,
		RootParameterSRV,
		RootParameterUAV,
		RootParametersCount
	};

	// Indices of shader resources in the descriptor heap.
	enum DescriptorHeapIndex : UINT32
	{
		UavParticlePosVelo0,
		UavParticlePosVelo1,
		SrvParticlePosVelo0,
		SrvParticlePosVelo1,
		SrvParticleForDraw,
		DescriptorCount
	};

	void SetupImgui();
	void LoadPipeline();
	void LoadAssets();
	void CreateVertexBuffer();

	float RandomPercent();
	void LoadParticles(_Out_writes_(numParticles) Particle* pParticles, const XMFLOAT3& center, const XMFLOAT4& velocity, float spread, UINT numParticles);
	void CreateParticleBuffers();
	void RecordRenderCommandList();

	void Simulate();
	void RecordComputeCommandList();

	void RecordCopyCommandList();

	void MoveToNextFrame();
};
