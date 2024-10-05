#include "ISceneNode.h"
#include <algorithm>

const matrix& ISceneNode::GetTransformLazyUpdate() noexcept
{
    if (TransformChanged)
    {
        if (ParentNode)
        {
            AbsoluteTransform = ParentNode->GetTransformLazyUpdate() * RelativeTransform;
        }
        else
        {
            AbsoluteTransform = RelativeTransform;
        }

        WorldBounds = LocalBounds;
        WorldBounds.Transform(AbsoluteTransform);

        TransformChanged = false;
    }

    return AbsoluteTransform;
}

void ISceneNode::SetParent(ISceneNode* parent)
{
    if (ParentNode)
    {
        ParentNode->ChildNodes.erase(
            std::remove_if(
                ParentNode->ChildNodes.begin(),
                ParentNode->ChildNodes.end(),
                [this](ISceneNode* other) { return other == this; }
            ),
            ParentNode->ChildNodes.end()
        );
    }

    ParentNode = parent;

    if (ParentNode)
    {
        ParentNode->ChildNodes.push_back(this);
    }
}
