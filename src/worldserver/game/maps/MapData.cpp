#include "MapData.h"

#include "utilities/Random.h"
#include "utilities/TimeUtil.h"
#include "logging/Log.h"
#include "game/utils/MathTools.h"
#include "game/world/ObjectMgr.h"
#include "Application.h"

#define MAP_DIR									"map"

MapData::MapData(MapConfig const& config) :
	m_config(config),
	m_mapInfo(nullptr),
	m_debugGroup(nullptr),
	m_tileData(nullptr),
	m_groundData(nullptr),
	m_districtData(nullptr)
{
	m_resourcePath = sApplication->getRoot();
	m_tmxFilename = StringUtil::format("%s/%d.tmx", MAP_DIR, m_config.id);
}


MapData::~MapData()
{
	m_debugGroup = nullptr;

	if (m_tileData)
	{
		delete[] m_tileData;
		m_tileData = nullptr;
	}
	if (m_groundData)
	{
		delete[] m_groundData;
		m_groundData = nullptr;
	}
	if (m_districtData)
	{
		delete[] m_districtData;
		m_districtData = nullptr;
	}

	if (m_mapInfo)
	{
		delete m_mapInfo;
		m_mapInfo = nullptr;
	}
}


bool MapData::loadData()
{
	do
	{
		NS_LOG_INFO("world.mapdata", "Loading map data from %s", m_tmxFilename.c_str());
		std::string tmxFile = m_resourcePath + m_tmxFilename;
		if (!this->initMapInfo(tmxFile))
		{
			NS_LOG_ERROR("world.mapdata", "Failed to initialize map info from %s", m_tmxFilename.c_str());
			break;
		}

		this->classifyTiles();
		this->splitOpenPoints();
		this->initNearestOpenPoints();
		this->initNearestHidingSpots();
		this->initWaypoints();
		this->initDistricts();

		return true;
	} while (0);

	return false;
}

bool MapData::isSameDistrict(TileCoord const& tileCoord1, TileCoord const& tileCoord2) const
{
	if (tileCoord1 == tileCoord2)
		return true;

	int32 tileIndex1 = this->getTileIndex(tileCoord1);
	int32 tileIndex2 = this->getTileIndex(tileCoord2);
	return m_districtData[tileIndex1] == m_districtData[tileIndex2];
}

uint32 MapData::getDistrictId(TileCoord const& tileCoord) const
{
	int32 tileIndex = this->getTileIndex(tileCoord);
	return m_districtData[tileIndex];
}

bool MapData::initMapInfo(std::string const& tmxFile)
{
	do 
	{
		m_mapInfo = new TMXMapInfo();
		NS_BREAK_IF(!m_mapInfo || !m_mapInfo->initWithTMXFile(tmxFile));

		m_debugGroup = m_mapInfo->getObjectGroup("DebugGroup");

		return true;
	} while (0);

	return false;
}

std::unordered_map<int32, MapData::TileObject> MapData::parseTileObjects()
{
	std::unordered_map<int32, TileObject> tileObjects;
	auto const& tilePropeties = m_mapInfo->getTileProperties();
	for (auto const& pr : tilePropeties)
	{
		int32 gid = pr.first;
		auto const& properties = pr.second;
		if (properties.isNull() || properties.getType() != Value::Type::MAP)
			continue;

		auto& valueMap = properties.asValueMap();
		auto it = valueMap.find("ObjectData");
		if (it != valueMap.end())
		{
			TileObject& object = tileObjects[gid];
			std::string data = (*it).second.asString();
			object.width = valueMap.at("ObjectWidth").asInt();
			object.height = valueMap.at("ObjectHeight").asInt();
			it = valueMap.find("ObjectOffsetX");
			object.offsetX = it != valueMap.end() ? (*it).second.asInt() : 0;
			it = valueMap.find("ObjectOffsetY");
			object.offsetY = it != valueMap.end() ? (*it).second.asInt() : 0;

			std::vector<std::string> elemTokens;
			std::istringstream dataStm(data);
			std::string sRow;
			while (getline(dataStm, sRow, '\n'))
			{
				std::string sElem;
				std::istringstream rowStm(sRow);
				while (getline(rowStm, sElem, ','))
				{
					elemTokens.push_back(sElem);
				}
			}

			NS_ASSERT(elemTokens.size() == object.width * object.height);
			object.data.resize(elemTokens.size());
			for (std::size_t i = 0; i < elemTokens.size(); ++i)
			{
				auto elem = (uint8)strtoul(elemTokens[i].c_str(), nullptr, 10);
				object.data[i] = elem;
			}
		}
	}

	return tileObjects;
}

void MapData::classifyTiles()
{
	NS_ASSERT(!m_groundData);
	NS_ASSERT(!m_tileData);

	int32 width = (int32)this->getMapSize().width;
	int32 height = (int32)this->getMapSize().height;
	int32 nTiles = width * height;

	m_groundData = new GroundType[nTiles];
	std::fill(m_groundData, m_groundData + nTiles, GROUND_TYPE_NONE);

	m_tileData = new TileType[nTiles];
	std::fill(m_tileData, m_tileData + nTiles, TILE_TYPE_NONE);

	TMXLayerInfo* groundLayer = nullptr;
	std::unordered_map<int32, TileObject> tileObjects = this->parseTileObjects();
	for (auto layer : m_mapInfo->getLayers())
	{
		bool useAutomaticVertexZ = this->isUseAutomaticVertexZ(layer);
		for (int32 x = 0; x < width; ++x)
		{
			for (int32 y = 0; y < height; ++y)
			{
				int32 gid = layer->getTileGIDAt(Point((float)x, (float)y));
				if (!gid)
					continue;

				auto it = tileObjects.find(gid);
				if (it == tileObjects.end())
					continue;

				TileObject const& object = (*it).second;
				for (int32 i = 0; i < object.width; ++i)
				{
					for (int32 j = 0; j < object.height; ++j)
					{
						TileCoord coord;
						coord.x = x + i + object.offsetX;
						coord.y = y + j + object.offsetY;
						int32 tileIndex = this->getTileIndex(coord);
						uint8 tileType = object.data[i + j * object.width];
						if (m_tileData[tileIndex] == TILE_TYPE_NONE || !useAutomaticVertexZ)
							m_tileData[tileIndex] = static_cast<TileType>(tileType);
					}
				}
			}
		}

		if (layer->_name == "Ground")
			groundLayer = layer;
	}

	for (int32 x = 0; x < width; ++x)
	{
		for (int32 y = 0; y < height; ++y)
		{
			TileCoord coord(x, y);
			int32 tileIndex = this->getTileIndex(coord);

			if ((x < MAP_MARGIN_IN_TILES || x >= width - MAP_MARGIN_IN_TILES)
				|| (y < MAP_MARGIN_IN_TILES || y >= height - MAP_MARGIN_IN_TILES))
			{
				if (m_tileData[tileIndex] == TILE_TYPE_NONE || m_tileData[tileIndex] == TILE_TYPE_CONCEALABLE)
					m_tileData[tileIndex] = TILE_TYPE_PENETRABLE;
			}

			if (groundLayer)
			{
				if (this->getTilePropertyAsBool("Snow", coord, groundLayer))
					m_groundData[tileIndex] = GROUND_TYPE_SNOW;
				else if (this->getTilePropertyAsBool("Water", coord, groundLayer))
					m_groundData[tileIndex] = GROUND_TYPE_WATER;
			}

			if (this->isConcealable(coord))
				m_hidingSpots.emplace_back(x, y);

			if (!this->isWall(coord))
				m_openPoints.emplace_back(x, y);
			else
				m_closedPoints.emplace_back(x, y);

			if (!this->isWall(coord) && !this->isConcealable(coord))
				m_unconcealableOpenPoints.emplace_back(x, y);
			else
				m_concealableClosedPoints.emplace_back(x, y);
		}
	}
}

void MapData::findNearestOpenPoint(TileCoord const& findCoord, TileCoord& result, bool isExcludeHidingSpots) const
{
	std::vector<TileCoord> foundCoordList;
	this->findNearestOpenPointList(findCoord, foundCoordList, isExcludeHidingSpots);
	NS_ASSERT(!foundCoordList.empty());
	int32 index = 0;
	if (foundCoordList.size() > 1)
		index = random(0, (int32)(foundCoordList.size() - 1));
	result = foundCoordList[index];
}

void MapData::findNearestOpenPointList(std::map<int32, std::map<int32, TileCoord>> const& splitOpenPoints, TileCoord const& findCoord, std::vector<TileCoord>& result) const
{
	this->findClosestOpenPointList(splitOpenPoints, findCoord, result);
}

void MapData::findNearestOpenPointList(TileCoord const& findCoord, std::vector<TileCoord>& result, bool isExcludeHidingSpots) const
{
	TileCoord vaildCoord;
	if (!this->isValidTileCoord(findCoord))
	{
		vaildCoord.x = std::min(std::max(findCoord.x, 0), (int32)m_mapInfo->getMapSize().width - 1);
		vaildCoord.y = std::min(std::max(findCoord.y, 0), (int32)m_mapInfo->getMapSize().height - 1);
	}
	else
		vaildCoord = findCoord;
	if (!this->isWall(vaildCoord) && (!isExcludeHidingSpots || !this->isConcealable(vaildCoord)))
		result.emplace_back(vaildCoord);
	else
	{
		int32 tileIndex = this->getTileIndex(vaildCoord);
		if (isExcludeHidingSpots)
		{
			auto it = m_nearestUnconcealableOpenPoints.find(tileIndex);
			NS_ASSERT(it != m_nearestUnconcealableOpenPoints.end());
			result = (*it).second;
		}
		else
		{
			auto it = m_nearestOpenPoints.find(tileIndex);
			NS_ASSERT(it != m_nearestOpenPoints.end());
			result = (*it).second;
		}
	}
}

bool MapData::findNearestHidingSpot(TileCoord const& center, TileCoord& result) const
{
	int32 tileIndex = center.x * (int32)this->getMapSize().height + center.y;
	auto it = m_nearestHidingSpots.find(tileIndex);
	if (it != m_nearestHidingSpots.end())
	{
		result = (*it).second;
		return true;
	}

	return false;
}

bool MapData::findWaypoints(TileCoord const& findCoord, std::vector<TileCoord>& result)
{
	int32 tileIndex = this->getTileIndex(findCoord);
	auto it = m_districtWaypoints.find(m_districtData[tileIndex]);
	if (it != m_districtWaypoints.end())
	{
		result = (*it).second;
		return true;
	}

	return false;
}

bool MapData::getLinkedWaypoint(TileCoord const& source, TileCoord& target)
{
	int32 tileIndex = this->getTileIndex(source);
	auto it = m_linkedWaypoints.find(tileIndex);
	if (it != m_linkedWaypoints.end())
	{
		target = (*it).second;
		return true;
	}
	return false;
}

std::vector<TileCoord> const& MapData::getWaypointExtent(TileCoord const& waypoint) const
{
	auto it = m_waypointExtents.find(this->getTileIndex(waypoint));
	NS_ASSERT(it != m_waypointExtents.end());
	return (*it).second;
}

uint32 MapData::getWaypointDistrictId(TileCoord const& waypoint) const
{
	int32 tileIndex = this->getTileIndex(waypoint);
	auto it = m_waypointDistrictIds.find(tileIndex);
	if (it != m_waypointDistrictIds.end())
		return (*it).second;

	return 0;
}

Point MapData::mapToOpenGLPos(Point const& mapPos) const
{
	Point glPos = this->mapToScreenPos(mapPos);
	glPos.y = m_mapInfo->getMapSize().height * m_mapInfo->getTileSize().height - 1 - glPos.y;

	return glPos;
}

Point MapData::openGLToMapPos(Point const& glPos) const
{
	float glY = m_mapInfo->getMapSize().height * m_mapInfo->getTileSize().height - 1 - glPos.y;
	return this->screenToMapPos(Point(glPos.x, glY));
}

void MapData::initNearestHidingSpots()
{
	for (auto const& curr : m_openPoints)
	{
		int32 tileIndex = curr.x * (int32)this->getMapSize().height + curr.y;
		TileCoord nearest = TileCoord::INVALID;
		for (auto const& p : m_hidingSpots)
		{
			int32 d1 = std::abs(curr.x - p.x) + std::abs(curr.y - p.y);
			int32 d2 = std::abs(curr.x - nearest.x) + std::abs(curr.y - nearest.y);
			if (d1 < d2)
				nearest = p;
		}
		m_nearestHidingSpots[tileIndex] = nearest;
	}
}

void MapData::initWaypoints()
{
	TMXObjectGroup* waypointGroup = m_mapInfo->getObjectGroup("WaypointGroup");
	if (waypointGroup)
	{
		auto const& objects = waypointGroup->getObjects();
		std::unordered_map<int32, TileCoord> waypointCoords;
		std::unordered_map<int32, int32> waypointLinkIds;
		for (auto const& obj : objects)
		{
			auto const& properties = obj.asValueMap();
			std::string name = properties.at("name").asString();
			if (name == "Waypoint")
			{
				//uint32 gid = properties.at("gid").asUnsignedInt();
				int32 id = properties.at("id").asInt();
				float x = properties.at("x").asFloat();
				float y = properties.at("y").asFloat();
				uint32 districtID = properties.at("DistrictID").asUnsignedInt();
				int32 linkId = properties.at("LinkID").asInt();
				waypointLinkIds.emplace(id, linkId);

				Point glPos = this->mapToOpenGLPos(Point(x, y));
				TileCoord coord(m_mapInfo->getMapSize(), glPos);
				waypointCoords.emplace(id, coord);
				m_districtWaypoints[districtID].emplace_back(coord);
				m_waypointDistrictIds[this->getTileIndex(coord)] = districtID;
			}
		}

		std::unordered_set<int32> extentFlags;
		for (auto it = waypointLinkIds.begin(); it != waypointLinkIds.end(); ++it)
		{
			TileCoord const& source = waypointCoords.at((*it).first);
			TileCoord const& target = waypointCoords.at((*it).second);
			int32 sourceTileIndex = this->getTileIndex(source);
			m_linkedWaypoints.emplace(sourceTileIndex, target);

			// Initialize the waypoint area
			int32 diffX = target.x - source.x;
			int32 diffY = target.y - source.y;
			int32 areaW, areaH;
			// Horizontal splitting
			if (diffX * diffY > 0 || diffY == 0)
			{
				areaW = (int32)std::ceil((std::abs(diffX) + 1) / 2.f);
				areaH = std::abs(diffY) + 1;
			}
			// Vertical splitting
			else
			{
				areaW = std::abs(diffX) + 1;
				areaH = (int32)std::ceil((std::abs(diffY) + 1) / 2.f);
			}
			TileCoord coord;
			for (int32 x = 0; x < areaW; x++)
			{
				for (int32 y = 0; y < areaH; y++)
				{
					coord.x = source.x + (diffX > 0 ? x : -x);
					coord.y = source.y + (diffY > 0 ? y : -y);
					if (!this->isWall(coord))
					{
						int32 tileIndex = this->getTileIndex(coord);
						if (extentFlags.find(tileIndex) == extentFlags.end())
						{
							m_waypointExtents[sourceTileIndex].emplace_back(coord);
							extentFlags.insert(tileIndex);
						}
					}
				}
			}
		}
	}
}

void MapData::splitOpenPoints()
{
	auto currTime = getHighResolutionTimeMillis();

	for (auto it = m_openPoints.begin(); it != m_openPoints.end(); ++it)
	{
		TileCoord const& coord = *it;
		m_splitOpenPoints[coord.x - coord.y][coord.x + coord.y].setTileCoord(coord.x, coord.y);
	}

	for (auto it = m_unconcealableOpenPoints.begin(); it != m_unconcealableOpenPoints.end(); ++it)
	{
		TileCoord const& coord = *it;
		m_splitUnconcealableOpenPoints[coord.x - coord.y][coord.x + coord.y].setTileCoord(coord.x, coord.y);
	}

	auto elapsed = getHighResolutionTimeMillis() - currTime;
	NS_LOG_DEBUG("world.mapdata", "splitOpenPoints() time in %f ms", elapsed);
}

void MapData::findClosestOpenPointList(std::map<int32, std::map<int32, TileCoord>> const& splitOpenPoints, TileCoord const& findCoord, std::vector<TileCoord>& result) const
{
	std::multimap<int32, TileCoord> sortedPoints;
	TileCoord const& cp = findCoord;

	auto boundIt = splitOpenPoints.lower_bound(cp.x - cp.y);
	if (boundIt != splitOpenPoints.end())
	{
		auto const& points = (*boundIt).second;
		NS_ASSERT(!points.empty());
		this->findClosestPoints(points, cp, sortedPoints);

		auto forwardIt = std::next(boundIt);
		auto backwardIt = splitOpenPoints.end();
		if (boundIt != splitOpenPoints.begin())
			backwardIt = std::prev(boundIt);
		while (forwardIt != splitOpenPoints.end() || backwardIt != splitOpenPoints.end())
		{
			if (forwardIt != splitOpenPoints.end())
			{
				auto const& points = (*forwardIt).second;
				if (!this->findClosestPoints(points, cp, sortedPoints))
					forwardIt = splitOpenPoints.end();
				else
					++forwardIt;
			}

			if (backwardIt != splitOpenPoints.end())
			{
				auto const& points = (*backwardIt).second;
				if (!this->findClosestPoints(points, cp, sortedPoints))
					backwardIt = splitOpenPoints.end();
				else
					--backwardIt;
			}
		}
	}
	else
	{
		for (auto it = splitOpenPoints.rbegin(); it != splitOpenPoints.rend(); ++it)
		{
			auto const& points = (*it).second;
			NS_ASSERT(!points.empty());
			if (!this->findClosestPoints(points, cp, sortedPoints))
				break;
		}
	}

	int32 minD = 0;
	for (auto it = sortedPoints.begin(); it != sortedPoints.end(); ++it)
	{
		int32 d = (*it).first;
		if (minD != 0 && d > minD)
			break;
		minD = d;
		result.emplace_back((*it).second);
	}
}

void MapData::initNearestOpenPoints()
{
	auto currTime = getHighResolutionTimeMillis();
	int32 nClosedPoints;

	nClosedPoints = (int32)m_closedPoints.size();
	for (int32 i = 0; i < nClosedPoints; ++i)
	{
		TileCoord const& cp = m_closedPoints[i];
		int32 tileIndex = this->getTileIndex(cp);
		this->findClosestOpenPointList(m_splitOpenPoints, cp, m_nearestOpenPoints[tileIndex]);
	}
	NS_ASSERT(m_nearestOpenPoints.size() == m_closedPoints.size());

	nClosedPoints = (int32)m_concealableClosedPoints.size();
	for (int32 i = 0; i < nClosedPoints; ++i)
	{
		TileCoord const& cp = m_concealableClosedPoints[i];
		int32 tileIndex = this->getTileIndex(cp);
		this->findClosestOpenPointList(m_splitUnconcealableOpenPoints, cp, m_nearestUnconcealableOpenPoints[tileIndex]);
	}
	NS_ASSERT(m_nearestUnconcealableOpenPoints.size() == m_concealableClosedPoints.size());

	auto elapsed = getHighResolutionTimeMillis() - currTime;
	NS_LOG_DEBUG("world.mapdata", "initNearestOpenPoints() time in %f ms", elapsed);
}

bool MapData::findClosestPoints(std::map<int32, TileCoord> const& withinPoints, TileCoord const& findPoint, std::multimap<int32, TileCoord>& result) const
{
	bool found = false;
	auto boundIt = withinPoints.lower_bound(findPoint.x + findPoint.y);
	if (boundIt != withinPoints.end())
	{
		TileCoord const& p = (*boundIt).second;
		found = this->addPointIfCloserToTarget(p, findPoint, result);

		if (boundIt != withinPoints.begin())
		{
			--boundIt;
			TileCoord const& p = (*boundIt).second;
			found = this->addPointIfCloserToTarget(p, findPoint, result);
		}
	}
	else
	{
		auto it = withinPoints.rbegin();
		if (it != withinPoints.rend())
		{
			TileCoord const& p = (*it).second;
			found = this->addPointIfCloserToTarget(p, findPoint, result);
		}
	}

	return found;
}

bool MapData::addPointIfCloserToTarget(TileCoord const& point, TileCoord const& target, std::multimap<int32, TileCoord>& result) const
{
	int32 dx = target.x - point.x;
	int32 dy = target.y - point.y;
	int32 d = dx * dx + dy * dy;
	auto retIt = result.begin();
	if (retIt != result.end())
	{
		if (d <= (*retIt).first)
		{
			result.emplace(d, point);
			return true;
		}

	}
	else
	{
		retIt = result.emplace(d, point);
		return true;
	}

	return false;
}

Value const& MapData::getTileProperty(std::string const& propertyName, TileCoord const& tileCoord, TMXLayerInfo* layer) const
{
	if (!this->isValidTileCoord(tileCoord))
		return Value::Null;

	Point p;
	p.x = static_cast<float>(tileCoord.x);
	p.y = static_cast<float>(tileCoord.y);
	int gid = layer->getTileGIDAt(p);
	auto& properties = m_mapInfo->getPropertiesForGID(gid);
	if (properties.isNull() || properties.getType() != Value::Type::MAP)
		return Value::Null;

	auto& valueMap = properties.asValueMap();
	auto it = valueMap.find(propertyName);
	if (it != valueMap.end())
		return (*it).second;

	return Value::Null;
}

bool MapData::getTilePropertyAsBool(std::string const& propertyName, TileCoord const& tileCoord, TMXLayerInfo* layer) const
{
	Value const& value = this->getTileProperty(propertyName, tileCoord, layer);
	return !value.isNull() && value.asBool();
}

bool MapData::isUseAutomaticVertexZ(TMXLayerInfo* layer) const
{
	auto const& properties = layer->getProperties();
	auto it = properties.find("cc_vertexz");
	if (it != properties.end())
	{
		auto const& val = (*it).second.asString();
		if (val == "automatic")
			return true;
	}

	return false;
}

Point MapData::screenToMapPos(Point const& screenPos) const
{
	const float tileWidth = m_mapInfo->getTileSize().width;
	const float tileHeight = m_mapInfo->getTileSize().height;

	const float x = screenPos.x - m_mapInfo->getMapSize().height * tileWidth / 2;
	const float tileY = screenPos.y / tileHeight;
	const float tileX = x / tileWidth;

	return Point((tileY + tileX) * tileHeight,
		(tileY - tileX) * tileHeight);
}

Point MapData::mapToScreenPos(Point const& mapPos) const
{
	const float tileWidth = m_mapInfo->getTileSize().width;
	const float tileHeight = m_mapInfo->getTileSize().height;
	const float originX = m_mapInfo->getMapSize().height * tileWidth / 2;
	const float tileY = mapPos.y / tileHeight;
	const float tileX = mapPos.x / tileHeight;

	return Point((tileX - tileY) * tileWidth / 2 + originX,
		(tileX + tileY) * tileHeight / 2);
}

void MapData::initDistricts()
{
	NS_ASSERT(!m_districtData);
	int32 width = (int32)this->getMapSize().width;
	int32 height = (int32)this->getMapSize().height;
	int32 nTiles = width * height;
	m_districtData = new uint32[nTiles];
	std::fill(m_districtData, m_districtData + nTiles, 0);

	for (auto extentIt = m_waypointExtents.begin(); extentIt != m_waypointExtents.end(); ++extentIt)
	{
		int32 waypointIndex = (*extentIt).first;
		auto const& extent = (*extentIt).second;
		NS_ASSERT(!extent.empty());

		NS_ASSERT(!extent.empty());
		auto const& waypoint = extent.front();
		NS_ASSERT(waypointIndex == this->getTileIndex(waypoint));
		uint32 districtId = this->getWaypointDistrictId(waypoint);
		NS_ASSERT(districtId != 0);
		for (auto it = extent.begin(); it != extent.end(); ++it)
		{
			auto const& point = *it;
			int32 tileIndex = this->getTileIndex(point);
			m_districtData[tileIndex] = districtId;
		}
	}

	for (auto waypointsIt = m_districtWaypoints.begin(); waypointsIt != m_districtWaypoints.end(); ++waypointsIt)
	{
		uint32 districtId = (*waypointsIt).first;
		auto const& waypoints = (*waypointsIt).second;
		for (auto it = waypoints.begin(); it != waypoints.end(); ++it)
		{
			auto const& waypoint = *it;
			this->fillDistrict(waypoint, districtId);
		}
	}
}


bool MapData::isValidDistrictTileCoord(TileCoord const& tileCoord)
{
	int32 tileIndex = this->getTileIndex(tileCoord);
	return this->isValidTileCoord(tileCoord) && !this->isWall(tileCoord) && !m_districtData[tileIndex];
}

void MapData::fillDistrict(TileCoord const& startCoord, uint32 districtId)
{
	std::vector<TileCoord> queue;
	queue.push_back(startCoord);

	TileCoord newCoord;
	while (queue.size() > 0)
	{
		TileCoord currCoord = queue[queue.size() - 1];
		queue.pop_back();

		newCoord.x = currCoord.x + 1;
		newCoord.y = currCoord.y;
		if (this->isValidDistrictTileCoord(newCoord))
		{
			int32 tileIndex = this->getTileIndex(newCoord);
			m_districtData[tileIndex] = districtId;
			queue.push_back(newCoord);
		}

		newCoord.x = currCoord.x - 1;
		newCoord.y = currCoord.y;
		if (this->isValidDistrictTileCoord(newCoord))
		{
			int32 tileIndex = this->getTileIndex(newCoord);
			m_districtData[tileIndex] = districtId;
			queue.push_back(newCoord);
		}

		newCoord.x = currCoord.x;
		newCoord.y = currCoord.y + 1;
		if (this->isValidDistrictTileCoord(newCoord))
		{
			int32 tileIndex = this->getTileIndex(newCoord);
			m_districtData[tileIndex] = districtId;
			queue.push_back(newCoord);
		}

		newCoord.x = currCoord.x;
		newCoord.y = currCoord.y - 1;
		if (this->isValidDistrictTileCoord(newCoord))
		{
			int32 tileIndex = this->getTileIndex(newCoord);
			m_districtData[tileIndex] = districtId;
			queue.push_back(newCoord);
		}
	}
}
