#pragma once

#include <vector>

#include "sqlite3.h"
#include "IDataAccess.h"
#include "Album.h"
#include "Picture.h"
#include "User.h"

class DatabaseAccess : public IDataAccess
{
private:
	sqlite3* _db;
	std::vector<User*> _createdUsers;
	std::vector<Album*> _createdAlbum;
	std::vector<Picture*> _createdPicture;

public:
	//database related
	bool open() override;
	void close() override;
	void clear() override;

	//album related
	void deleteAlbum(const std::string& albumName, int userId) override;
	void closeAlbum(Album& pAlbum) override;

	//picture related
	void addPictureToAlbumByName(const std::string& albumName, const Picture& picture) override;
	void removePictureFromAlbumByName(const std::string& albumName, const std::string& pictureName) override;
	void tagUserInPicture(const std::string& albumName, const std::string& pictureName, int userId) override;
	void untagUserInPicture(const std::string& albumName, const std::string& pictureName, int userId) override;

	//user related
	void createUser(User& user) override;
	void deleteUser(const User& user) override;

	//statistics related
	float averageTagsPerAlbumOfUser(const User& user) override;

private:
	bool initDatabase();
};

