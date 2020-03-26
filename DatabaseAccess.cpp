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
	usage: deletes an album from the database
	in: the album's name, the album's owner id
	out: no
*/
void DatabaseAccess::deleteAlbum(const std::string& albumName, int userId)
{
	char* errBuff = nullptr;
	std::string deleteAlbumQuery(std::string("DELETE FROM ALBUMS WHERE NAME = ") + 
		albumName + std::string(" AND USER_ID = ") + 
		std::to_string(userId) + 
		std::string(";")
	);

	sqlite3_exec(this->_db, deleteAlbumQuery.c_str(), nullptr, nullptr, &errBuff);
}

/*
	usage: closes an album
	in: reference to the album
	out: no
*/
void DatabaseAccess::closeAlbum(Album& pAlbum)
{
	delete &pAlbum;
}

/*
	usage: adds a picture to an album by name
	in: the album's name, the picture
	out: no
*/
void DatabaseAccess::addPictureToAlbumByName(const std::string& albumName, const Picture& picture)
{
	char* errBuff = nullptr;
	std::string getAlbumIdQuery(std::string("(SELECT ID FROM ALBUMS WHERE NAME = ") + 
		albumName + 
		std::string(" LIMIT 1 OFFSET 0;)")
	);
	std::string addPictureQuery(std::string("INSERT INTO PICTURES (NAME, LOCATION, CREATION_DATE, ALBUM_ID) VALUES(") +
		picture.getName() +
		std::string(", ") +
		picture.getPath() +
		std::string(", ") +
		picture.getCreationDate() +
		std::string(", ") +
		getAlbumIdQuery +
		std::string(");")
	);

	sqlite3_exec(this->_db, addPictureQuery.c_str(), nullptr, nullptr, &errBuff);
}

/*
	usage: removes a picture to an album by name
	in: the album's name, the picture's name
	out: no
*/
void DatabaseAccess::removePictureFromAlbumByName(const std::string& albumName, const std::string& pictureName)
{
	char* errBuff = nullptr;
	std::string getAlbumIdQuery(std::string("(SELECT ID FROM ALBUMS WHERE NAME = ") +
		albumName +
		std::string(" LIMIT 1 OFFSET 0;)")
	);
	std::string deletePictureQuery(std::string("DELETE FROM PICTURES WHERE ALBUM_ID = ") +
		getAlbumIdQuery +
		std::string("AND NAME = ") +
		pictureName +
		std::string(";")
	);

	sqlite3_exec(this->_db, deletePictureQuery.c_str(), nullptr, nullptr, &errBuff);
}

/*
	usage: tags an user to a picture by name
	in: the album's name, the picture's name, the user to untag id
	out: no
*/
void DatabaseAccess::tagUserInPicture(const std::string& albumName, const std::string& pictureName, int userId)
{
	char* errBuff = nullptr;
	std::string getPictureIdQuery(std::string("(SELECT PICTURES.ID FROM ALBUMS INNER JOIN PICTURES ON ALBUMS.ID = PICTURES.ALBUM_ID WHERE ALBUM.NAME = ") +
		albumName +
		std::string("AND PICTURES.NAME = ") +
		pictureName +
		std::string(" LIMIT 1 OFFSET 0;)")
	);
	std::string addTagQuery(std::string("INSERT INTO TAGS (PICTURE_ID, USER_ID) VALUES(") +
		getPictureIdQuery +
		std::string(", ") +
		std::to_string(userId) +
		std::string(");")
	);

	sqlite3_exec(this->_db, addTagQuery.c_str(), nullptr, nullptr, &errBuff);
}

/*
	usage: untags an user from a picture by name
	in: the album's name, the picture's name, the user to untag id
	out: no
*/
void DatabaseAccess::untagUserInPicture(const std::string& albumName, const std::string& pictureName, int userId)
{
	char* errBuff = nullptr;
	std::string getPictureIdQuery(std::string("(SELECT PICTURES.ID FROM ALBUMS INNER JOIN PICTURES ON ALBUMS.ID = PICTURES.ALBUM_ID WHERE ALBUM.NAME = ") +
		albumName +
		std::string("AND PICTURES.NAME = ") +
		pictureName +
		std::string(" LIMIT 1 OFFSET 0;)")
	);
	std::string deleteTagQuery(std::string("DELETE FROM TAGS WHERE PICTURE_ID = ") +
		getPictureIdQuery +
		std::string("AND USER_ID = ") +
		std::to_string(userId) +
		std::string(";")
	);

	sqlite3_exec(this->_db, deleteTagQuery.c_str(), nullptr, nullptr, &errBuff);
}

/*
	usage: created an user
	in: the user
	out: no
*/
void DatabaseAccess::createUser(User& user)
{
	char* errBuff = nullptr;
	std::string createUserQuery(std::string("INSERT INTO USERS (id, NAME) VALUES(") +
		std::to_string(user.getId()) +
		std::string(", ") +
		user.getName() +
		std::string(");")
	);

	sqlite3_exec(this->_db, createUserQuery.c_str(), nullptr, nullptr, &errBuff);
}

/*
	usage: deletes an user
	in: the user
	out: no
*/
void DatabaseAccess::deleteUser(const User& user)
{
	char* errBuff = nullptr;
	std::string deleteUserQuery(std::string("DELETE FROM ALBUMS WHERE ID = ") +
		std::to_string(user.getId()) +
		std::string(";")
	);

	sqlite3_exec(this->_db, deleteUserQuery.c_str(), nullptr, nullptr, &errBuff);
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
		"CREATE TABLE ALBUMS (ID INTEGER PRIMERY KEY AUTOINCREMENT, NAME TEXT NOT NULL, CREATION_DATE DATETIME NOT NULL, USER_ID INTEGER FOREIGEN KEY REFERENCES USERS(ID));",
		"CREATE TABLE PICTURES (ID INTEGER PRIMERY KEY AUTOINCREMENT, NAME TEXT NOT NULL, LOCATION TEXT NOT NULL, CREATION_DATE DATETIME NOT NULL, ALBUM_ID INTEGER FOREIGON KEY REFERENCES ALBUMS(ID));",
		"CREATE TABLE TAGS (ID INTEGER PRIMERY KEY AUTOINCREMENT, PICTURE_ID INTEGER FOREIGON KEY REFERENCES PICTURES(ID), USER_ID INTEGER FOREIGON KEY REFERENCES USERS(ID));",
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
