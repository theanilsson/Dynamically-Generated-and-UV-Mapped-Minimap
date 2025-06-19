#pragma once
#include <DreamEngine\math\vector2.h>
#include <DreamEngine\graphics\Camera.h>
#include <DreamEngine\graphics\DepthBuffer.h>
#include <DreamEngine\graphics\RenderTarget.h>
#include <DreamEngine\graphics\sprite.h>
#include <DreamEngine\shaders\SpriteShader.h>
#include <DreamEngine\utilities\CountTimer.h>
#include <memory>
#include "Observer.h"
#include <vector>
#include <utility>

namespace DreamEngine
{
	class TextureResource;
}

class Minimap : public Observer
{
public:
	Minimap();
	Minimap(const Minimap& anOtherMinimap) = delete;
	Minimap& operator=(const Minimap& anOtherMinimap) = delete;
	Minimap(Minimap&& anOtherMinimap) = default;
	Minimap& operator=(Minimap&& anOtherMinimap) = default;
	~Minimap();

	void Update(float aDeltaTime);
	void UpdateGeometryRenderBoatPos(DE::Vector2f& aPos);
	void RefreshDynamicSprites(std::vector<std::pair<DE::Vector2f, float>> someEnemyHeatBlips, std::vector<std::pair<DE::Vector2f, float>> someLeviathanHeatBlips, std::vector<DE::Vector2f> someSurvivors, DE::Vector2f aKeyItemPosOrArrowDirection, bool aShowArrow);

	void Receive(const Message& aMsg) override;

	DE::DepthBuffer& GetDepthBuffer();
	DE::RenderTarget& GetGeometryTarget();
	DE::RenderTarget& GetCanvas();
	DreamEngine::TextureResource& GetColorTexture();
	DE::Camera& GetCamera();
	float& GetCurrentRadius();
	bool& GetCameraUsingMinimumRadius();
	bool& GetJustUpdatedCameraDimensions();
	void ResetJustUpdatedCameraDimensions();
	DE::Vector2f& GetSmoothstep();
	const bool IsRadarActive() const;

	void Render();

private:
	DE::Vector2f CalculateRelativeHUDPosition(DE::Vector2f& aPosition);

	std::unique_ptr<DE::Camera> myCamera;
	DE::DepthBuffer myDepthBuffer;
	DE::RenderTarget myGeometryTarget;
	DE::RenderTarget myCanvas;
	DreamEngine::TextureResource* myColorTexture = nullptr;
	DE::Vector2f mySmoothstepValues = DE::Vector2f(8.0f, -8.0f);
	DE::Vector2f myLastGeometryRenderBoatPos;
	DE::Vector2f myLastKnownBoatPos;
	DE::Vector2f myCameraDimensions;
	float myMinimumRadius = 100.0f;
	float myMaximumRadius = 200.0f;
	float myCurrentRadius = 100.0f;
	float myRadarVisibilityPercentScalar = 1.0f;
	bool myCameraUsingMinimumRadius = true;
	bool myJustUpdatedCameraDimensions = false;
	bool myShouldRender = false;

	DreamEngine::Sprite2DInstanceData myBackgroundInstance;
	DreamEngine::Sprite2DInstanceData myGeometryInstance;
	DreamEngine::Sprite2DInstanceData myArrowInstance;
	DreamEngine::Sprite2DInstanceData myBorderInstance;
	DreamEngine::Sprite2DInstanceData myRadarLineInstance;

	DE::Vector2f myBoatBaseSize;
	DE::Vector2f myKeyItemBaseSize;
	DE::Vector2f mySurvivorBaseSize;
	DE::Vector2f myHeatmapBlipBaseSize;

	DreamEngine::Sprite2DInstanceData myBoatInstance;
	DreamEngine::Sprite2DInstanceData myKeyItemInstance;
	std::vector<DreamEngine::Sprite2DInstanceData> mySurvivorInstances;
	std::vector<DreamEngine::Sprite2DInstanceData> myHeatmapBlipInstances;

	DreamEngine::SpriteSharedData* myBackgroundSprite;
	DreamEngine::SpriteSharedData myGeometrySprite;
	DreamEngine::SpriteSharedData* myBoatSprite;
	DreamEngine::SpriteSharedData* myArrowSprite;
	DreamEngine::SpriteSharedData* myKeyItemSprite;
	DreamEngine::SpriteSharedData* mySurvivorSprite;
	DreamEngine::SpriteSharedData* myBorderSprite;
	DreamEngine::SpriteSharedData* myHeatmapBlipSprite;
	DreamEngine::SpriteSharedData* myRadarLineSprite;

	std::unique_ptr<DreamEngine::SpriteShader> myDynamicUVMappingShader;
	CommonUtilities::CountdownTimer myRadarDurationTimer;
};