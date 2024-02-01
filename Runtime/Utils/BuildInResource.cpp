#include "BuildInResource.h"
#include "D3d12/RootSignature.h"
#include "Renderer/GfxDevice.h"
#include "RenderObject/Mesh.h"
#include "RenderObject/VertexSemantic.hpp"

static BuildInResource sInstance;

BuildInResource::BuildInResource() {
	_onCreateCallbackHandle = GlobalCallbacks::Get().OnCreate.Register(this, &BuildInResource::OnCreate);
	_onDestroyCallbackHandle = GlobalCallbacks::Get().OnDestroy.Register(this, &BuildInResource::OnDestroy);
}

void BuildInResource::OnCreate() {
	BuildSkyBoxCubeMesh();
	BuildCubeMesh();
#if ENABLE_RAY_TRACING
	_pEmptyLocalRootSignature = std::make_shared<dx::RootSignature>();
	_pEmptyLocalRootSignature->OnCreate(0);
	_pEmptyLocalRootSignature->Generate(GfxDevice::GetInstance()->GetDevice(), D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE);
#endif
}

void BuildInResource::OnDestroy() {
	_pSkyBoxCubeMesh = nullptr;
	_pEmptyLocalRootSignature = nullptr;
}

auto BuildInResource::Get() -> BuildInResource & {
	return sInstance;
}

void BuildInResource::BuildSkyBoxCubeMesh() {
	_pSkyBoxCubeMesh = std::make_shared<Mesh>();
	_pSkyBoxCubeMesh->SetName("BuildInResource::SkyBoxCubeMesh");
	_pSkyBoxCubeMesh->Resize(SemanticMask::eVertex, 36, 0);

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

	_pSkyBoxCubeMesh->SetVertices(skyBoxVertices);
	_pSkyBoxCubeMesh->UploadMeshData();
}

void BuildInResource::BuildCubeMesh() {

	struct CubeVertex {
		glm::vec3 position;
		glm::vec3 normal;
		glm::vec4 tangent;
		glm::vec2 uv0;
	};

	[[maybe_unused]] CubeVertex vertices[] = {

	};

	_pCubeMesh = std::make_shared<Mesh>();
	_pCubeMesh->SetName("BuildInResource::CubeMesh");

}
