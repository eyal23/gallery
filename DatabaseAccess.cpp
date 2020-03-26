#include <iostream>
#include <io.h>

#include "DatabaseAccess.h"
#include "Album.h"

#define TABLES_AMOUNT 4

/*
	usage: opens the gallery database
	in: no
	out: if the opening succeeded
*/
bool DatabaseAccess::open()
{
	const std::string dbName = "galleryDB.sqlite";
	sqlite3* db = nullptr;
	int isExisting;

	isExisting = _access(dbName.c_str(), 0);

	if (sqlite3_open(dbName.c_str(), &db) == SQLITE_OK)
	{
		if (isExisting == -1)
		{
			if (!initDatabase())
			{
				db = nullptr;
				return false;
			}
		}

		return true;
	}

	db = nullptr;
	return false;
}

/*
	usage: closes the galery database
	in: no
	out: no
*/
void DatabaseAccess::close()
{
	sqlite3_close(this->_db);
}

/*
	usage: clears all saved objects of the class
	in: no
	out: no
*/
void DatabaseAccess::clear()
{
	for (int i = 0; i < this->_createdUsers.size(); i++)
	{
		delete this->_createdUsers[i];
	}
	for (int i = 0; i < this->_createdAlbum.size(); i++)
	{
		delete this->_createdAlbum[i];
	}
	for (int i = 0; i < this->_createdPicture.size(); i++)
	{
		delete this->_createdPicture[i];
	}
}

/*
	usage: closes an album
	in: reference to the album
	out: no
*/
void DatabaseAccess::deleteAlbum(const std::string& albumName, int userId)
{
	//std::string query = std::string("INSERT INTO")
}


void DatabaseAccess::closeAlbum(Album& pAlbum)
{
	delete &pAlbum;
}


void DatabaseAccess::addPictureToAlbumByName(const std::string& albumName, const Picture& picture)
{
}


void DatabaseAccess::removePictureFromAlbumByName(const std::string& albumName, const std::string& pictureName)
{
}


void DatabaseAccess::tagUserInPicture(const std::string& albumName, const std::string& pictureName, int userId)
{
}


void DatabaseAccess::untagUserInPicture(const std::string& albumName, const std::string& pictureName, int userId)
{
}


void DatabaseAccess::createUser(User& user)
{
}


void DatabaseAccess::deleteUser(const User& user)
{
}


float DatabaseAccess::averageTagsPerAlbumOfUser(const User& user)
{
	return 0.0f;
}

/*
	usage: inits the gallery database
	in: no
	out: if the initialization succeeded
*/
bool DatabaseAccess::initDatabase()
{
	const char* tableCreations[TABLES_AMOUNT] = {
		"CREATE TABLE USERS(ID INTEGER PRIMERY KEY, NAME TEXT NOT NULL);",
		"CREATE TABLE ALBUMS (ID INTEGER PRIMERY KEY, NAME TEXT NOT NULL, CREATION_DATE DATETIME NOT NULL, USER_ID INTEGER FOREIGEN KEY REFERENCES USERS(ID));",
		"CREATE TABLE PICTURES (ID INTEGER PRIMERY KEY, NAME TEXT NOT NULL, LOCATION TEXT NOT NULL, CREATION_DATE DATETIME NOT NULL, ALBUM_ID INTEGER FOREIGON KEY REFERENCES ALBUMS(ID));",
		"CREATE TABLE TAGS (ID INTEGER PRIMERY KEY, PICTURE_ID INTEGER FOREIGON KEY REFERENCES PICTURES(ID), USER_ID INTEGER FOREIGON KEY REFERENCES USERS(ID));",
	};
	char* errBuff = nullptr;

	for (int i = 0; i < TABLES_AMOUNT; i++)
	{
		if (sqlite3_exec(this->_db, tableCreations[i], nullptr, nullptr, &errBuff) != SQLITE_OK)
		{
			return false;
		}
	}

	return true;
}
