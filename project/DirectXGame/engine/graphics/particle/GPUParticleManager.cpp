#define NOMINMAX
#include "GPUParticleManager.h"
#include "DirectXCommon.h"
#include "Renderer.h"
#include "Method.h"
#include <cassert>
#include <dxcapi.h>

namespace {
static constexpr UINT Align256_(UINT n) { return (n + 255u) & ~255u; }

Microsoft::WRL::ComPtr<IDxcBlob> CompileShader(
    const std::wstring& filePath,
    const std::wstring& profile,
    IDxcUtils* dxcUtils,
    IDxcCompiler3* dxcCompiler,
    IDxcIncludeHandler* includeHandler) {
    
    Microsoft::WRL::ComPtr<IDxcBlobEncoding> shaderSource;
    HRESULT hr = dxcUtils->LoadFile(filePath.c_str(), nullptr, &shaderSource);
    if (FAILED(hr) || !shaderSource) {
        OutputDebugStringA("Failed to load shader file: ");
        OutputDebugStringW(filePath.c_str());
        OutputDebugStringA("\n");
        assert(false);
    }

    DxcBuffer sourceBuffer;
    sourceBuffer.Ptr = shaderSource->GetBufferPointer();
    sourceBuffer.Size = shaderSource->GetBufferSize();
    sourceBuffer.Encoding = DXC_CP_UTF8;

    std::vector<LPCWSTR> arguments;
    arguments.push_back(filePath.c_str());
    arguments.push_back(L"-E");
    arguments.push_back(L"main");
    arguments.push_back(L"-T");
    arguments.push_back(profile.c_str());
    arguments.push_back(L"-Zi");
    arguments.push_back(L"-Od");
    arguments.push_back(L"-Zpr");

    Microsoft::WRL::ComPtr<IDxcResult> shaderResult;
    dxcCompiler->Compile(&sourceBuffer, arguments.data(), (uint32_t)arguments.size(), includeHandler, IID_PPV_ARGS(&shaderResult));

    Microsoft::WRL::ComPtr<IDxcBlobUtf8> shaderError;
    shaderResult->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&shaderError), nullptr);
    if (shaderError != nullptr && shaderError->GetStringLength() != 0) {
        OutputDebugStringA(shaderError->GetStringPointer());
        assert(false);
    }

    Microsoft::WRL::ComPtr<IDxcBlob> shaderBlob;
    shaderResult->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&shaderBlob), nullptr);
    return shaderBlob;
}
}

GPUParticleManager* GPUParticleManager::GetInstance() {
    static GPUParticleManager instance;
    return &instance;
}

void GPUParticleManager::Initialize(DirectXCommon* dx) {
    dx_ = dx;
    CreateResources();
    CreateComputePipeline();
    CreateGraphicsPipeline();
    CreateQuad();

    // Run Initialization CS
    auto* cmdList = dx_->GetCommandList();
    
    // Transition to UAV
    D3D12_RESOURCE_BARRIER barrier{};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Transition.pResource = particleBuffer_.Get();
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COMMON;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    cmdList->ResourceBarrier(1, &barrier);

    ID3D12DescriptorHeap* heaps[] = { dx_->GetSRVHeap() };
    cmdList->SetDescriptorHeaps(1, heaps);

    cmdList->SetComputeRootSignature(computeRootSignature_.Get());
    cmdList->SetPipelineState(computePipelineState_.Get());
    
    auto& srvAlloc = dx_->GetSrvAllocator();
    cmdList->SetComputeRootDescriptorTable(0, srvAlloc.Gpu(uavIndex_));
    
    cmdList->Dispatch(1, 1, 1); // 1024 threads in 1 group

    // Resource Barrier (UAV -> SRV/Vertex)
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
    cmdList->ResourceBarrier(1, &barrier);
}

void GPUParticleManager::Update() {
    const Matrix4x4& view = Renderer::GetInstance()->GetViewMatrix();
    const Matrix4x4& proj = Renderer::GetInstance()->GetProjectionMatrix();

    perViewMapped_->viewProjection = Multiply(view, proj);
    
    Matrix4x4 billboard = Inverse(view);
    billboard.m[3][0] = billboard.m[3][1] = billboard.m[3][2] = 0.0f;
    perViewMapped_->billboardMatrix = billboard;
}

void GPUParticleManager::Draw() {
    auto* cmdList = dx_->GetCommandList();
    auto& srvAlloc = dx_->GetSrvAllocator();

    cmdList->SetGraphicsRootSignature(graphicsRootSignature_.Get());
    cmdList->SetPipelineState(graphicsPipelineState_.Get());

    cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    cmdList->IASetVertexBuffers(0, 1, &vbView_);
    cmdList->IASetIndexBuffer(&ibView_);

    ID3D12DescriptorHeap* heaps[] = { dx_->GetSRVHeap() };
    cmdList->SetDescriptorHeaps(1, heaps);

    // 0: CBV(b0)
    cmdList->SetGraphicsRootConstantBufferView(0, perViewCB_->GetGPUVirtualAddress());
    // 1: SRV(t0)
    cmdList->SetGraphicsRootDescriptorTable(1, srvAlloc.Gpu(srvIndex_));

    cmdList->DrawIndexedInstanced(6, kMaxParticles, 0, 0, 0);
}

void GPUParticleManager::CreateResources() {
    auto* renderer = Renderer::GetInstance();
    auto& srvAlloc = dx_->GetSrvAllocator();

    // Particle Buffer
    size_t bufferSize = sizeof(GPUParticle) * kMaxParticles;
    particleBuffer_ = renderer->CreateUAVBuffer(bufferSize);

    // SRV
    srvIndex_ = srvAlloc.Allocate();
    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
    srvDesc.Format = DXGI_FORMAT_UNKNOWN;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.Buffer.FirstElement = 0;
    srvDesc.Buffer.NumElements = kMaxParticles;
    srvDesc.Buffer.StructureByteStride = sizeof(GPUParticle);
    srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;
    dx_->GetDevice()->CreateShaderResourceView(particleBuffer_.Get(), &srvDesc, srvAlloc.Cpu(srvIndex_));

    // UAV
    uavIndex_ = srvAlloc.Allocate();
    D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc{};
    uavDesc.Format = DXGI_FORMAT_UNKNOWN;
    uavDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
    uavDesc.Buffer.FirstElement = 0;
    uavDesc.Buffer.NumElements = kMaxParticles;
    uavDesc.Buffer.StructureByteStride = sizeof(GPUParticle);
    uavDesc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_NONE;
    dx_->GetDevice()->CreateUnorderedAccessView(particleBuffer_.Get(), nullptr, &uavDesc, srvAlloc.Cpu(uavIndex_));

    // PerView CB
    perViewCB_ = renderer->CreateUploadBuffer(Align256_(sizeof(PerView)));
    perViewCB_->Map(0, nullptr, reinterpret_cast<void**>(&perViewMapped_));
}

void GPUParticleManager::CreateComputePipeline() {
    auto* device = dx_->GetDevice();

    // Root Signature
    D3D12_DESCRIPTOR_RANGE range{};
    range.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
    range.NumDescriptors = 1;
    range.BaseShaderRegister = 0;

    D3D12_ROOT_PARAMETER param{};
    param.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    param.DescriptorTable.NumDescriptorRanges = 1;
    param.DescriptorTable.pDescriptorRanges = &range;
    param.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

    D3D12_ROOT_SIGNATURE_DESC rsDesc{};
    rsDesc.NumParameters = 1;
    rsDesc.pParameters = &param;
    rsDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_NONE;

    ComPtr<ID3DBlob> blob, err;
    D3D12SerializeRootSignature(&rsDesc, D3D_ROOT_SIGNATURE_VERSION_1, &blob, &err);
    device->CreateRootSignature(0, blob->GetBufferPointer(), blob->GetBufferSize(), IID_PPV_ARGS(&computeRootSignature_));

    // PSO
    ComPtr<IDxcBlob> csBlob = CompileShader(L"resources/shaders/InitializeGPUParticle.CS.hlsl", L"cs_6_0", dx_->GetDXCUtils(), dx_->GetDXCCompiler(), dx_->GetDXCIncludeHandler());
    
    D3D12_COMPUTE_PIPELINE_STATE_DESC psoDesc{};
    psoDesc.pRootSignature = computeRootSignature_.Get();
    psoDesc.CS = { csBlob->GetBufferPointer(), csBlob->GetBufferSize() };
    device->CreateComputePipelineState(&psoDesc, IID_PPV_ARGS(&computePipelineState_));
}

void GPUParticleManager::CreateGraphicsPipeline() {
    auto* device = dx_->GetDevice();

    // Root Signature
    D3D12_DESCRIPTOR_RANGE srvRange{};
    srvRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    srvRange.NumDescriptors = 1;
    srvRange.BaseShaderRegister = 0;

    D3D12_ROOT_PARAMETER params[2]{};
    // 0: PerView (b0)
    params[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
    params[0].Descriptor.ShaderRegister = 0;
    params[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;
    // 1: Particles (t0)
    params[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    params[1].DescriptorTable.NumDescriptorRanges = 1;
    params[1].DescriptorTable.pDescriptorRanges = &srvRange;
    params[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;

    D3D12_ROOT_SIGNATURE_DESC rsDesc{};
    rsDesc.NumParameters = 2;
    rsDesc.pParameters = params;
    rsDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

    ComPtr<ID3DBlob> blob, err;
    D3D12SerializeRootSignature(&rsDesc, D3D_ROOT_SIGNATURE_VERSION_1, &blob, &err);
    device->CreateRootSignature(0, blob->GetBufferPointer(), blob->GetBufferSize(), IID_PPV_ARGS(&graphicsRootSignature_));

    // PSO
    ComPtr<IDxcBlob> vsBlob = CompileShader(L"resources/shaders/GPUParticle.VS.hlsl", L"vs_6_0", dx_->GetDXCUtils(), dx_->GetDXCCompiler(), dx_->GetDXCIncludeHandler());
    ComPtr<IDxcBlob> psBlob = CompileShader(L"resources/shaders/GPUParticle.PS.hlsl", L"ps_6_0", dx_->GetDXCUtils(), dx_->GetDXCCompiler(), dx_->GetDXCIncludeHandler());

    D3D12_INPUT_ELEMENT_DESC inputLayout[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
    };

    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc{};
    psoDesc.pRootSignature = graphicsRootSignature_.Get();
    psoDesc.VS = { vsBlob->GetBufferPointer(), vsBlob->GetBufferSize() };
    psoDesc.PS = { psBlob->GetBufferPointer(), psBlob->GetBufferSize() };
    psoDesc.InputLayout = { inputLayout, _countof(inputLayout) };
    psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
    
    // Alpha Blend
    psoDesc.BlendState.RenderTarget[0].BlendEnable = TRUE;
    psoDesc.BlendState.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
    psoDesc.BlendState.RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
    psoDesc.BlendState.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;

    psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
    psoDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO; // パーティクルなので書き込まない
    
    psoDesc.SampleMask = UINT_MAX;
    psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    psoDesc.NumRenderTargets = 1;
    psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
    psoDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
    psoDesc.SampleDesc.Count = 1;

    device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&graphicsPipelineState_));
}

void GPUParticleManager::CreateQuad() {
    auto* renderer = Renderer::GetInstance();
    
    struct Vtx { float px, py, pz; float u, v; };
    Vtx quad[4] = {
        {-0.5f,  0.5f, 0.0f, 0.0f, 0.0f},
        { 0.5f,  0.5f, 0.0f, 1.0f, 0.0f},
        {-0.5f, -0.5f, 0.0f, 0.0f, 1.0f},
        { 0.5f, -0.5f, 0.0f, 1.0f, 1.0f},
    };
    uint16_t idx[6] = { 0,1,2, 2,1,3 };

    vb_ = renderer->CreateUploadBuffer(sizeof(quad));
    void* vbMapped = nullptr;
    vb_->Map(0, nullptr, &vbMapped);
    std::memcpy(vbMapped, quad, sizeof(quad));
    vb_->Unmap(0, nullptr);

    vbView_.BufferLocation = vb_->GetGPUVirtualAddress();
    vbView_.StrideInBytes = sizeof(Vtx);
    vbView_.SizeInBytes = sizeof(quad);

    ib_ = renderer->CreateUploadBuffer(sizeof(idx));
    void* ibMapped = nullptr;
    ib_->Map(0, nullptr, &ibMapped);
    std::memcpy(ibMapped, idx, sizeof(idx));
    ib_->Unmap(0, nullptr);

    ibView_.BufferLocation = ib_->GetGPUVirtualAddress();
    ibView_.Format = DXGI_FORMAT_R16_UINT;
    ibView_.SizeInBytes = sizeof(idx);
}
