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
	void closeAlbum(Album& pAlbum) override;

private:
	void initDatabase();
};

