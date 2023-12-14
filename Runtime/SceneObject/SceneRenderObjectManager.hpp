#pragma once
#include <vector>
#include "Foundation/NonCopyable.h"

class RenderObject;
class SceneRenderObjectManager : private NonCopyable {
public:
	void AddRenderObject(RenderObject *pRenderObject) {
		_renderObjects.push_back(pRenderObject);
	}
	void Reset() {
		_renderObjects.clear();
	}
private:
	std::vector<RenderObject *> _renderObjects; 
};