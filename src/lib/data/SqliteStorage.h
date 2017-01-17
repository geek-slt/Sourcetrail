#ifndef SQLITE_STORAGE_H
#define SQLITE_STORAGE_H

#include <memory>
#include <string>
#include <vector>

#include "sqlite/CppSQLite3.h"

#include "data/location/TokenLocationFile.h"
#include "data/location/TokenLocationCollection.h"
#include "data/name/NameHierarchy.h"
#include "data/StorageTypes.h"
#include "data/SqliteIndex.h"
#include "utility/file/FilePath.h"
#include "utility/types.h"
#include "utility/utility.h"
#include "utility/utilityString.h"

class TextAccess;
class Version;
struct ParseLocation;

class SqliteStorage
{
public:
	enum StorageModeType
	{
		STORAGE_MODE_UNKNOWN = 0,
		STORAGE_MODE_READ = 1,
		STORAGE_MODE_WRITE = 2,
		STORAGE_MODE_CLEAR = 4,
	};

	SqliteStorage(const FilePath& dbFilePath);
	~SqliteStorage();

	void setup();
	void clear();

	void setMode(const StorageModeType mode);

	void beginTransaction();
	void commitTransaction();
	void rollbackTransaction();

	void optimizeMemory() const;

	FilePath getDbFilePath() const;

	bool isEmpty() const;
	bool isIncompatible() const;
	std::string getProjectSettingsText() const;
	void setProjectSettingsText(std::string text);

	void setVersion();

	Id addEdge(int type, Id sourceNodeId, Id targetNodeId);

private:
	Id addNode(int type, const std::string& serializedName);
public:
	Id addSymbol(int type, const std::string& serializedName, int definitionType);
	Id addFile(const std::string& serializedName, const std::string& filePath, const std::string& modificationTime);
	Id addLocalSymbol(const std::string& name);
	Id addSourceLocation(Id fileNodeId, uint startLine, uint startCol, uint endLine, uint endCol, int type);
	bool addOccurrence(Id elementId, Id sourceLocationId);
	Id addComponentAccess(Id nodeId, int type);
	Id addCommentLocation(Id fileNodeId, uint startLine, uint startCol, uint endLine, uint endCol);
	Id addError(const std::string& message, const FilePath& filePath, uint lineNumber, uint columnNumber, bool fatal, bool indexed);

	void removeElement(Id id);
	void removeElements(const std::vector<Id>& ids);
	void removeElementsWithLocationInFiles(const std::vector<Id>& fileIds);

	void removeErrorsInFiles(const std::vector<FilePath>& filePaths);

	bool isEdge(Id elementId) const;
	bool isNode(Id elementId) const;
	bool isFile(Id elementId) const;

	StorageEdge getEdgeBySourceTargetType(Id sourceId, Id targetId, int type) const;

	std::vector<StorageEdge> getEdgesBySourceId(Id sourceId) const;
	std::vector<StorageEdge> getEdgesBySourceIds(const std::vector<Id>& sourceIds) const;
	std::vector<StorageEdge> getEdgesByTargetId(Id targetId) const;
	std::vector<StorageEdge> getEdgesByTargetIds(const std::vector<Id>& targetIds) const;
	std::vector<StorageEdge> getEdgesBySourceOrTargetId(Id id) const;

	std::vector<StorageEdge> getEdgesByType(int type) const;
	std::vector<StorageEdge> getEdgesBySourceType(Id sourceId, int type) const;
	std::vector<StorageEdge> getEdgesByTargetType(Id targetId, int type) const;
	std::vector<StorageEdge> getEdgesByTargetType(const std::vector<Id>& targetIds, int type) const;

	StorageNode getNodeBySerializedName(const std::string& serializedName) const;
	StorageSymbol getSymbolBySerializedName(const std::string& serializedName) const;

	StorageLocalSymbol getLocalSymbolByName(const std::string& name) const;

	StorageFile getFileByPath(const std::string& filePath) const;

	std::vector<StorageFile> getFilesByPaths(const std::vector<FilePath>& filePaths) const;
	std::shared_ptr<TextAccess> getFileContentByPath(const std::string& filePath) const;
	std::shared_ptr<TextAccess> getFileContentById(Id fileId) const;

	void setNodeType(int type, Id nodeId);
	void setSymbolDefinitionType(int definitionType, Id symbolId);

	StorageSourceLocation getSourceLocationByAll(const Id fileNodeId, const uint startLine, const uint startCol, const uint endLine, const uint endCol, const int type) const;
	std::shared_ptr<TokenLocationFile> getTokenLocationsForFile(const FilePath& filePath) const;

	std::vector<StorageOccurrence> getOccurrencesForLocationId(Id locationId) const;
	std::vector<StorageOccurrence> getOccurrencesForLocationIds(const std::vector<Id>& locationIds) const;
	std::vector<StorageOccurrence> getOccurrencesForElementIds(const std::vector<Id>& elementIds) const;

	StorageComponentAccess getComponentAccessByNodeId(Id memberEdgeId) const;
	std::vector<StorageComponentAccess> getComponentAccessesByNodeIds(const std::vector<Id>& memberEdgeIds) const;

	std::vector<StorageCommentLocation> getCommentLocationsInFile(const FilePath& filePath) const;

	template <typename ResultType>
	std::vector<ResultType> getAll() const
	{
		return doGetAll<ResultType>("");
	}

	template <typename ResultType>
	ResultType getFirstById(const Id id) const
	{
		if (id != 0)
		{
			return doGetFirst<ResultType>("WHERE id == " + std::to_string(id));
		}
		return ResultType();
	}

	template <typename ResultType>
	std::vector<ResultType> getAllByIds(const std::vector<Id>& ids) const
	{
		return doGetAll<ResultType>("WHERE id IN (" + utility::join(utility::toStrings(ids), ',') + ")");
	}

	int getNodeCount() const;
	int getEdgeCount() const;
	int getFileCount() const;
	int getFileLineSum() const;
	int getSourceLocationCount() const;

private:
	static const size_t STORAGE_VERSION;

	void clearTables();
	void setupTables();

	void executeStatement(const std::string& statement) const;
	void executeStatement(CppSQLite3Statement& statement) const;
	int executeScalar(const std::string& statement) const;
	CppSQLite3Query executeQuery(const std::string& statement) const;
	CppSQLite3Query executeQuery(CppSQLite3Statement& statement) const;

	bool hasTable(const std::string& tableName) const;

	std::string getMetaValue(const std::string& key) const;
	void insertOrUpdateMetaValue(const std::string& key, const std::string& value);

	size_t getStorageVersion() const;
	void setStorageVersion();

	Version getApplicationVersion() const;
	void setApplicationVersion();

	template <typename ResultType>
	std::vector<ResultType> doGetAll(const std::string& query) const;

	template <typename ResultType>
	ResultType doGetFirst(const std::string& query) const
	{
		std::vector<ResultType> results = doGetAll<ResultType>(query + " LIMIT 1");
		if (results.size() > 0)
		{
			return results[0];
		}
		return ResultType();
	}

	mutable CppSQLite3DB m_database;
	FilePath m_dbFilePath;

	StorageModeType m_mode;
	std::vector<std::pair<int, SqliteIndex>> m_indices;
};

template <>
StorageSymbol SqliteStorage::getFirstById<StorageSymbol>(const Id id) const;
template <>
StorageFile SqliteStorage::getFirstById<StorageFile>(const Id id) const;

template <>
std::vector<StorageSymbol> SqliteStorage::getAllByIds<StorageSymbol>(const std::vector<Id>& ids) const;
template <>
std::vector<StorageFile> SqliteStorage::getAllByIds<StorageFile>(const std::vector<Id>& ids) const;

template <>
std::vector<StorageEdge> SqliteStorage::doGetAll<StorageEdge>(const std::string& query) const;
template <>
std::vector<StorageNode> SqliteStorage::doGetAll<StorageNode>(const std::string& query) const;
template <>
std::vector<StorageSymbol> SqliteStorage::doGetAll<StorageSymbol>(const std::string& query) const;
template <>
std::vector<StorageFile> SqliteStorage::doGetAll<StorageFile>(const std::string& query) const;
template <>
std::vector<StorageLocalSymbol> SqliteStorage::doGetAll<StorageLocalSymbol>(const std::string& query) const;
template <>
std::vector<StorageSourceLocation> SqliteStorage::doGetAll<StorageSourceLocation>(const std::string& query) const;
template <>
std::vector<StorageOccurrence> SqliteStorage::doGetAll<StorageOccurrence>(const std::string& query) const;
template <>
std::vector<StorageComponentAccess> SqliteStorage::doGetAll<StorageComponentAccess>(const std::string& query) const;
template <>
std::vector<StorageCommentLocation> SqliteStorage::doGetAll<StorageCommentLocation>(const std::string& query) const;
template <>
std::vector<StorageError> SqliteStorage::doGetAll<StorageError>(const std::string& query) const;


#endif // SQLITE_STORAGE_H
