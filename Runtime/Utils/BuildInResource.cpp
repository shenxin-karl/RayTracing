#include "BuildInResource.h"
#include "RenderObject/Mesh.h"
#include "RenderObject/VertexSemantic.hpp"

static BuildInResource sInstance;

BuildInResource::BuildInResource() {
	_onCreateCallbackHandle = GlobalCallbacks::Get().onCreate.Register(this, &BuildInResource::OnCreate);
	_onDestroyCallbackHandle = GlobalCallbacks::Get().onDestroy.Register(this, &BuildInResource::OnDestroy);
}

void BuildInResource::OnCreate() {
	BuildSkyBoxCubeMesh();
}

void BuildInResource::OnDestroy() {
	_pSkyBoxCubeMesh = nullptr;
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
