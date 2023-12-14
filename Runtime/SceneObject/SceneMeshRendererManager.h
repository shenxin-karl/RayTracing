#pragma once
#include <vector>
#include "Foundation/NonCopyable.h"

class RenderObject;
class SceneMeshRendererManager : private NonCopyable {

private:
	std::vector<RenderObject *> _renderObjects; 
};