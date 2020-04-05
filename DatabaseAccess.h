#pragma once

#include <vector>
#include <deque>

#include "sqlite3.h"
#include "IDataAccess.h"
#include "Album.h"
#include "Picture.h"
#include "User.h"

class DatabaseAccess : public IDataAccess
{
public:
	sqlite3* _db;

public:
	// album related
	const std::list<Album> getAlbums() override;
	const std::list<Album> getAlbumsOfUser(const User& user) override;
    void createAlbum(const Album& album) override;
	void deleteAlbum(const std::string& albumName, int userId) override;
	bool doesAlbumExists(const std::string& albumName, int userId) override;
	Album openAlbum(const std::string& albumName) override;
	void closeAlbum(Album& pAlbum) override;
	void printAlbums() override;

	// picture related
	void addPictureToAlbumByName(const std::string& albumName, const Picture& picture) override;
	void removePictureFromAlbumByName(const std::string& albumName, const std::string& pictureName) override;
	void tagUserInPicture(const std::string& albumName, const std::string& pictureName, int userId) override;
	void untagUserInPicture(const std::string& albumName, const std::string& pictureName, int userId) override;

	// user related
	void printUsers() override;
	User getUser(int userId) override;
	void createUser(User& user) override;
	void deleteUser(const User& user) override;
	bool doesUserExists(int userId) override;

	// user statistics
	int countAlbumsOwnedOfUser(const User& user) override;
	int countAlbumsTaggedOfUser(const User& user) override;
	int countTagsOfUser(const User& user) override;
	float averageTagsPerAlbumOfUser(const User& user) override;

	// queries
	User getTopTaggedUser() override;
	Picture getTopTaggedPicture() override;
	std::list<Picture> getTaggedPicturesOfUser(const User& user) override;

	bool open() override;
	void close() override;
	void clear() override;

private:
	bool initDatabase();
	static int callback(void* data, int argc, char** argv, char** azColName);

	std::string makeInsertQuery(std::string table, std::deque<std::string> values) const;
	std::string makeDeleteQuery(std::string table, std::deque<std::pair<std::string, std::string>> conditions) const;
	std::string makeSelectQuery(std::deque<std::string> tables, std::deque<std::pair<std::string, std::string>> joiners, std::deque<std::string> returnFields, std::deque<std::pair<std::string, std::string>> conditions, bool isWhere, std::string groupBy) const;
};

