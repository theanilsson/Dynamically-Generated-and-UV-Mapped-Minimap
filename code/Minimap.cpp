#include "Minimap.h"
#include <DreamEngine\engine.h>
#include <DreamEngine\graphics\GraphicsEngine.h>
#include <DreamEngine\graphics\GraphicsStateStack.h>
#include <DreamEngine\graphics\TextureManager.h>
#include <DreamEngine\graphics\SpriteDrawer.h>
#include <DreamEngine\utilities\UtilityFunctions.h>
#include <imgui\imgui.h>
#include "WorldGrid.h"
#include "MainSingleton.h"

Minimap::Minimap(WorldGrid* aWorldGrid)
{
	myWorldDimensions = aWorldGrid->GetWorldDimensions();
	myWorldCenterPoint = aWorldGrid->GetWorldCenterPoint();
	myCamera = std::make_unique<DE::Camera>();
	myCamera->SetOrtographicProjection(myWorldDimensions.x / 2.0f, -myWorldDimensions.x / 2.0f, myWorldDimensions.y / 2.0f, -myWorldDimensions.y / 2.0f, 1.0f, 50000.0f);
	myCamera->SetPosition(DE::Vector3f(myWorldCenterPoint.x, 2000.0f, myWorldCenterPoint.y));
	myCamera->GetTransform().SetRotation(DE::Rotator(90.0f, 0.0f, 0.0f));
	myWorldCenterPoint /= 100.0f;
	myWorldDimensions /= 100.0f;
	
	DE::Vector2ui highdetailResolution = DE::Vector2ui(15360, 8640);
	myDepthBuffer = DE::DepthBuffer::Create(highdetailResolution);
	myGeometryTarget = DE::RenderTarget::Create(highdetailResolution, DXGI_FORMAT_R32G32B32A32_FLOAT);
	myCanvas = DE::RenderTarget::Create(highdetailResolution);

	myShader = std::make_unique<DE::SpriteShader>();
	myShader->Init(L"shaders/instanced_sprite_shader_VS.cso", L"shaders/MinimapSpritePS.cso");

	myGeometrySprite.myTexture = &myCanvas;
	myGeometrySprite.myCustomShader = myShader.get();
	
	MainSingleton::GetInstance()->GetPostMaster().Subscribe(eMessageType::ShowHud, this);
	MainSingleton::GetInstance()->GetPostMaster().Subscribe(eMessageType::BoatTransformUpdate, this);
	
	auto& sprites = MainSingleton::GetInstance()->GetMinimapSprites();
	myBackgroundSprite = &sprites[0];
	myBoatSprite = &sprites[1];
	myArrowSprite = &sprites[2];
	myKeyItemSprite = &sprites[3];
	mySurvivorSprite = &sprites[4];
	myBorderSprite = &sprites[5];
	myHeatmapDotSprite = &sprites[6];

	auto& engine = *DreamEngine::Engine::GetInstance();
	DreamEngine::Vector2ui intResolution = engine.GetRenderSize();
	DreamEngine::Vector2f resolution = { (float)intResolution.x, (float)intResolution.y };

	myBackgroundInstance.myPosition = DE::Vector2f(0.9225, 0.865) * resolution;
	myGeometryInstance.myPosition = DE::Vector2f(0.9225, 0.865) * resolution;
	myBoatInstance.myPosition = DE::Vector2f(0.9225, 0.865) * resolution;
	myBorderInstance.myPosition = DE::Vector2f(0.9225, 0.865) * resolution;
	myArrowInstance.myPosition = DE::Vector2f(0.9225, 0.99) * resolution;
	myKeyItemInstance.myPosition = DE::Vector2f(-5000.0f);

	float radiusScalar = UtilityFunctions::Lerp(0.2f, 0.1f, (myRadius - 50.0f) / 50.0f);
	myBoatInstance.mySize = DE::Vector2f(0.1f * resolution.x, 0.25f * resolution.y) * radiusScalar;
	myBackgroundInstance.mySize = DE::Vector2f(0.135f * resolution.x, 0.25f * resolution.y);
	myGeometryInstance.mySize = DE::Vector2f(0.135f * resolution.x, 0.25f * resolution.y);
	myBorderInstance.mySize = DE::Vector2f(0.135f * resolution.x, 0.25f * resolution.y);
	myArrowInstance.mySize = DE::Vector2f(0.01f * resolution.x, 0.02f * resolution.y);
	myKeyItemInstance.mySize = DE::Vector2f(0.05f * resolution.x, 0.088889f * resolution.y) * radiusScalar;

	for (int i = 0; i < mySurvivorInstances.size(); i++)
	{
		mySurvivorInstances[i].myPosition = DE::Vector2f(-5000.0f);
		mySurvivorInstances[i].mySize = DE::Vector2f(0.05f * resolution.x, 0.088889f * resolution.y) * radiusScalar;
	}
}

Minimap::~Minimap()
{
	MainSingleton::GetInstance()->GetPostMaster().Unsubscribe(eMessageType::ShowHud, this);
	MainSingleton::GetInstance()->GetPostMaster().Unsubscribe(eMessageType::BoatTransformUpdate, this);
}

void Minimap::Update()
{
	if (!myShouldRender) return;

	auto& engine = *DreamEngine::Engine::GetInstance();
	DreamEngine::Vector2ui intResolution = engine.GetRenderSize();
	DreamEngine::Vector2f resolution = { (float)intResolution.x, (float)intResolution.y };

#ifndef _RETAIL
	if (ImGui::Begin("World map & minimap settings"))
	{
		ImGui::DragFloat("Minimap radius", &myRadius, 0.2f, 50.0f, 100.0f);
		//ImGui::DragFloat2("Smoothstep topology", &mySmoothstepValues.x, 1.0f, -10.0f, 10.0f);
		ImGui::Text("Full size world map:");
		ImGui::Image((ImTextureID)myCanvas.GetShaderResourceView(), ImGui::GetContentRegionAvail());
	}
	ImGui::End();
#endif

	float radiusScalar = UtilityFunctions::Lerp(0.2f, 0.1f, (myRadius - 50.0f) / 50.0f);
	myBoatInstance.mySize = DE::Vector2f(0.1f * resolution.x, 0.25f * resolution.y) * radiusScalar;
	myKeyItemInstance.mySize = DE::Vector2f(0.05f * resolution.x, 0.088889f * resolution.y) * radiusScalar;

	for (int i = 0; i < mySurvivorInstances.size(); i++)
	{
		mySurvivorInstances[i].myPosition = DE::Vector2f(-5000.0f);
		mySurvivorInstances[i].mySize = DE::Vector2f(0.05f * resolution.x, 0.088889f * resolution.y) * radiusScalar;
	}
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
			DE::Vector3f* boatData = static_cast<DE::Vector3f*>(aMsg.messageData);
			myBoatInstance.myRotation = boatData->y * UtilityFunctions::ourDegToRad;
			myLastKnownBoatPos = DE::Vector2f(boatData->x, boatData->z) / 100.0f;
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

DE::Camera& Minimap::GetCamera()
{
	return *myCamera;
}

float& Minimap::GetRadius()
{
	return myRadius;
}

DE::Vector2f& Minimap::GetSmoothstep()
{
	return mySmoothstepValues;
}

void Minimap::Render()
{
	if (!myShouldRender) return;

	auto& engine = *DreamEngine::Engine::GetInstance();
	DreamEngine::SpriteDrawer& spriteDrawer(engine.GetGraphicsEngine().GetSpriteDrawer());
	auto& graphicsStateStack = DreamEngine::Engine::GetInstance()->GetGraphicsEngine().GetGraphicsStateStack();
	
	graphicsStateStack.Push();
	graphicsStateStack.SetAlphaTestThreshold(0.0f);
	graphicsStateStack.SetBlendState(DreamEngine::BlendState::AlphaBlend);

	DE::Vector2f worldMin = myWorldCenterPoint - myWorldDimensions / 2.0f;
	DE::Vector4f mapParams;
	mapParams.x = myRadius / myWorldDimensions.x;
	mapParams.y = myRadius / myWorldDimensions.y;
	mapParams.z = (myLastKnownBoatPos.x - worldMin.x) / myWorldDimensions.x;
	mapParams.z = std::abs(mapParams.z - 1.0f);
	mapParams.w = (myLastKnownBoatPos.y - worldMin.y) / myWorldDimensions.y;
	graphicsStateStack.SetCustomShaderParameters(mapParams);

	graphicsStateStack.UpdateGpuStates(false);

	spriteDrawer.Draw(*myBackgroundSprite, myBackgroundInstance);

	myCanvas.SetAsResourceOnSlot(11);
	spriteDrawer.Draw(myGeometrySprite, myGeometryInstance);

	for (int i = 0; i < myHeatmapDotInstances.size(); i++)
	{
		spriteDrawer.Draw(*myHeatmapDotSprite, myHeatmapDotInstances[i]);
	}
	for (int i = 0; i < mySurvivorInstances.size(); i++)
	{
		spriteDrawer.Draw(*mySurvivorSprite, mySurvivorInstances[i]);
	}
	spriteDrawer.Draw(*myKeyItemSprite, myKeyItemInstance);
	spriteDrawer.Draw(*myBoatSprite, myBoatInstance);
	spriteDrawer.Draw(*myBorderSprite, myBorderInstance);
	spriteDrawer.Draw(*myArrowSprite, myArrowInstance);

	graphicsStateStack.Pop();
}