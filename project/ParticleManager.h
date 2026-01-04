#pragma once

#include "AABB.h"
#include "Matrix.h"
#include "SrvHandle.h"
#include "Transform.h"
#include "Vector.h"

#include <d3d12.h>
#include <wrl.h>

#include <cstdint>
#include <list>
#include <memory>
#include <string>
#include <unordered_map>

class DirectXCommon;
class TextureResource;
class UnifiedPipeline;

// 加速度フィールド（範囲内の粒子に加速度を与える）
struct AccelerationField {
    Vector3 acceleration;
    AABB area;
};

// パーティクル（CPU側）
struct Particle {
    Transform transform;
    Vector3 velocity;
    float lifetime = 1.0f;
    float age = 0.0f;
    Vector4 color{ 1,1,1,1 };
};

// GPUへ送るインスタンシングデータ
struct ParticleForGPU {
    Matrix4x4 WVP;
    Matrix4x4 World;
    Vector4 color;
};

// PS側 Material(b0)
struct ParticleMaterialData {
    Vector4 color;
    int32_t enableLighting;
    float pad[3];
    Matrix4x4 uvTransform;
};

// パーティクルシステム全体を管理する（SRV/StructuredBuffer/描画/更新）
// - グループ単位（テクスチャ単位）でインスタンシング描画
// - Emit は外部（Emitter等）から呼ぶ
class ParticleManager {
public:
    template <class T> using ComPtr = Microsoft::WRL::ComPtr<T>;

    static ParticleManager* GetInstance();

    // DX関連を受け取って初期化
    bool Initialize(DirectXCommon* dx);
    void Finalize();

    // テクスチャ単位のグループを作成
    // - name: グループ名（ユーザーが付けるキー）
    // - texturePath: TextureManagerで読むファイルパス
    // - maxInstances: StructuredBuffer の最大要素数
    bool CreateParticleGroup(const std::string& name, const std::string& texturePath, uint32_t maxInstances);

    // パーティクルをグループに発生させる（Emitterから呼ぶ想定）
    void Emit(const std::string& name, const Vector3& position, const Vector3& velocity,
        const Vector3& scale, float lifetime, const Vector4& color);

    // Update: 粒子更新 + インスタンスデータ書き込み
    void Update(const Matrix4x4& view, const Matrix4x4& proj, float deltaTime);

    // Draw: グループごとに DrawIndexedInstanced
    // - pipeline: Particle用パイプライン（blendごとに差し替え可能）
    void Draw(ID3D12GraphicsCommandList* cmdList, UnifiedPipeline* pipeline);

    // パラメータ
    void SetAccelerationField(const AccelerationField& field) { accelerationField_ = field; }
    void SetEnableAccelerationField(bool enable) { enableAccelerationField_ = enable; }

    // グループの最大インスタンス（UIで減らす等に使う）
    void SetGroupInstanceLimit(const std::string& name, uint32_t limit);

	// グループ内の全粒子をクリア
    void ClearParticleGroup(const std::string& name);

private:
    ParticleManager() = default;
    ParticleManager(const ParticleManager&) = delete;
    ParticleManager& operator=(const ParticleManager&) = delete;

    Matrix4x4 MakeBillboardMatrix_(const Vector3& scale, const Vector3& translate, const Matrix4x4& viewMatrix) const;
    bool IsCollision_(const AABB& aabb, const Vector3& point) const;

    void EnsureQuadGeometry_();

private:
    struct ParticleGroup {
        std::shared_ptr<TextureResource> texture;
        D3D12_GPU_DESCRIPTOR_HANDLE textureSrvGpu{};

        std::list<Particle> particles;

        uint32_t maxInstances = 0;
        uint32_t instanceLimit = 0;
        uint32_t activeInstanceCount = 0;

        // StructuredBuffer (t1)
        ComPtr<ID3D12Resource> instanceBuffer;
        ParticleForGPU* instanceMapped = nullptr;
        SrvHandle instanceSrv;
        D3D12_GPU_DESCRIPTOR_HANDLE instanceSrvGpu{};

        // Material (b0)
        ComPtr<ID3D12Resource> materialCB;
        ParticleMaterialData* materialMapped = nullptr;
    };

    DirectXCommon* dx_ = nullptr;
    ComPtr<ID3D12Device> device_;

    std::unordered_map<std::string, ParticleGroup> groups_;

    // 共通の板ポリ
    D3D12_VERTEX_BUFFER_VIEW vbView_{};
    D3D12_INDEX_BUFFER_VIEW ibView_{};
    ComPtr<ID3D12Resource> vb_;
    ComPtr<ID3D12Resource> ib_;
    bool quadReady_ = false;

    AccelerationField accelerationField_{};
    bool enableAccelerationField_ = false;
};
