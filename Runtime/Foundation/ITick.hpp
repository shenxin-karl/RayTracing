#pragma once
#include "Foundation/NonCopyable.h"

class GameTimer;
class ITick : public NonCopyable {
public:
	// logic tick
	virtual void OnPreUpdate(GameTimer &timer) {}
	virtual void OnUpdate(GameTimer &timer) {}
	virtual void OnPostUpdate(GameTimer &timer) {}

	// render tick
	virtual void OnPreRender(GameTimer &timer) {}
	virtual void OnRender(GameTimer &timer) {}
	virtual void OnPostRender(GameTimer &timer) {}

	virtual ~ITick() = default;
};