#pragma once
#include <DreamEngine\math\vector2.h>
#include <DreamEngine\graphics\Camera.h>
#include <DreamEngine\graphics\DepthBuffer.h>
#include <DreamEngine\graphics\RenderTarget.h>
#include <DreamEngine\graphics\sprite.h>
#include <DreamEngine\shaders\SpriteShader.h>
#include <memory>
#include "Observer.h"

class WorldGrid;

class Minimap : public Observer
{
public:
	Minimap() = delete;
	Minimap(WorldGrid* aWorldGrid);
	Minimap(const Minimap& anOtherMinimap) = delete;
	Minimap& operator=(const Minimap& anOtherMinimap) = delete;
	Minimap(Minimap&& anOtherMinimap) = default;
	Minimap& operator=(Minimap&& anOtherMinimap) = default;
	~Minimap();

	void Update();

	void Receive(const Message& aMsg) override;

	DE::DepthBuffer& GetDepthBuffer();
	DE::RenderTarget& GetGeometryTarget();
	DE::RenderTarget& GetCanvas();
	DE::Camera& GetCamera();
	float& GetRadius();
	DE::Vector2f& GetSmoothstep();
	void Render();

private:
	std::unique_ptr<DE::Camera> myCamera;
	DE::DepthBuffer myDepthBuffer;
	DE::RenderTarget myGeometryTarget;
	DE::RenderTarget myCanvas;
	DE::Vector2f mySmoothstepValues = DE::Vector2f(4.0f, -3.0f);
	DE::Vector2f myLastKnownBoatPos;
	DE::Vector2f myWorldDimensions;
	DE::Vector2f myWorldCenterPoint;
	float myRadius = 50.0f;

	DreamEngine::Sprite2DInstanceData myBackgroundInstance;
	DreamEngine::Sprite2DInstanceData myGeometryInstance;
	DreamEngine::Sprite2DInstanceData myBoatInstance;
	DreamEngine::Sprite2DInstanceData myArrowInstance;
	DreamEngine::Sprite2DInstanceData myKeyItemInstance;
	DreamEngine::Sprite2DInstanceData myBorderInstance;
	std::array<DreamEngine::Sprite2DInstanceData, 5> mySurvivorInstances;
	std::vector<DreamEngine::Sprite2DInstanceData> myHeatmapDotInstances;

	DreamEngine::SpriteSharedData* myBackgroundSprite;
	DreamEngine::SpriteSharedData myGeometrySprite;
	DreamEngine::SpriteSharedData* myBoatSprite;
	DreamEngine::SpriteSharedData* myArrowSprite;
	DreamEngine::SpriteSharedData* myKeyItemSprite;
	DreamEngine::SpriteSharedData* mySurvivorSprite;
	DreamEngine::SpriteSharedData* myBorderSprite;
	DreamEngine::SpriteSharedData* myHeatmapDotSprite;

	std::unique_ptr<DreamEngine::SpriteShader> myShader;

	bool myShouldRender = false;
};