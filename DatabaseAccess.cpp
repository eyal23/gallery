#include <iostream>
#include <io.h>
#include <map>

#include "DatabaseAccess.h"
#include "MyException.h"
#include "Album.h"

#define TABLES_AMOUNT 4
#define USER_ID_COLUMN "USER_ID"
#define USER_NAME_COLUMN "USER_NAME"
#define ALBUM_ID_COLUMN "ALBUM_ID"
#define ALBUM_NAME_COLUMN "ALBUM_NAME"
#define ALBUM_CREATION_DATE_COLUMN "ALBUM_CREATION_DATE"
#define ALBUM_USER_ID_COLUMN "ALBUM_USER_ID"
#define PICTURE_ID_COLUMN "PICTURE_ID"
#define PICTURE_NAME_COLUMN "PICTURE_NAME"
#define PICTURE_LOCATION_COLUMN "PICTURE_LOCATION"
#define PICTURE_CREATION_DATE_COLUMN "PICTURE_CREATION_DATE"
#define PICTURE_ALBUM_ID_COLUMN "PICTURE_ALBUM_ID"
#define TAG_ID_COLUMN "TAG_ID"
#define TAG_PICTURE_ID_COLUMN "TAG_PICTURE_ID"
#define TAG_USER_ID_COLUMN "TAG_USER_ID"

/*
	usage: opens the gallery database
	in: no
	out: if the opening succeeded
*/
bool DatabaseAccess::open()
{
	const std::string dbName = "galleryDB.sqlite";
	int isExisting;

	isExisting = _access(dbName.c_str(), 0);

	if (sqlite3_open(dbName.c_str(), &this->_db) == SQLITE_OK)
	{
		if (isExisting == -1)
		{
			if (!initDatabase())
			{
				this->_db = nullptr;
				return false;
			}
		}

		return true;
	}

	this->_db = nullptr;
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
}

/*
	usage: gets all the albums
	in: no
	out: no
*/
const std::list<Album> DatabaseAccess::getAlbums()
{
	std::list<Album> albums;
	std::deque<std::map<std::string, std::string>> albumsData;
	albumsData.push_back({
		{ALBUM_NAME_COLUMN, ""}, 
		{ALBUM_CREATION_DATE_COLUMN, ""},
		{ALBUM_USER_ID_COLUMN, ""},
	});
	char* errBuff = nullptr;
	std::string allAlbumsQuery = makeSelectQuery(
	    { "ALBUMS" }, 
		{}, 
		{ 
			ALBUM_NAME_COLUMN,
			ALBUM_CREATION_DATE_COLUMN,
			ALBUM_USER_ID_COLUMN
		}, 
		{},
		true,
		""
	);

	sqlite3_exec(this->_db, allAlbumsQuery.c_str(), callback, &albumsData, &errBuff);
	albumsData.pop_back();

	for (int i = 0; i < albumsData.size(); i++)
	{
		albums.push_back(Album(
			atoi(albumsData[i][ALBUM_USER_ID_COLUMN].c_str()), 
			albumsData[i][ALBUM_NAME_COLUMN],
			albumsData[i][ALBUM_CREATION_DATE_COLUMN]
		));
	}

	return albums;
}

/*
	usage: gets all the albums of user
	in: the user
	out: the user's albums
*/
const std::list<Album> DatabaseAccess::getAlbumsOfUser(const User& user)
{
	std::list<Album> albums;
	std::deque<std::map<std::string, std::string>> albumsData;
	albumsData.push_back({ 
		{ALBUM_NAME_COLUMN, ""},
		{ALBUM_CREATION_DATE_COLUMN, ""},
		{ALBUM_USER_ID_COLUMN, ""},
		});
	char* errBuff = nullptr;
	std::string allAlbumsQuery = makeSelectQuery(
		{ "ALBUMS" }, 
		{}, 
		{ 
			ALBUM_NAME_COLUMN,
			ALBUM_CREATION_DATE_COLUMN, 
			ALBUM_USER_ID_COLUMN 
		}, 
		{ 
			{ ALBUM_USER_ID_COLUMN, std::to_string(user.getId()) }
		},
		true,
		""
	);

	sqlite3_exec(this->_db, allAlbumsQuery.c_str(), callback, &albumsData, &errBuff);
	albumsData.pop_back();

	for (int i = 0; i < albumsData.size(); i++)
	{
		albums.push_back(Album(atoi(albumsData[i][ALBUM_USER_ID_COLUMN].c_str()), albumsData[i][ALBUM_NAME_COLUMN], albumsData[i][ALBUM_CREATION_DATE_COLUMN]));
	}

	return albums;
}

/*
	usage: creates an album
	in: the album
	out: no
*/
void DatabaseAccess::createAlbum(const Album& album)
{
	if (!doesAlbumExists(album.getName(), album.getOwnerId()))
	{
		char* errBuff = nullptr;
		std::string createAlbumQuery = makeInsertQuery(
			"ALBUMS",
			{ 
				std::string("\"") + album.getName() + std::string("\""),
				std::string("\"") + album.getCreationDate() + std::string("\""),
				std::to_string(album.getOwnerId())
			}
		);

		sqlite3_exec(this->_db, createAlbumQuery.c_str(), nullptr, nullptr, &errBuff);
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
	std::deque<std::map<std::string, std::string>> picturesData;
	picturesData.push_back({
		{ PICTURE_NAME_COLUMN, "" },
		});
	std::string albumPicturesQuery = makeSelectQuery(
		{
			"ALBUMS",
			"PICTURES",
		},
		{
			{std::string("ALBUMS.") + ALBUM_ID_COLUMN, std::string("PICTURES.") + PICTURE_ALBUM_ID_COLUMN}
		},
		{ std::string("PICTURES.") + PICTURE_NAME_COLUMN },
		{
			{ std::string("ALBUMS.") + ALBUM_NAME_COLUMN, std::string("\"") + albumName + std::string("\"") },
			{ std::string("ALBUMS.") + ALBUM_USER_ID_COLUMN, std::to_string(userId) }
		},
		true,
		""
	);
	std::string deleteAlbumQuery = makeDeleteQuery(
		"ALBUMS", 
		{ 
			{ALBUM_NAME_COLUMN, std::string("\"") + albumName + std::string("\"")}, 
			{ALBUM_USER_ID_COLUMN, std::to_string(userId)} 
		}
	);

	sqlite3_exec(this->_db, albumPicturesQuery.c_str(), callback, &picturesData, &errBuff);
	picturesData.pop_back();
	for (int i = 0; i < picturesData.size(); i++)
	{
		removePictureFromAlbumByName(albumName, picturesData[i][PICTURE_NAME_COLUMN]);
	}
	sqlite3_exec(this->_db, deleteAlbumQuery.c_str(), nullptr, nullptr, &errBuff);
}

/*
	usage: checks if an album exists
	in: the album's name, the owner's id
	out: if the album exists
*/
bool DatabaseAccess::doesAlbumExists(const std::string& albumName, int userId)
{
	std::deque<std::map<std::string, std::string>> albumsData;
	albumsData.push_back({
		{ ALBUM_ID_COLUMN, "" },
		});
	char* errBuff = nullptr;
	std::string allAlbumsQuery = makeSelectQuery(
		{ "ALBUMS" },
		{}, 
		{ ALBUM_ID_COLUMN },
		{
			{ ALBUM_NAME_COLUMN, std::string("\"") + albumName + std::string("\"") },
			{ ALBUM_USER_ID_COLUMN, std::to_string(userId) }
		},
		true,
		""
	);

	sqlite3_exec(this->_db, allAlbumsQuery.c_str(), callback, &albumsData, &errBuff);
	albumsData.pop_back();

	return !albumsData.empty();
}

/*
	usage: open's an album (album with it's full members)
	in: the album's name
	out: the album
*/
Album DatabaseAccess::openAlbum(const std::string& albumName)
{
	Album album;
	std::deque<std::map<std::string, std::string>> albumsAndPicturesData;
	albumsAndPicturesData.push_back({
		{ PICTURE_ID_COLUMN, "" },
		{ PICTURE_NAME_COLUMN, "" },
		{ PICTURE_LOCATION_COLUMN, "" },
		{ PICTURE_CREATION_DATE_COLUMN, "" },
		{ ALBUM_NAME_COLUMN, "" },
		{ ALBUM_CREATION_DATE_COLUMN, "" },
		{ ALBUM_USER_ID_COLUMN, "" },
		});
	char* errBuff = nullptr;
	std::string albumPicturesQuery = makeSelectQuery(
		{
			"ALBUMS",
			"PICTURES",
		},
		{
			{ std::string("ALBUMS.") + ALBUM_ID_COLUMN, std::string("PICTURES.") + PICTURE_ALBUM_ID_COLUMN }
		},
		{ 
			std::string("PICTURES.") + PICTURE_ID_COLUMN ,
			std::string("PICTURES.") + PICTURE_NAME_COLUMN ,
			std::string("PICTURES.") + PICTURE_LOCATION_COLUMN ,
			std::string("PICTURES.") + PICTURE_CREATION_DATE_COLUMN ,
			std::string("ALBUMS.") + ALBUM_NAME_COLUMN,
			std::string("ALBUMS.") + ALBUM_CREATION_DATE_COLUMN,
			std::string("ALBUMS.") + ALBUM_USER_ID_COLUMN,
		},
		{
			{ std::string("ALBUMS.") + ALBUM_NAME_COLUMN, std::string("\"") + albumName + std::string("\"") }
		},
		true,
		""
	);

	sqlite3_exec(this->_db, albumPicturesQuery.c_str(), callback, &albumsAndPicturesData, &errBuff);
	albumsAndPicturesData.pop_back();

	if (albumsAndPicturesData.empty())
	{
		std::deque<std::map<std::string, std::string>> albumsData;
		albumsData.push_back({
			{ ALBUM_NAME_COLUMN, "" },
			{ ALBUM_CREATION_DATE_COLUMN, "" },
			{ ALBUM_USER_ID_COLUMN, "" },
			});
		char* errBuff = nullptr;
		std::string albumQuery = makeSelectQuery(
			{ "ALBUMS" },
			{},
			{ 
				ALBUM_NAME_COLUMN,
				ALBUM_CREATION_DATE_COLUMN,
				ALBUM_USER_ID_COLUMN
			},
			{ 
				{ ALBUM_NAME_COLUMN, std::string("\"") + albumName + std::string("\"") }
			},
			true,
			""
		);

		sqlite3_exec(this->_db, albumQuery.c_str(), callback, &albumsData, &errBuff);
		albumsData.pop_back();

		album = Album(
			atoi((albumsData.front()[ALBUM_USER_ID_COLUMN]).c_str()),
			albumsData.front()[ALBUM_NAME_COLUMN],
			albumsData.front()[ALBUM_CREATION_DATE_COLUMN]
		);

		return album;
	}

	album = Album(
		atoi((albumsAndPicturesData.front()[ALBUM_USER_ID_COLUMN]).c_str()),
		albumsAndPicturesData.front()[ALBUM_NAME_COLUMN],
		albumsAndPicturesData.front()[ALBUM_CREATION_DATE_COLUMN]
	);

	for (int i = 0; i < albumsAndPicturesData.size(); i++)
	{
		Picture picture(
			atoi((albumsAndPicturesData[i][PICTURE_ID_COLUMN]).c_str()), 
			albumsAndPicturesData[i][PICTURE_NAME_COLUMN],
			albumsAndPicturesData[i][PICTURE_LOCATION_COLUMN],
			albumsAndPicturesData[i][PICTURE_CREATION_DATE_COLUMN]
		);
		std::deque<std::map<std::string, std::string>> picturesAndTagsData;
		picturesAndTagsData.push_back({
			{ TAG_USER_ID_COLUMN, "" }
			});
		std::string pictureTagsQuery = makeSelectQuery(
			{
				"PICTURES",
				"TAGS",
			},
			{
				{ std::string("PICTURES.") + PICTURE_ID_COLUMN, std::string("TAGS.") + TAG_PICTURE_ID_COLUMN }
			},
			{ std::string("TAGS.") + TAG_USER_ID_COLUMN },
			{
				{ std::string("PICTURES.") + PICTURE_ID_COLUMN, albumsAndPicturesData[i][PICTURE_ID_COLUMN] }
			},
			true,
			""
		);

		sqlite3_exec(this->_db, pictureTagsQuery.c_str(), callback, &picturesAndTagsData, &errBuff);
		picturesAndTagsData.pop_back();

		for (int i = 0; i < picturesAndTagsData.size(); i++)
		{
			picture.tagUser(atoi((picturesAndTagsData[i][TAG_USER_ID_COLUMN]).c_str()));
		}

		album.addPicture(picture);
	}

	return album;
}

/*
	usage: closes an album
	in: reference to the album
	out: no
*/
void DatabaseAccess::closeAlbum(Album& pAlbum)
{
}

/*
	usage: prints all the albums
	in: no
	out: no
*/
void DatabaseAccess::printAlbums()
{
	std::list<Album> albums = getAlbums();

	if (albums.empty())
	{
		throw MyException("There are no existing albums.");
	}

	std::cout << "Album list:" << std::endl;
	std::cout << "-----------" << std::endl;
	while (!albums.empty())
	{
		std::cout << std::setw(5) << "* " << albums.front();
		albums.pop_front();
	}
}

/*
	usage: adds a picture to an album by name
	in: the album's name, the picture
	out: no
*/
void DatabaseAccess::addPictureToAlbumByName(const std::string& albumName, const Picture& picture)
{
	std::deque<std::map<std::string, std::string>> albumsData;
	albumsData.push_back({
		{ALBUM_ID_COLUMN, ""}
		});
	char* errBuff = nullptr;
	std::string albumIdQuery = makeSelectQuery(
		{ "ALBUMS" },
		{},
		{ ALBUM_ID_COLUMN },
		{
			{ ALBUM_NAME_COLUMN, std::string("\"") + albumName + std::string("\"") }
		},
		true,
		""
	);

	sqlite3_exec(this->_db, albumIdQuery.c_str(), callback, &albumsData, &errBuff);
	albumsData.pop_back();

	std::string addPictureQuery = makeInsertQuery(
		"PICTURES", 
		{
			std::string("\"") + picture.getName() + std::string("\""),
			std::string("\"") + picture.getPath() + std::string("\""),
			std::string("\"") + picture.getCreationDate() + std::string("\""),
			albumsData.front()[ALBUM_ID_COLUMN]
		}
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
	std::deque<std::map<std::string, std::string>> albumsData;
	albumsData.push_back({
		{ALBUM_ID_COLUMN, ""}
		});
	char* errBuff = nullptr;
	std::string albumIdQuery = makeSelectQuery(
		{ "ALBUMS" },
		{},
		{ ALBUM_ID_COLUMN },
		{
			{ ALBUM_NAME_COLUMN, std::string("\"") + albumName + std::string("\"") }
		},
		true,
		""
	);

	sqlite3_exec(this->_db, albumIdQuery.c_str(), callback, &albumsData, &errBuff);
	albumsData.pop_back();

	std::deque<std::map<std::string, std::string>> tagsAndPicturesData;
	tagsAndPicturesData.push_back({
		{ TAG_USER_ID_COLUMN, "" }
		});
	std::string tagsOfPictureQuery = makeSelectQuery(
		{
			"PICTURES",
			"TAGS"
		},
		{
			{ std::string("PICTURES.") + PICTURE_ID_COLUMN, std::string("TAGS.") + TAG_PICTURE_ID_COLUMN }
		},
		{ std::string("TAGS.") + TAG_USER_ID_COLUMN },
		{
			{ std::string("PICTURES.") + PICTURE_NAME_COLUMN, std::string("\"") + pictureName + std::string("\"") },
			{ std::string("PICTURES.") + PICTURE_ALBUM_ID_COLUMN, albumsData.front()[ALBUM_ID_COLUMN] }
		},
		true,
		""
	);
	std::string deletePictureQuery = makeDeleteQuery(
		"PICTURES",
		{
			{ PICTURE_NAME_COLUMN, std::string("\"") + pictureName + std::string("\"") },
			{ PICTURE_ALBUM_ID_COLUMN, albumsData.front()[ALBUM_ID_COLUMN] },
		}
	);

	sqlite3_exec(this->_db, tagsOfPictureQuery.c_str(), callback, &tagsAndPicturesData, &errBuff);
	tagsAndPicturesData.pop_back();
	for (int i = 0; i < tagsAndPicturesData.size(); i++)
	{
		untagUserInPicture(albumName, pictureName, atoi(tagsAndPicturesData[i][TAG_USER_ID_COLUMN].c_str()));
	}
	sqlite3_exec(this->_db, deletePictureQuery.c_str(), nullptr, nullptr, &errBuff);
}

/*
	usage: tags an user to a picture by name
	in: the album's name, the picture's name, the user to untag id
	out: no
*/
void DatabaseAccess::tagUserInPicture(const std::string& albumName, const std::string& pictureName, int userId)
{
	std::deque<std::map<std::string, std::string>> albumsAndPicturesData;
	albumsAndPicturesData.push_back({
		{ PICTURE_ID_COLUMN, "" }
		});
	char* errBuff = nullptr;
	std::string pictureIdQuery = makeSelectQuery(
		{ 
			"ALBUMS",
			"PICTURES"
		},
		{ 
			{ std::string("ALBUMS.") + ALBUM_ID_COLUMN, std::string("PICTURES.") + PICTURE_ALBUM_ID_COLUMN }
		},
		{ std::string("PICTURES.") + PICTURE_ID_COLUMN },
		{
			{ std::string("ALBUMS.") + ALBUM_NAME_COLUMN, std::string("\"") + albumName + std::string("\"") },
			{ std::string("PICTURES.") + PICTURE_NAME_COLUMN, std::string("\"") + pictureName + std::string("\"") }
		},
		true,
		""
	);

	sqlite3_exec(this->_db, pictureIdQuery.c_str(), callback, &albumsAndPicturesData, &errBuff);
	albumsAndPicturesData.pop_back();

	std::string addTagQuery = makeInsertQuery(
		"TAGS", 
		{
			albumsAndPicturesData.front()[PICTURE_ID_COLUMN],
			std::to_string(userId)
		}
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
	std::deque<std::map<std::string, std::string>> albumsAndPicturesData;
	albumsAndPicturesData.push_back({
		{ PICTURE_ID_COLUMN, "" }
		});
	char* errBuff = nullptr;
	std::string pictureIdQuery = makeSelectQuery(
		{
			"ALBUMS",
			"PICTURES"
		},
		{
			{ std::string("ALBUMS.") + ALBUM_ID_COLUMN, std::string("PICTURES.") + PICTURE_ALBUM_ID_COLUMN }
		},
		{ std::string("PICTURES.") + PICTURE_ID_COLUMN },
		{
			{ std::string("ALBUMS.") + ALBUM_NAME_COLUMN, std::string("\"") + albumName + std::string("\"") },
			{ std::string("PICTURES.") + PICTURE_NAME_COLUMN, std::string("\"") + pictureName + std::string("\"") }
		},
		true,
		""
		);

	sqlite3_exec(this->_db, pictureIdQuery.c_str(), callback, &albumsAndPicturesData, &errBuff);
	albumsAndPicturesData.pop_back();

	std::string deleteTagQuery = makeDeleteQuery(
		"TAGS",
		{
			{ std::string("TAGS.") + TAG_PICTURE_ID_COLUMN, albumsAndPicturesData.front()[PICTURE_ID_COLUMN], },
			{ std::string("TAGS.") + TAG_USER_ID_COLUMN, std::to_string(userId) }
		}
	);

	sqlite3_exec(this->_db, deleteTagQuery.c_str(), nullptr, nullptr, &errBuff);
}

/*
	usage: prints all the users
	in: no
	out: no
*/
void DatabaseAccess::printUsers()
{
	std::list<User> users;
	std::deque<std::map<std::string, std::string>> usersData;
	usersData.push_back({
		{USER_ID_COLUMN, ""},
		{USER_NAME_COLUMN, ""}
		});
	char* errBuff = nullptr;
	std::string allUsersQuery = makeSelectQuery(
		{ "USERS" },
		{},
		{
			USER_ID_COLUMN,
			USER_NAME_COLUMN
		},
		{},
		true,
		""
	);

	sqlite3_exec(this->_db, allUsersQuery.c_str(), callback, &usersData, &errBuff);
	usersData.pop_back();

	if (usersData.empty())
	{
		throw MyException("There are no existing users.");
	}

	for (int i = 0; i < usersData.size(); i++)
	{
		users.push_back(User(
			atoi(usersData[i][USER_ID_COLUMN].c_str()),
			usersData[i][USER_NAME_COLUMN]
		));
	}

	std::cout << "Users list:" << std::endl;
	std::cout << "-----------" << std::endl;
	while (!users.empty())
	{
		std::cout << users.front() << std::endl;
		users.pop_front();
	}
}

/*
	usage: gets a user
	in: the user's id
	out: the user
*/
User DatabaseAccess::getUser(int userId)
{
	std::deque<std::map<std::string, std::string>> usersData;
	usersData.push_back({
		{USER_ID_COLUMN, ""},
		{USER_NAME_COLUMN, ""}
		});
	char* errBuff = nullptr;
	std::string getUserQuery = makeSelectQuery(
		{ "USERS" },
		{},
		{
			USER_ID_COLUMN,
			USER_NAME_COLUMN
		},
		{
			{ USER_ID_COLUMN, std::to_string(userId) }
		},
		true,
		""
	);

	sqlite3_exec(this->_db, getUserQuery.c_str(), callback, &usersData, &errBuff);
	usersData.pop_back();

	return User(
		atoi(usersData.front()[USER_ID_COLUMN].c_str()),
		usersData.front()[USER_NAME_COLUMN]
	);
}

/*
	usage: created an user
	in: the user
	out: no
*/
void DatabaseAccess::createUser(User& user)
{
	char* errBuff = nullptr;
	std::string createUserQuery = makeInsertQuery(
		"USERS",
		{
			std::string("\"") + user.getName() + std::string("\"")
		}
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
	std::deque<std::map<std::string, std::string>> usersAndAlbumsData;
	usersAndAlbumsData.push_back({
		{ ALBUM_NAME_COLUMN, "" },
		});
	char* errBuff = nullptr;
	std::string userAlbumsQuery = makeSelectQuery(
		{
			"USERS",
			"ALBUMS"
		},
		{
			{ std::string("USERS.") + USER_ID_COLUMN, std::string("ALBUMS.") + ALBUM_USER_ID_COLUMN }
		},
		{ std::string("ALBUMS.") + ALBUM_NAME_COLUMN },
		{
			{ std::string("USERS.") + USER_ID_COLUMN, std::to_string(user.getId()) }
		},
		true,
		""
	);
	std::string deleteUserQuery = makeDeleteQuery(
		"USERS",
		{
			{ USER_ID_COLUMN, std::to_string(user.getId()) }
		}
	);

	sqlite3_exec(this->_db, userAlbumsQuery.c_str(), callback, &usersAndAlbumsData, &errBuff);
	usersAndAlbumsData.pop_back();
	for (int i = 0; i < usersAndAlbumsData.size(); i++)
	{
		deleteAlbum(usersAndAlbumsData[i][ALBUM_NAME_COLUMN], user.getId());
	}
	sqlite3_exec(this->_db, deleteUserQuery.c_str(), nullptr, nullptr, &errBuff);
}

/*
	usage: checks if an user exists
	in: the user's id
	out: if the user exists
*/
bool DatabaseAccess::doesUserExists(int userId)
{
	std::deque<std::map<std::string, std::string>> usersData;
	usersData.push_back({
		{USER_ID_COLUMN, ""},
		});
	char* errBuff = nullptr;
	std::string getUserQuery = makeSelectQuery(
		{ "USERS" },
		{},
		{ USER_ID_COLUMN },
		{
			{ USER_ID_COLUMN, std::to_string(userId) }
		},
		true,
		""
	);

	sqlite3_exec(this->_db, getUserQuery.c_str(), callback, &usersData, &errBuff);
	usersData.pop_back();

	return !usersData.empty();
}

/*
	usage: counts the amount of albums owned by a user
	in: the user
	out: the amount
*/
int DatabaseAccess::countAlbumsOwnedOfUser(const User& user)
{
	return getAlbumsOfUser(user).size();
}

/*
	usage: counts the amount of albums which a user is taged in
	in: the user
	out: the amount
*/
int DatabaseAccess::countAlbumsTaggedOfUser(const User& user)
{
	int topTaggedIndex = 0;
	std::deque<std::map<std::string, std::string>> tagsAndPicturesAndAlbumsData;
	tagsAndPicturesAndAlbumsData.push_back({
		{ "TAGS_AMOUNT", "" }
		});
	char* errBuff = nullptr;
	std::string albumsWithTagsQuery = makeSelectQuery(
		{
			"TAGS",
			"PICTURES", 
			"ALBUMS"
		},
		{
			{ std::string("TAGS.") + TAG_PICTURE_ID_COLUMN, std::string("PICTURES.") + PICTURE_ID_COLUMN },
			{ std::string("ALBUMS.") + ALBUM_ID_COLUMN, std::string("PICTURES.") + PICTURE_ALBUM_ID_COLUMN }
		},
		{ "COUNT(TAGS.TAG_ID) AS TAGS_AMOUNT" },
		{
			{ "TAGS_AMOUNT", "0" },
			{ std::string("TAGS.") + TAG_USER_ID_COLUMN, std::to_string(user.getId()) }
		},
		false,
		std::string("ALBUMS.") + ALBUM_ID_COLUMN
	);

	sqlite3_exec(this->_db, albumsWithTagsQuery.c_str(), callback, &tagsAndPicturesAndAlbumsData, &errBuff);
	tagsAndPicturesAndAlbumsData.pop_back();

	return tagsAndPicturesAndAlbumsData.size();
}

/*
	usage: counts the amount of times a user is taged
	in: the user
	out: the amount
*/
int DatabaseAccess::countTagsOfUser(const User& user)
{
	std::deque<std::map<std::string, std::string>> tagsData;
	tagsData.push_back({
		{ TAG_ID_COLUMN, "" },
		});
	char* errBuff = nullptr;
	std::string tagsAmountOfUserQuery = makeSelectQuery(
		{ "TAGS" },
		{},
		{ TAG_ID_COLUMN },
		{
			{ TAG_USER_ID_COLUMN, std::to_string(user.getId()) }
		},
		true,
		""
	);

	sqlite3_exec(this->_db, tagsAmountOfUserQuery.c_str(), callback, &tagsData, &errBuff);
	tagsData.pop_back();

	return tagsData.size();
}

/*
	usage: caculates the average of times a user is taged in an album
	in: the user
	out: the average
*/
float DatabaseAccess::averageTagsPerAlbumOfUser(const User& user)
{
	float albumsCounted = countAlbumsTaggedOfUser(user);

	return albumsCounted == 0 
		? 0
		: countTagsOfUser(user) / albumsCounted;
}

/*
	usage: gets the top taged user
	in: no
	out: the user
*/
User DatabaseAccess::getTopTaggedUser()
{
	int topTaggedIndex = 0;
	std::deque<std::map<std::string, std::string>> tagsAndUsersData;
	tagsAndUsersData.push_back({
		{ USER_ID_COLUMN, "" },
		{ USER_NAME_COLUMN, "" },
		{ "TAGS_AMOUNT", "" },
		});
	char* errBuff = nullptr;
	std::string tagsAmountOfUserQuery = makeSelectQuery(
		{ 
			"TAGS",
			"USERS"
		},
		{
			{ std::string("USERS.") + USER_ID_COLUMN, std::string("TAGS.") + TAG_USER_ID_COLUMN }
		},
		{
			std::string("USERS.") + USER_ID_COLUMN,
			std::string("USERS.") + USER_NAME_COLUMN,
			"COUNT(TAGS.TAG_ID) AS TAGS_AMOUNT" 
		},
		{},
		false,
		std::string("USERS.") + USER_ID_COLUMN
	);

	sqlite3_exec(this->_db, tagsAmountOfUserQuery.c_str(), callback, &tagsAndUsersData, &errBuff);
	tagsAndUsersData.pop_back();

	if (tagsAndUsersData.size() == 0)
	{
		throw MyException("There isn't any tagged user.");
	}

	if (tagsAndUsersData.size() > 1)
	{
		for (int i = 1; i < tagsAndUsersData.size(); i++)
		{
			if (atoi(tagsAndUsersData[i]["TAGS_AMOUNT"].c_str()) > atoi(tagsAndUsersData[topTaggedIndex]["TAGS_AMOUNT"].c_str()))
			{
				topTaggedIndex = i;
			}
		}
	}

	return User(
		atoi(tagsAndUsersData[topTaggedIndex][USER_ID_COLUMN].c_str()),
		tagsAndUsersData[topTaggedIndex][USER_NAME_COLUMN]
	);
}

/*
	usage: gets the top taged picture
	in: no
	out: the picture
*/
Picture DatabaseAccess::getTopTaggedPicture()
{
	int topTaggedIndex = 0;
	std::deque<std::map<std::string, std::string>> tagsAndPicturesData;
	tagsAndPicturesData.push_back({
		{ PICTURE_ID_COLUMN, "" },
		{ PICTURE_NAME_COLUMN, "" },
		{ PICTURE_LOCATION_COLUMN, "" },
		{ PICTURE_CREATION_DATE_COLUMN, "" },
		{ "TAGS_AMOUNT", "" },
		});
	char* errBuff = nullptr;
	std::string tagsAmountOfPictureQuery = makeSelectQuery(
		{
			"TAGS",
			"PICTURES"
		},
		{
			{ std::string("PICTURES.") + PICTURE_ID_COLUMN, std::string("TAGS.") + TAG_PICTURE_ID_COLUMN }
		},
		{
			std::string("PICTURES.") + PICTURE_ID_COLUMN,
			std::string("PICTURES.") + PICTURE_NAME_COLUMN,
			std::string("PICTURES.") + PICTURE_LOCATION_COLUMN,
			std::string("PICTURES.") + PICTURE_CREATION_DATE_COLUMN,
			"COUNT(TAGS.TAG_ID) AS TAGS_AMOUNT"
		},
		{},
		false,
		std::string("PICTURES.") + PICTURE_ID_COLUMN
	);

	sqlite3_exec(this->_db, tagsAmountOfPictureQuery.c_str(), callback, &tagsAndPicturesData, &errBuff);
	tagsAndPicturesData.pop_back();

	if (tagsAndPicturesData.size() == 0)
	{
		throw MyException("There isn't any tagged picture.");
	}

	if (tagsAndPicturesData.size() > 1)
	{
		for (int i = 1; i < tagsAndPicturesData.size(); i++)
		{
			if (atoi(tagsAndPicturesData[i]["TAGS_AMOUNT"].c_str()) > atoi(tagsAndPicturesData[topTaggedIndex]["TAGS_AMOUNT"].c_str()))
			{
				topTaggedIndex = i;
			}
		}
	}

	return Picture(
		atoi(tagsAndPicturesData[topTaggedIndex][PICTURE_ID_COLUMN].c_str()),
		tagsAndPicturesData[topTaggedIndex][PICTURE_NAME_COLUMN],
		tagsAndPicturesData[topTaggedIndex][PICTURE_LOCATION_COLUMN],
		tagsAndPicturesData[topTaggedIndex][PICTURE_CREATION_DATE_COLUMN]
	);
}

/*
	usage: gets all the pictures a user is taged in
	in: the user
	out: the pictures
*/
std::list<Picture> DatabaseAccess::getTaggedPicturesOfUser(const User& user)
{
	std::deque<std::map<std::string, std::string>> tagsAndPicturesData;
	tagsAndPicturesData.push_back({
		{ PICTURE_ID_COLUMN, "" },
		{ PICTURE_NAME_COLUMN, "" },
		{ PICTURE_LOCATION_COLUMN, "" },
		{ PICTURE_CREATION_DATE_COLUMN, "" },
		});
	char* errBuff = nullptr;
	std::list<Picture> pictures;
	std::string picturesOfTagsQuery = makeSelectQuery(
		{
			"TAGS",
			"PICTURES"
		},
		{
			{ std::string("TAGS.") + TAG_PICTURE_ID_COLUMN, std::string("PICTURES.") + PICTURE_ID_COLUMN }
		},
		{
			std::string("PICTURES.") + PICTURE_ID_COLUMN ,
			std::string("PICTURES.") + PICTURE_NAME_COLUMN ,
			std::string("PICTURES.") + PICTURE_LOCATION_COLUMN ,
			std::string("PICTURES.") + PICTURE_CREATION_DATE_COLUMN ,
		},
		{
			{ std::string("TAGS.") + TAG_USER_ID_COLUMN, std::to_string(user.getId()) }
		},
		true,
		""
	);

	sqlite3_exec(this->_db, picturesOfTagsQuery.c_str(), callback, &tagsAndPicturesData, &errBuff);
	tagsAndPicturesData.pop_back();

	for (int i = 0; i < tagsAndPicturesData.size(); i++)
	{
		pictures.push_back(Picture(
			atoi(tagsAndPicturesData[i][PICTURE_ID_COLUMN].c_str()),
			tagsAndPicturesData[i][PICTURE_NAME_COLUMN],
			tagsAndPicturesData[i][PICTURE_LOCATION_COLUMN],
			tagsAndPicturesData[i][PICTURE_CREATION_DATE_COLUMN]
		));
	}

	return pictures;
}

/*
	usage: inits the gallery database
	in: no
	out: if the initialization succeeded
*/
bool DatabaseAccess::initDatabase()
{
	const char* tableCreations[TABLES_AMOUNT] = {
		"CREATE TABLE USERS (USER_ID INTEGER PRIMARY KEY AUTOINCREMENT , USER_NAME TEXT NOT NULL);",
		"CREATE TABLE ALBUMS (ALBUM_ID INTEGER PRIMARY KEY AUTOINCREMENT , ALBUM_NAME TEXT NOT NULL, ALBUM_CREATION_DATE DATETIME NOT NULL , ALBUM_USER_ID INTEGER FOREIGEN KEY REFERENCES USERS(USER_ID));",
		"CREATE TABLE PICTURES (PICTURE_ID INTEGER PRIMARY KEY AUTOINCREMENT , PICTURE_NAME TEXT NOT NULL , PICTURE_LOCATION TEXT NOT NULL , PICTURE_CREATION_DATE DATETIME NOT NULL , PICTURE_ALBUM_ID INTEGER FOREIGON KEY REFERENCES ALBUMS(ALBUM_ID));",
		"CREATE TABLE TAGS (TAG_ID INTEGER PRIMARY KEY AUTOINCREMENT , TAG_PICTURE_ID INTEGER FOREIGON KEY REFERENCES PICTURES(ID) , TAG_USER_ID INTEGER FOREIGON KEY REFERENCES USERS(ID));",
	};
	char* errBuff = nullptr;

	for (int i = 0; i < TABLES_AMOUNT; i++)
	{
		if (int res = sqlite3_exec(this->_db, tableCreations[i], nullptr, nullptr, &errBuff) != SQLITE_OK)
		{
			//return false;
			std::cout << res;
		}
	}

	return true;
}

/*
	usage: callback function for sqlite3_excx function 
	in: the deque to save the data at, the amount of columns, the values, the columns
	out: 0
*/
int DatabaseAccess::callback(void* data, int argc, char** argv, char** azColName)
{
	std::deque<std::map<std::string, std::string>>* pData = (std::deque<std::map<std::string, std::string>>*)data;
	
	for (int i = 0; i < argc; i++)
	{
		if (pData->back().count(azColName[i]) > 0)
		{
			pData->back()[azColName[i]] = argv[i];
		}
	}

	pData->push_back(pData->back());

	return 0;
}

/*
	usage: makes an sql insert query
	in: the table to insert to, the values
	out: the query
*/
std::string DatabaseAccess::makeInsertQuery(std::string table, std::deque<std::string> values) const
{
	std::string query = std::string("INSERT INTO ") +
		table +
		std::string(" (");

	if (table == "USERS")
	{
		query += "USER_NAME) VALUES (";
	}
	else if (table == "ALBUMS")
	{
		query += "ALBUM_NAME , ALBUM_CREATION_DATE , ALBUM_USER_ID) VALUES (";
	}
	else if (table == "PICTURES")
	{
		query += "PICTURE_NAME , PICTURE_LOCATION , PICTURE_CREATION_DATE , PICTURE_ALBUM_ID) VALUES (";
	}
	else 
	{
		query += "TAG_PICTURE_ID , TAG_USER_ID) VALUES (";
	}

	for (int i = 0; i < values.size(); i++)
	{
		query += values[i];
		if (i < values.size() - 1)
		{
			query += " , ";
		}
		else
		{
			query += ");";
		}
	}

	return query;
}

/*
	usage: makes an sql delete query
	in: the table to delete from, the conditions
	out: the query
*/
std::string DatabaseAccess::makeDeleteQuery(std::string table, std::deque<std::pair<std::string, std::string>> conditions) const
{
	std::string query = std::string("DELETE FROM ") +
		table +
		std::string(" WHERE ");

	for (int i = 0; i < conditions.size(); i++)
	{
		query += conditions[i].first +
			std::string(" = ") +
			conditions[i].second;
		if (i < conditions.size() - 1)
		{
			query += " AND ";
		}
		else
		{
			query += ";";
		}
	}

	return query;
}

/*
	usage: makes an sql select query
	in: the tables to select from, the joiners between tables, the fields to return, the conditions, if to use "WHERE" or "HAVING", how to groug by
	out: the query
*/
std::string DatabaseAccess::makeSelectQuery(std::deque<std::string> tables, std::deque<std::pair<std::string, std::string>> joiners, std::deque<std::string> returnFields, std::deque<std::pair<std::string, std::string>> conditions, bool isWhere, std::string groupBy) const
{
	std::string query = "SELECT ";

	for (int i = 0; i < returnFields.size(); i++)
	{
		query += returnFields[i];
		if (i < returnFields.size() - 1)
		{
			query += ",";
		}
		query += " ";
	}

	query += std::string("FROM ") +
		tables[0] +
		std::string(" ");

	for (int i = 1; i < tables.size(); i++)
	{
		query += std::string("INNER JOIN ") +
			tables[i] +
			std::string(" ON ") +
			joiners[i - 1].first +
			std::string(" = ") +
			joiners[i - 1].second +
			std::string(" ");
	}

	if (conditions.size() > 0)
	{
		if (!isWhere && conditions.size() > 1 || isWhere)
		{
			query += "WHERE ";
			int i = 0;
			for (i = isWhere ? 0 : 1; i < conditions.size(); i++)
			{
				query += conditions[i].first +
					std::string(" = ") +
					conditions[i].second;
				if (i < conditions.size() - 1)
				{
					query += " AND ";
				}
			}
		}
	}

	if (!isWhere)
	{
		query += std::string(" GROUP BY ") +
			groupBy;

		if (conditions.size() > 0)
		{
			query += std::string(" HAVING ") +
				conditions[0].first +
				std::string(" > ") +
				conditions[0].second;
		}
	}

	query += ";";

	return query;
}