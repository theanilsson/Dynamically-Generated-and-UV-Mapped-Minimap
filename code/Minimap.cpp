#include "Minimap.h"
#include <DreamEngine\engine.h>
#include <DreamEngine\graphics\GraphicsEngine.h>
#include <DreamEngine\graphics\GraphicsStateStack.h>
#include <DreamEngine\graphics\TextureManager.h>
#include <DreamEngine\graphics\SpriteDrawer.h>
#include <DreamEngine\utilities\UtilityFunctions.h>
#include <imgui\imgui.h>
#include "MainSingleton.h"

Minimap::Minimap()
{
	myCamera = std::make_unique<DE::Camera>();
	myCamera->SetOrtographicProjection(myMinimumRadius * 210.0f, myMinimumRadius * -210.0f, myMinimumRadius * 210.0f, myMinimumRadius * -210.0f, 1.0f, 50000.0f);
	myCamera->GetTransform().SetRotation(DE::Rotator(90.0f, 0.0f, 0.0f));
	myCameraDimensions = DE::Vector2f(myMinimumRadius * 4.2f);

	// The map looks fine at 1024 resolution, but we have plenty of room in the render budget to run it at 2048 for extra detail
	// We can run it at even lower resolution without losing detail if we render the static geometry more often
	DE::Vector2ui textureResolution = DE::Vector2ui(2048, 2048);
	myDepthBuffer = DE::DepthBuffer::Create(textureResolution);
	myGeometryTarget = DE::RenderTarget::Create(textureResolution, DXGI_FORMAT_R32G32B32A32_FLOAT);
	myCanvas = DE::RenderTarget::Create(textureResolution);

	DE::Engine& engine = *DreamEngine::Engine::GetInstance();
	myColorTexture = engine.GetTextureManager().GetTexture(L"2D/S_UI_MinimapTexture.png");
	myDynamicUVMappingShader = std::make_unique<DE::SpriteShader>();
	myDynamicUVMappingShader->Init(L"shaders/instanced_sprite_shader_VS.cso", L"shaders/MinimapSpritePS.cso");
	myGeometrySprite.myTexture = &myCanvas;
	myGeometrySprite.myCustomShader = myDynamicUVMappingShader.get();

	MainSingleton::GetInstance()->GetPostMaster().Subscribe(eMessageType::ShowHud, this);
	MainSingleton::GetInstance()->GetPostMaster().Subscribe(eMessageType::BoatTransformUpdate, this);
	MainSingleton::GetInstance()->GetPostMaster().Subscribe(eMessageType::StartRadarPing, this);

	myRadarDurationTimer.SetResetValue(10.0f);

	DreamEngine::Vector2f resolution((float)engine.GetRenderSize().x, (float)engine.GetRenderSize().y);

	DE::Vector2f minimapCenterPosition(0.9225f * resolution.x, 0.865f * resolution.y);
	myBackgroundInstance.myPosition = minimapCenterPosition;
	myGeometryInstance.myPosition = minimapCenterPosition;
	myBoatInstance.myPosition = minimapCenterPosition;
	myBorderInstance.myPosition = minimapCenterPosition;
	myRadarLineInstance.myPosition = minimapCenterPosition;

	myBackgroundInstance.mySize = DE::Vector2f(0.135f * resolution.x, 0.25f * resolution.y);
	myGeometryInstance.mySize = DE::Vector2f(0.1296f * resolution.x, 0.24f * resolution.y);
	myBorderInstance.mySize = DE::Vector2f(0.1323f * resolution.x, 0.245f * resolution.y);
	myRadarLineInstance.mySize = DE::Vector2f(0.135f * resolution.x, 0.25f * resolution.y);
	myArrowInstance.mySize = DE::Vector2f(0.015f * resolution.x, 0.03f * resolution.y);
	myBoatBaseSize = DE::Vector2f(0.035f, 0.125f);
	myKeyItemBaseSize = DE::Vector2f(0.057f, 0.1f);
	mySurvivorBaseSize = DE::Vector2f(0.075f, 0.1f);
	myHeatmapBlipBaseSize = DE::Vector2f(0.057f, 0.1f);

	float radiusScalar = UtilityFunctions::Lerp(0.2f, 0.1f, (myCurrentRadius - myMinimumRadius) / myMinimumRadius);
	myBoatInstance.mySize = myBoatBaseSize * resolution * radiusScalar;
	myArrowInstance.myIsHidden = true;
	myKeyItemInstance.myIsHidden = true;

	auto& sprites = MainSingleton::GetInstance()->GetMinimapSprites();
	myBackgroundSprite = &sprites[0];
	myBoatSprite = &sprites[1];
	myArrowSprite = &sprites[2];
	myKeyItemSprite = &sprites[3];
	mySurvivorSprite = &sprites[4];
	myBorderSprite = &sprites[5];
	myHeatmapBlipSprite = &sprites[6];
	myRadarLineSprite = &sprites[7];
}

Minimap::~Minimap()
{
	MainSingleton::GetInstance()->GetPostMaster().Unsubscribe(eMessageType::ShowHud, this);
	MainSingleton::GetInstance()->GetPostMaster().Unsubscribe(eMessageType::BoatTransformUpdate, this);
	MainSingleton::GetInstance()->GetPostMaster().Unsubscribe(eMessageType::StartRadarPing, this);
}

void Minimap::Update(float aDeltaTime)
{
	if (!myShouldRender) return;

	DE::Engine& engine = *DreamEngine::Engine::GetInstance();
	DreamEngine::Vector2f resolution((float)engine.GetRenderSize().x, (float)engine.GetRenderSize().y);

	float radiusScalar = UtilityFunctions::Lerp(0.2f, 0.1f, (myCurrentRadius - myMinimumRadius) / myMinimumRadius);
	myBoatInstance.mySize = myBoatBaseSize * resolution * radiusScalar;

	if (!myRadarDurationTimer.IsDone())
	{
		myRadarDurationTimer.Update(aDeltaTime);
		if (myRadarDurationTimer.IsDone())
		{
			myRadarVisibilityPercentScalar = 0.0f;
		}
		else if (myRadarDurationTimer.GetCurrentValue() > myRadarDurationTimer.GetResetValue() * 0.5f)
		{
			myRadarVisibilityPercentScalar = UtilityFunctions::Lerp(2.0f, 0.0f, myRadarDurationTimer.GetCurrentValue() / myRadarDurationTimer.GetResetValue());
		}
		else
		{
			myRadarVisibilityPercentScalar = UtilityFunctions::Lerp(0.0f, 2.0f, myRadarDurationTimer.GetCurrentValue() / myRadarDurationTimer.GetResetValue());
		}
	}
	myRadarLineInstance.myRotation += UtilityFunctions::ourPiHalf * 1.5f * aDeltaTime;
	if (myRadarLineInstance.myRotation > UtilityFunctions::ourPiHalf * 4.0f)
		myRadarLineInstance.myRotation -= UtilityFunctions::ourPiHalf * 4.0f;
}

void Minimap::UpdateGeometryRenderBoatPos(DE::Vector2f& aPos)
{
	myLastGeometryRenderBoatPos = aPos;
}

void Minimap::RefreshDynamicSprites(std::vector<std::pair<DE::Vector2f, float>> someEnemyHeatBlips, std::vector<std::pair<DE::Vector2f, float>> someLeviathanHeatBlips, std::vector<DE::Vector2f> someSurvivors, DE::Vector2f aKeyItemPosOrArrowDirection, bool aShowArrow)
{
	float spriteResizeScalar = UtilityFunctions::Clamp((myCurrentRadius - myMinimumRadius) / myMinimumRadius, 0.0f, 1.0f) + 1.0f;
	float radiusScalar = UtilityFunctions::Lerp(0.2f, 0.1f, (myCurrentRadius - myMinimumRadius) / myMinimumRadius);
	DE::Engine& engine = *DreamEngine::Engine::GetInstance();
	DreamEngine::Vector2f resolution = { (float)engine.GetRenderSize().x, (float)engine.GetRenderSize().y };

	if (aShowArrow)
	{
		myKeyItemInstance.myIsHidden = true;
		myArrowInstance.myIsHidden = false;
		myArrowInstance.myPosition = myBorderInstance.myPosition + aKeyItemPosOrArrowDirection * myBorderInstance.mySize.x * 0.5f;
		myArrowInstance.myRotation = UtilityFunctions::CalculateRadianYRotationBetweenDirections(aKeyItemPosOrArrowDirection.x, aKeyItemPosOrArrowDirection.y, 1.0f, 0.0f) - UtilityFunctions::ourPiHalf;
	}
	else
	{
		myKeyItemInstance.myIsHidden = false;
		myArrowInstance.myIsHidden = true;
		myKeyItemInstance.myPosition = CalculateRelativeHUDPosition(aKeyItemPosOrArrowDirection);
		myKeyItemInstance.mySize = myKeyItemBaseSize * spriteResizeScalar * resolution * radiusScalar;
		myKeyItemInstance.myColor.myA = myRadarVisibilityPercentScalar;
	}

	mySurvivorInstances.clear();
	mySurvivorInstances.resize(someSurvivors.size());
	for (size_t i = 0; i < mySurvivorInstances.size(); i++)
	{
		mySurvivorInstances[i].myPosition = CalculateRelativeHUDPosition(someSurvivors[i]);
		mySurvivorInstances[i].mySize = mySurvivorBaseSize * spriteResizeScalar * resolution * radiusScalar;
		mySurvivorInstances[i].myColor.myA = myRadarVisibilityPercentScalar;
	}

	myHeatmapBlipInstances.clear();
	myHeatmapBlipInstances.resize(someEnemyHeatBlips.size() + someLeviathanHeatBlips.size());
	for (size_t i = 0; i < someEnemyHeatBlips.size(); i++)
	{
		myHeatmapBlipInstances[i].myPosition = CalculateRelativeHUDPosition(someEnemyHeatBlips[i].first);
		myHeatmapBlipInstances[i].mySizeMultiplier.x = someEnemyHeatBlips[i].second;
		myHeatmapBlipInstances[i].mySizeMultiplier.y = someEnemyHeatBlips[i].second;
		myHeatmapBlipInstances[i].mySize = myHeatmapBlipBaseSize * resolution * radiusScalar;
		myHeatmapBlipInstances[i].myColor.myA = myRadarVisibilityPercentScalar * someEnemyHeatBlips[i].second;
	}
	for (size_t i = 0; i < someLeviathanHeatBlips.size(); i++)
	{
		myHeatmapBlipInstances[someEnemyHeatBlips.size() + i].myPosition = CalculateRelativeHUDPosition(someLeviathanHeatBlips[i].first);
		myHeatmapBlipInstances[someEnemyHeatBlips.size() + i].mySizeMultiplier.x = someLeviathanHeatBlips[i].second;
		myHeatmapBlipInstances[someEnemyHeatBlips.size() + i].mySizeMultiplier.y = someLeviathanHeatBlips[i].second;
		myHeatmapBlipInstances[someEnemyHeatBlips.size() + i].mySize = myHeatmapBlipBaseSize * resolution * radiusScalar;
		myHeatmapBlipInstances[someEnemyHeatBlips.size() + i].myColor.myA = myRadarVisibilityPercentScalar;
	}

	myRadarLineInstance.myColor.myA = myRadarVisibilityPercentScalar;
}

void Minimap::Receive(const Message& aMsg)
{
	switch (aMsg.messageType)
	{
		case eMessageType::ShowHud:
		{
			myShouldRender = *static_cast<bool*>(aMsg.messageData);
			break;
		}
		case eMessageType::BoatTransformUpdate:
		{
			DE::Vector4f* boatData = static_cast<DE::Vector4f*>(aMsg.messageData);
			myBoatInstance.myRotation = boatData->y * UtilityFunctions::ourDegToRad + UtilityFunctions::ourPi;
			myLastKnownBoatPos = DE::Vector2f(boatData->x, boatData->z) / 100.0f;
			float roundedLerpScalar = std::round(boatData->w * 1000.0f) / 1000.0f;
			myCurrentRadius = UtilityFunctions::Lerp(myMinimumRadius, myMaximumRadius, roundedLerpScalar);
			if (!myCameraUsingMinimumRadius && myCurrentRadius == myMinimumRadius)
			{
				myCameraUsingMinimumRadius = true;
				myJustUpdatedCameraDimensions = true;
				myCamera->SetOrtographicProjection(myMinimumRadius * 210.0f, myMinimumRadius * -210.0f, myMinimumRadius * 210.0f, myMinimumRadius * -210.0f, 1.0f, 50000.0f);
				myCameraDimensions = DE::Vector2f(myMinimumRadius * 4.2f);
			}
			else if (myCameraUsingMinimumRadius && myCurrentRadius != myMinimumRadius)
			{
				myCameraUsingMinimumRadius = false;
				myJustUpdatedCameraDimensions = true;
				myCamera->SetOrtographicProjection(myMaximumRadius * 210.0f, myMaximumRadius * -210.0f, myMaximumRadius * 210.0f, myMaximumRadius * -210.0f, 1.0f, 50000.0f);
				myCameraDimensions = DE::Vector2f(myMaximumRadius * 4.2f);
			}
			break;
		}
		case eMessageType::StartRadarPing:
		{
			myRadarDurationTimer.Reset();
			myRadarVisibilityPercentScalar = 0.0f;
			break;
		}
	}
}

DE::DepthBuffer& Minimap::GetDepthBuffer()
{
	return myDepthBuffer;
}

DE::RenderTarget& Minimap::GetGeometryTarget()
{
	return myGeometryTarget;
}

DE::RenderTarget& Minimap::GetCanvas()
{
	return myCanvas;
}

DreamEngine::TextureResource& Minimap::GetColorTexture()
{
	return *myColorTexture;
}

DE::Camera& Minimap::GetCamera()
{
	return *myCamera;
}

float& Minimap::GetCurrentRadius()
{
	return myCurrentRadius;
}

bool& Minimap::GetCameraUsingMinimumRadius()
{
	return myCameraUsingMinimumRadius;
}

bool& Minimap::GetJustUpdatedCameraDimensions()
{
	return myJustUpdatedCameraDimensions;
}

void Minimap::ResetJustUpdatedCameraDimensions()
{
	myJustUpdatedCameraDimensions = false;
}

DE::Vector2f& Minimap::GetSmoothstep()
{
	return mySmoothstepValues;
}

const bool Minimap::IsRadarActive() const
{
	return !myRadarDurationTimer.IsDone();
}

void Minimap::Render()
{
	if (!myShouldRender) return;

	DreamEngine::SpriteDrawer& spriteDrawer = DreamEngine::Engine::GetInstance()->GetGraphicsEngine().GetSpriteDrawer();
	DE::GraphicsStateStack& graphicsStateStack = DreamEngine::Engine::GetInstance()->GetGraphicsEngine().GetGraphicsStateStack();
	graphicsStateStack.Push();
	graphicsStateStack.SetBlendState(DreamEngine::BlendState::AlphaBlend);
	graphicsStateStack.SetSamplerState(DE::SamplerFilter::Trilinear, DE::SamplerAddressMode::Clamp);
	DE::Vector2f canvasMin = myLastGeometryRenderBoatPos - myCameraDimensions / 2.0f;
	DE::Vector4f mapParams;
	mapParams.x = myCurrentRadius / myCameraDimensions.x;
	mapParams.y = myCurrentRadius / myCameraDimensions.y;
	mapParams.z = (myLastKnownBoatPos.x - canvasMin.x) / myCameraDimensions.x;
	mapParams.z = std::abs(mapParams.z - 1.0f);
	mapParams.w = (myLastKnownBoatPos.y - canvasMin.y) / myCameraDimensions.y;
	graphicsStateStack.SetCustomShaderParameters(mapParams);
	graphicsStateStack.UpdateGpuStates(false);

	spriteDrawer.Draw(*myBackgroundSprite, myBackgroundInstance);
	spriteDrawer.Draw(myGeometrySprite, myGeometryInstance);
	spriteDrawer.Draw(*myBorderSprite, myBorderInstance);
	if (!myRadarDurationTimer.IsDone())
	{
		for (int i = 0; i < myHeatmapBlipInstances.size(); i++)
		{
			spriteDrawer.Draw(*myHeatmapBlipSprite, myHeatmapBlipInstances[i]);
		}
		for (int i = 0; i < mySurvivorInstances.size(); i++)
		{
			spriteDrawer.Draw(*mySurvivorSprite, mySurvivorInstances[i]);
		}
		spriteDrawer.Draw(*myKeyItemSprite, myKeyItemInstance);
		spriteDrawer.Draw(*myArrowSprite, myArrowInstance);
		spriteDrawer.Draw(*myRadarLineSprite, myRadarLineInstance);
	}
	spriteDrawer.Draw(*myBoatSprite, myBoatInstance);
	graphicsStateStack.Pop();
}

DE::Vector2f Minimap::CalculateRelativeHUDPosition(DE::Vector2f& aPosition)
{
	DE::Vector2f relativeWorldPosition = myLastKnownBoatPos - aPosition / 100.0f;
	float hudScalar = relativeWorldPosition.Length() / myCurrentRadius;
	return myBackgroundInstance.myPosition + myBackgroundInstance.mySize.x * 0.5f * relativeWorldPosition.GetNormalized() * hudScalar;
}