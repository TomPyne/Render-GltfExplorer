#pragma once

#include "SceneRendering.h"

#include <memory>

class ISceneNode
{
public:

	virtual void Update(float deltaSeconds) {}
	virtual void Draw(const RenderInfo& info, Renderables& renderables) const noexcept {}

	inline bool CanTick() const noexcept { return Ticking; }
	inline bool CanRender() const noexcept { return Visible; }

	const matrix& GetTransformLazyUpdate() noexcept;

	void SetParent(ISceneNode* parent);

protected:

	inline void SetCanTick(bool canTick) noexcept { Ticking = canTick; }
	inline void SetCanRender(bool canRender) noexcept { Visible = canRender; }	

	bool Ticking = false;
	bool Visible = false;

	AABB LocalBounds = {};
	AABB WorldBounds = {};
	matrix RelativeTransform = {};
	matrix AbsoluteTransform = {};
	bool TransformChanged = true;

	ISceneNode* ParentNode = nullptr;
	std::vector<ISceneNode*> ChildNodes;
};

using SceneNodePtr = std::shared_ptr<ISceneNode>;