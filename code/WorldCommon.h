#pragma once

enum class eHeatmapType
{
	EnemyMovement, // Exists primarily for use on the minimap
	LeviathanMovement, // Exists primarily for use on the minimap
	BoatMovement,  // Exists only for gameplay coding use, e.g. if we want to sample previous boat positions for AI/Leviathan logic
	DefaultTypeAndCount
};

enum class eHeatmapCellScale
{
	EnemyMovement = 100,
	LeviathanMovement = 120,
	BoatMovement = 1000
};

constexpr int worldGridIndexWidth = 20;