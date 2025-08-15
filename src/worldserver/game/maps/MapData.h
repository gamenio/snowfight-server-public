#ifndef __MAP_DATA_H__
#define __MAP_DATA_H__

#include "Common.h"
#include "game/tiles/TileCoord.h"
#include "game/tiles/TileArea.h"
#include "mapparser/TMXXMLParser.h"

#define TRAINING_GROUND_MAP_ID							0	// The map ID of the training ground

#define MAP_MARGIN_IN_TILES								11	// Map margin in tiles

struct MapGrade
{
	uint8 combatGrade;			// Combat grade
	uint16 mapId;				// Map ID
};

struct MapConfig
{
	uint16 id;					// Map ID
	int32 populationCap;		// Population cap
	int32 battleDuration;		// Duration of battle, unit: seconds
};

class MapData
{
	enum TileType: uint8
	{
		TILE_TYPE_NONE,
		TILE_TYPE_PENETRABLE,
		TILE_TYPE_COLLIDABLE,
		TILE_TYPE_CONCEALABLE,
	};

	enum GroundType: uint8
	{
		GROUND_TYPE_NONE,
		GROUND_TYPE_SNOW,
		GROUND_TYPE_WATER,
	};

	struct TileObject
	{
		int32 width;
		int32 height;
		int32 offsetX;
		int32 offsetY;
		std::vector<uint8> data;
	};

public:
	MapData(MapConfig const& config);
	~MapData();

	bool loadData();
	uint16 getId() const { return m_config.id; }

	bool isWall(int32 x, int32 y) const { return this->isCollidable(x, y) || this->isPenetrable(x, y); }
	bool isWall(TileCoord const& tileCoord) const { return this->isWall(tileCoord.x, tileCoord.y); }
	bool isSnow(TileCoord const& tileCoord) const { return m_groundData[this->getTileIndex(tileCoord)] == GROUND_TYPE_SNOW; }
	bool isWater(TileCoord const& tileCoord) const { return m_groundData[this->getTileIndex(tileCoord)] == GROUND_TYPE_WATER; }
	bool isPenetrable(int32 x, int32 y) const { return m_tileData[this->getTileIndex(x, y)] == TILE_TYPE_PENETRABLE; }
	bool isPenetrable(TileCoord const& tileCoord) const { return this->isPenetrable(tileCoord.x, tileCoord.y); }
	bool isCollidable(int32 x, int32 y) const { return m_tileData[this->getTileIndex(x, y)] == TILE_TYPE_COLLIDABLE; }
	bool isCollidable(TileCoord const& tileCoord) const { return this->isCollidable(tileCoord.x, tileCoord.y); }
	bool isConcealable(TileCoord const& tileCoord) const { return m_tileData[this->getTileIndex(tileCoord)] == TILE_TYPE_CONCEALABLE; }
	bool isValidTileCoord(int32 x, int32 y) const { return x >= 0 && y >= 0 && x < m_mapInfo->getMapSize().width && y < m_mapInfo->getMapSize().height; }
	bool isValidTileCoord(TileCoord const& tileCoord) const { return this->isValidTileCoord(tileCoord.x, tileCoord.y); }

	bool isSameDistrict(TileCoord const& tileCoord1, TileCoord const& tileCoord2) const;
	uint32 getDistrictId(TileCoord const& tileCoord) const;
	std::unordered_map<uint32, std::vector<TileCoord>> const& getDistrictWaypoints() const { return m_districtWaypoints; }

	Size const& getMapSize() const { return m_mapInfo->getMapSize(); }
	Size const& getTileSize() const { return m_mapInfo->getTileSize(); }
	int32 getTileIndex(TileCoord const& tileCoord) const { return this->getTileIndex(tileCoord.x, tileCoord.y); }
	int32 getTileIndex(int32 x, int32 y) const { return x + y * (int32)m_mapInfo->getMapSize().width; }

	int32 getPopulationCap() const { return m_config.populationCap; }
	int32 getBattleDuration() const { return m_config.battleDuration; }

	void findNearestOpenPoint(TileCoord const& findCoord, TileCoord& result, bool isExcludeHidingSpots) const;
	std::map<int32, std::map<int32, TileCoord>> const& getSplitOpenPoints() const { return m_splitOpenPoints; }
	std::map<int32, std::map<int32, TileCoord>> const& getSplitUnconcealableOpenPoints() const { return m_splitUnconcealableOpenPoints; }
	void findNearestOpenPointList(std::map<int32, std::map<int32, TileCoord>> const& splitOpenPoints, TileCoord const& findCoord, std::vector<TileCoord>& result) const;
	void findNearestOpenPointList(TileCoord const& findCoord, std::vector<TileCoord>& result, bool isExcludeHidingSpots) const;
	bool findNearestHidingSpot(TileCoord const& center, TileCoord& result) const;

	bool findWaypoints(TileCoord const& findCoord, std::vector<TileCoord>& result);
	bool getLinkedWaypoint(TileCoord const& source, TileCoord& target);
	std::unordered_map<int32, std::vector<TileCoord>> const& getWaypointExtents() const { return m_waypointExtents; };
	std::vector<TileCoord> const& getWaypointExtent(TileCoord const& waypoint) const;
	uint32 getWaypointDistrictId(TileCoord const& waypoint) const;

	Point mapToOpenGLPos(Point const& mapPos) const;
	Point openGLToMapPos(Point const& glPos) const;

private:
	bool initMapInfo(std::string const& tmxFile);

	std::unordered_map<int32, TileObject> parseTileObjects();
	void classifyTiles();
	void initNearestHidingSpots();
	void initWaypoints();

	void splitOpenPoints();
	void findClosestOpenPointList(std::map<int32, std::map<int32, TileCoord>> const& splitOpenPoints, TileCoord const& findCoord, std::vector<TileCoord>& result) const;
	void initNearestOpenPoints();
	bool findClosestPoints(std::map<int32, TileCoord> const& withinPoints, TileCoord const& findPoint, std::multimap<int32, TileCoord>& result) const;
	bool addPointIfCloserToTarget(TileCoord const& point, TileCoord const& target, std::multimap<int32, TileCoord>& result) const;

	Value const& getTileProperty(std::string const& propertyName, TileCoord const& tileCoord, TMXLayerInfo* layer) const;
	bool getTilePropertyAsBool(std::string const& propertyName, TileCoord const& tileCoord, TMXLayerInfo* layer) const;
	bool isUseAutomaticVertexZ(TMXLayerInfo* layer) const;

	Point screenToMapPos(Point const& screenPos) const;
	Point mapToScreenPos(Point const& mapPos) const;

	void initDistricts();
	bool isValidDistrictTileCoord(TileCoord const& tileCoord);
	void fillDistrict(TileCoord const& startCoord, uint32 districtId);

	MapConfig m_config;
	std::string m_tmxFilename;
	std::string m_resourcePath;

	TMXMapInfo* m_mapInfo;
	TMXObjectGroup* m_debugGroup;

	TileType* m_tileData;
	GroundType* m_groundData;
	uint32* m_districtData;

	std::vector<TileCoord> m_openPoints;
	std::vector<TileCoord> m_closedPoints;
	std::vector<TileCoord> m_hidingSpots;
	std::map<int32, std::map<int32, TileCoord>> m_splitOpenPoints;
	std::unordered_map<int32 /* TileIndex */, std::vector<TileCoord>> m_nearestOpenPoints;
	std::unordered_map<int32, TileCoord> m_nearestHidingSpots;
	std::vector<TileCoord> m_unconcealableOpenPoints;
	std::vector<TileCoord> m_concealableClosedPoints;
	std::map<int32, std::map<int32, TileCoord>> m_splitUnconcealableOpenPoints;
	std::unordered_map<int32 /* TileIndex */, std::vector<TileCoord>> m_nearestUnconcealableOpenPoints;
	std::unordered_map<uint32 /* DistrictID */, std::vector<TileCoord>> m_districtWaypoints;
	std::unordered_map<int32 /* TileIndex */, uint32 /* DistrictID */> m_waypointDistrictIds;
	std::unordered_map<int32 /* TileIndex */, TileCoord> m_linkedWaypoints;
	std::unordered_map<int32 /* TileIndex */, std::vector<TileCoord>> m_waypointExtents;
};

#endif // __MAP_DATA_H__
