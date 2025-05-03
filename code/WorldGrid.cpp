#include "WorldGrid.h"

WorldGrid::WorldGrid(const DE::Vector2f aPlayableAreaMinimumPoint, const DE::Vector2f aPlayableAreaMaximumPoint, const float aCellScale)
{
	myCellScale = aCellScale;

	float gridWidth = aPlayableAreaMaximumPoint.x - aPlayableAreaMinimumPoint.x;
	float gridHeight = aPlayableAreaMaximumPoint.y - aPlayableAreaMinimumPoint.y;

	myIndexWidth = static_cast<int>(gridWidth / myCellScale);
	myIndexWidth % 2 == 1 ? myIndexWidth += 1 : myIndexWidth += 2;

	myIndexHeight = static_cast<int>(gridHeight / myCellScale);
	myIndexHeight % 2 == 1 ? myIndexHeight += 1 : myIndexHeight += 2;

	DE::Vector2f gridOriginPoint = DE::Vector2f(aPlayableAreaMinimumPoint.x + gridWidth * 0.5f,
											 aPlayableAreaMinimumPoint.y + gridHeight * 0.5f);
	gridOriginPoint.x -= static_cast<float>(myIndexWidth) / 2.0f * myCellScale;
	gridOriginPoint.y -= static_cast<float>(myIndexHeight) / 2.0f * myCellScale;

	myCells.resize(myIndexWidth);
	for (auto& column : myCells)
	{
		column.resize(myIndexHeight);
	}

	for (int x = 0; x < myCells.size(); x++)
	{
		for (int y = 0; y < myCells[x].size(); y++)
		{
			myCells[x][y].minPoint = DE::Vector2f(gridOriginPoint.x + myCellScale * x, gridOriginPoint.y + myCellScale * y);
			myCells[x][y].maxPoint = DE::Vector2f(gridOriginPoint.x + myCellScale + myCellScale * x, gridOriginPoint.y + myCellScale + myCellScale * y);
		}
	}
}

const DE::Vector2i WorldGrid::GetIndexDimensions() const
{
	return DE::Vector2i(myIndexWidth, myIndexHeight);
}

const DE::Vector2f WorldGrid::GetWorldDimensions() const
{
	DE::Vector2f vectorDimensions = myCells.back().back().maxPoint - myCells.front().front().minPoint;
	return DE::Vector2f(std::abs(vectorDimensions.x), std::abs(vectorDimensions.y));
}

const DE::Vector2f WorldGrid::GetWorldCenterPoint() const
{
	DE::Vector2f dimensions = GetWorldDimensions();
	return DE::Vector2f(myCells.front().front().minPoint + dimensions * 0.5f);
}

const DE::Vector2i WorldGrid::GetIndicesAtPosition(const DE::Vector2f& aPosition) const
{
	const int cellX = static_cast<int>((aPosition.x - myCells[0][0].minPoint.x) / myCellScale);
	const int cellY = static_cast<int>((aPosition.y - myCells[0][0].minPoint.y) / myCellScale);

	if (cellX >= 0 && cellX < myIndexWidth && cellY >= 0 && cellY < myIndexHeight)
	{
		return { cellX, cellY };
	}
	return DE::Vector2i(-1, -1);
}

const float WorldGrid::GetCellScale() const
{
    return myCellScale;
}