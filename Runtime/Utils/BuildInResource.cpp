#include "BuildInResource.h"

#include "AssetProjectSetting.h"
#include "D3d12/RootSignature.h"
#include "Renderer/GfxDevice.h"
#include "RenderObject/Mesh.h"
#include "RenderObject/VertexSemantic.hpp"
#include "TextureObject/TextureLoader.h"

static BuildInResource sInstance;

BuildInResource::BuildInResource() {
    _onCreateCallbackHandle = GlobalCallbacks::Get().OnCreate.Register(this, &BuildInResource::OnCreate);
    _onDestroyCallbackHandle = GlobalCallbacks::Get().OnDestroy.Register(this, &BuildInResource::OnDestroy);
}

void BuildInResource::OnCreate() {
    BuildSkyBoxCubeMesh();
    BuildCubeMesh();
    LoadWhiteTex();
#if ENABLE_RAY_TRACING
    _pEmptyLocalRootSignature = dx::RootSignature::Create(0);
    _pEmptyLocalRootSignature->Generate(GfxDevice::GetInstance()->GetDevice(),
        D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE);
#endif

}

void BuildInResource::OnDestroy() {
    _pSkyBoxCubeMesh = nullptr;
    _pCubeMesh = nullptr;
    _pEmptyLocalRootSignature = nullptr;
    _pWhiteTex = nullptr;
    _whiteTexSRV.Release();
}

auto BuildInResource::Get() -> BuildInResource & {
    return sInstance;
}

void BuildInResource::BuildSkyBoxCubeMesh() {
    _pSkyBoxCubeMesh = std::make_shared<Mesh>();
    _pSkyBoxCubeMesh->SetName("BuildInResource::SkyBoxCubeMesh");
    _pSkyBoxCubeMesh->Resize(SemanticMask::eVertex, 36, 0);

    // clang-format off
	glm::vec3 skyBoxVertices[] = {
		{ -1.0f,+1.0f,-1.0f }, { -1.0f,-1.0f,-1.0f }, { +1.0f,-1.0f,-1.0f },
		{ +1.0f,-1.0f,-1.0f }, { +1.0f,+1.0f,-1.0f }, { -1.0f,+1.0f,-1.0f },
		{ -1.0f,-1.0f,+1.0f }, { -1.0f,-1.0f,-1.0f }, { -1.0f,+1.0f,-1.0f },
		{ -1.0f,+1.0f,-1.0f }, { -1.0f,+1.0f,+1.0f }, { -1.0f,-1.0f,+1.0f },
		{ +1.0f,-1.0f,-1.0f }, { +1.0f,-1.0f,+1.0f }, { +1.0f,+1.0f,+1.0f },
		{ +1.0f,+1.0f,+1.0f }, { +1.0f,+1.0f,-1.0f }, { +1.0f,-1.0f,-1.0f },
		{ -1.0f,-1.0f,+1.0f }, { -1.0f,+1.0f,+1.0f }, { +1.0f,+1.0f,+1.0f },
		{ +1.0f,+1.0f,+1.0f }, { +1.0f,-1.0f,+1.0f }, { -1.0f,-1.0f,+1.0f },
		{ -1.0f,+1.0f,-1.0f }, { +1.0f,+1.0f,-1.0f }, { +1.0f,+1.0f,+1.0f },
		{ +1.0f,+1.0f,+1.0f }, { -1.0f,+1.0f,+1.0f }, { -1.0f,+1.0f,-1.0f },
		{ -1.0f,-1.0f,-1.0f }, { -1.0f,-1.0f,+1.0f }, { +1.0f,-1.0f,-1.0f },
		{ +1.0f,-1.0f,-1.0f }, { -1.0f,-1.0f,+1.0f }, { +1.0f,-1.0f,+1.0f },
	};
    // clang-format on

    _pSkyBoxCubeMesh->SetVertices(skyBoxVertices);
    _pSkyBoxCubeMesh->UploadMeshData();
}

static void GenerateTangentAndNormal(const std::vector<glm::vec3> &vertices,
    const std::vector<glm::vec2> &uv0,
    const std::vector<uint16_t> &indices,
    std::vector<glm::vec3> &outNormals,
    std::vector<glm::vec4> &outTangents) {
    if (indices.size() < 3) {
        return;
    }

    std::vector<glm::vec3> normals(vertices.size(), glm::vec3(0));
    std::vector<glm::vec3> tangents(vertices.size(), glm::vec3(0));
    std::vector<glm::vec3> bitangents(vertices.size(), glm::vec3(0));
    for (size_t i = 0; i < indices.size() - 2; i += 3) {
        size_t idx0 = indices[i + 0];
        size_t idx1 = indices[i + 1];
        size_t idx2 = indices[i + 2];
        const glm::vec3 &v0 = vertices[idx0];
        const glm::vec3 &v1 = vertices[idx1];
        const glm::vec3 &v2 = vertices[idx2];
        const glm::vec2 &texcoord0 = uv0[idx0];
        const glm::vec2 &texcoord1 = uv0[idx1];
        const glm::vec2 &texcoord2 = uv0[idx2];
        glm::vec3 E1 = v1 - v0;
        glm::vec3 E2 = v2 - v0;
        float t1 = texcoord1.y - texcoord0.y;
        float t2 = texcoord2.y - texcoord0.y;
        float u0 = texcoord1.x - texcoord0.x;
        float u1 = texcoord2.x - texcoord0.x;
        glm::vec3 normal = cross(E1, E2);
        glm::vec3 tangent = (t2 * E1) - (t1 * E2);
        glm::vec3 binormal = (-u1 * E1) + (u0 * E2);
        for (size_t j = i; j < i + 3; ++j) {
            size_t index = indices[j];
            normals[index] += normal;
            tangents[index] += tangent;
            bitangents[index] += binormal;
        }
    }

    outNormals.clear();
    outTangents.clear();
    outNormals.reserve(vertices.size());
    outTangents.reserve(vertices.size());
    for (size_t i = 0; i < tangents.size(); ++i) {
        glm::vec3 n = normalize(normals[i]);
        glm::vec3 t = tangents[i];
        t -= n * dot(n, t);    // Õý½»ÐÞÕý
        t = normalize(t);

        float det = 1.f;
        if (dot(cross(n, t), bitangents[i]) < 0.f)
            det = -1.f;

        outNormals.emplace_back(n);
        outTangents.emplace_back(t, det);
    }
}

void BuildInResource::BuildCubeMesh() {
    constexpr float x = 0.5f;
    constexpr float y = 0.5f;
    constexpr float z = 0.5f;

    // clang-format off
    std::vector<glm::vec3> vertices = {
        glm::vec3{ -x, -y, -z }, glm::vec3{ -x, +y, -z }, glm::vec3{ +x, +y, -z },
        glm::vec3{ +x, -y, -z }, glm::vec3{ +x, -y, -z }, glm::vec3{ +x, +y, -z },
        glm::vec3{ +x, +y, +z }, glm::vec3{ +x, -y, +z }, glm::vec3{ +x, -y, +z },
        glm::vec3{ +x, +y, +z }, glm::vec3{ -x, +y, +z }, glm::vec3{ -x, -y, +z },
        glm::vec3{ -x, -y, +z }, glm::vec3{ -x, +y, +z }, glm::vec3{ -x, +y, -z },
        glm::vec3{ -x, -y, -z }, glm::vec3{ -x, +y, -z }, glm::vec3{ -x, +y, +z },
        glm::vec3{ +x, +y, +z }, glm::vec3{ +x, +y, -z }, glm::vec3{ -x, -y, +z },
        glm::vec3{ -x, -y, -z }, glm::vec3{ +x, -y, -z }, glm::vec3{ +x, -y, +z },
    };
	std::vector<glm::vec2> uv0 = {
		glm::vec2{ 0.0f, 1.0f }, glm::vec2{ 0.0f, 0.0f }, glm::vec2{ 1.0f, 0.0f },
		glm::vec2{ 1.0f, 1.0f }, glm::vec2{ 0.0f, 1.0f }, glm::vec2{ 0.0f, 0.0f },
		glm::vec2{ 1.0f, 0.0f }, glm::vec2{ 1.0f, 1.0f }, glm::vec2{ 0.0f, 1.0f },
		glm::vec2{ 0.0f, 0.0f }, glm::vec2{ 1.0f, 0.0f }, glm::vec2{ 1.0f, 1.0f },
		glm::vec2{ 0.0f, 1.0f }, glm::vec2{ 0.0f, 0.0f }, glm::vec2{ 1.0f, 0.0f },
		glm::vec2{ 1.0f, 1.0f }, glm::vec2{ 0.0f, 1.0f }, glm::vec2{ 0.0f, 0.0f },
		glm::vec2{ 1.0f, 0.0f }, glm::vec2{ 1.0f, 1.0f }, glm::vec2{ 0.0f, 1.0f },
		glm::vec2{ 0.0f, 0.0f }, glm::vec2{ 1.0f, 0.0f }, glm::vec2{ 1.0f, 1.0f },
	};

	std::vector<uint16_t> indices = {
		0, 1, 2,
		0, 2, 3,
		4, 5, 6,
		4, 6, 7,
		8, 9,  10,
		8, 10, 11,
		12, 13, 14,
		12, 14, 15,
		16, 17, 18,
		16, 18, 19,
		20, 21, 22,
		20, 22, 23,
	};
    // clang-format on

    std::vector<glm::vec3> normals;
    std::vector<glm::vec4> tangents;
    GenerateTangentAndNormal(vertices, uv0, indices, normals, tangents);

    SemanticMask vertexMask = SemanticMask::eVertex | SemanticMask::eNormal | SemanticMask::eTangent |
                              SemanticMask::eTexCoord0;
    _pCubeMesh = std::make_shared<Mesh>();
    _pCubeMesh->SetName("BuildInResource::CubeMesh");
    _pCubeMesh->Resize(vertexMask, vertices.size(), indices.size());
    _pCubeMesh->SetVertices(vertices);
    _pCubeMesh->SetNormals(normals);
    _pCubeMesh->SetTangents(tangents);
    _pCubeMesh->SetUV0(uv0);
    _pCubeMesh->SetIndices(indices);
    _pCubeMesh->UploadMeshData();
}

void BuildInResource::LoadWhiteTex() {
    TextureLoader textureLoader;
    stdfs::path path = AssetProjectSetting::ToAssetPath("Textures/white.DDS");
    _pWhiteTex = textureLoader.LoadFromFile(path, true);
    _whiteTexSRV = textureLoader.GetSRV2D(_pWhiteTex.Get());
}
