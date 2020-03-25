#include <iostream>
#include <io.h>

#include "DatabaseAccess.h"
#include "MyException.h"
#include "Album.h"

#define TABLES_AMOUNT 4

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
			initDatabase();
		}

	}
	else
	{
		db = nullptr;
		throw MyException("Could'nt open database");
	}

	return 0;
}


void DatabaseAccess::close()
{
	sqlite3_close(this->_db);
}


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


void DatabaseAccess::closeAlbum(Album& pAlbum)
{
	delete &pAlbum;
}


void DatabaseAccess::initDatabase()
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
			throw MyException("Could'nt initialize database");
		}
	}
}
