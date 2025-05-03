#pragma once
#include <DreamEngine\math\vector2.h>
#include <vector>
#include "WorldCommon.h"

class GameObject;

class WorldGrid
{
public:
	WorldGrid() = delete;
	WorldGrid(const DE::Vector2f aPlayableAreaMinimumPoint, const DE::Vector2f aPlayableAreaMaximumPoint, const float aCellScale);
	WorldGrid(const WorldGrid& anOtherGrid) = delete;
	WorldGrid& operator=(const WorldGrid& anOtherGrid) = delete;
	WorldGrid(WorldGrid&& anOtherGrid) = default;
	WorldGrid& operator=(WorldGrid&& anOtherGrid) = default;
	~WorldGrid() = default;

	const DE::Vector2i GetIndexDimensions() const;
	const DE::Vector2f GetWorldDimensions() const;
	const DE::Vector2f GetWorldCenterPoint() const;
	const DE::Vector2i GetIndicesAtPosition(const DE::Vector2f& aPosition) const;
	const float GetCellScale() const;

private:
	struct Cell
	{
		DE::Vector2f minPoint;
		DE::Vector2f maxPoint;
	};

	float myCellScale = 0.0f;
	int myIndexHeight = 0;
	int myIndexWidth = 0;

	std::vector<std::vector<Cell>> myCells;
};