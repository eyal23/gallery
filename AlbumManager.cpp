#include "AlbumManager.h"

#include <Windows.h>
#include <WinBase.h>
#include <synchapi.h>
#include <tchar.h>
#include <strsafe.h>
#include <string>
#include <iostream>
#include <signal.h>

#include "Constants.h"
#include "MyException.h"
#include "AlbumNotOpenException.h"

#pragma comment(lib, "Kernel32.lib")

#define PAINT_PATH "\"C:\\WINDOWS\\system32\\mspaint.exe\""
#define IRFANVIEW_PATH "\"C:\\Program Files\\IrfanView\\i_view64.exe\""

BOOL WINAPI consoleHandler(DWORD signal);

//global variables
PROCESS_INFORMATION processInfo;

AlbumManager::AlbumManager(IDataAccess& dataAccess) :
    m_dataAccess(dataAccess), m_nextPictureId(100), m_nextUserId(200)
{
	// Left empty
	m_dataAccess.open();
}

void AlbumManager::executeCommand(CommandType command) {
	try {
		AlbumManager::handler_func_t handler = m_commands.at(command);
		(this->*handler)();
	} catch (const std::out_of_range&) {
			throw MyException("Error: Invalid command[" + std::to_string(command) + "]\n");
	}
}

void AlbumManager::printHelp() const
{
	std::cout << "Supported Album commands:" << std::endl;
	std::cout << "*************************" << std::endl;
	
	for (const struct CommandGroup& group : m_prompts) {
		std::cout << group.title << std::endl;
		std::string space(".  ");
		for (const struct CommandPrompt& command : group.commands) {
			space = command.type < 10 ? ".   " : ".  ";

			std::cout << command.type << space << command.prompt << std::endl;
		}
		std::cout << std::endl;
	}
}


// ******************* Album ******************* 
void AlbumManager::createAlbum()
{
	std::string userIdStr = getInputFromConsole("Enter user id: ");
	int userId = std::stoi(userIdStr);
	if ( !m_dataAccess.doesUserExists(userId) ) {
		throw MyException("Error: Can't create album since there is no user with id [" + userIdStr+"]\n");
	}

	std::string name = getInputFromConsole("Enter album name - ");
	if ( m_dataAccess.doesAlbumExists(name,userId) ) {
		throw MyException("Error: Failed to create album, album with the same name already exists\n");
	}

	Album newAlbum(userId,name);
	m_dataAccess.createAlbum(newAlbum);

	std::cout << "Album [" << newAlbum.getName() << "] created successfully by user@" << newAlbum.getOwnerId() << std::endl;
}

void AlbumManager::openAlbum()
{
	if (isCurrentAlbumSet()) {
		closeAlbum();
	}

	std::string userIdStr = getInputFromConsole("Enter user id: ");
	int userId = std::stoi(userIdStr);
	if ( !m_dataAccess.doesUserExists(userId) ) {
		throw MyException("Error: Can't open album since there is no user with id @" + userIdStr + ".\n");
	}

	std::string name = getInputFromConsole("Enter album name - ");
	if ( !m_dataAccess.doesAlbumExists(name, userId) ) {
		throw MyException("Error: Failed to open album, since there is no album with name:"+name +".\n");
	}

	m_openAlbum = m_dataAccess.openAlbum(name);
    m_currentAlbumName = name;
	// success
	std::cout << "Album [" << name << "] opened successfully." << std::endl;
}

void AlbumManager::closeAlbum()
{
	refreshOpenAlbum();

	std::cout << "Album [" << m_openAlbum.getName() << "] closed successfully." << std::endl;
	m_dataAccess.closeAlbum(m_openAlbum);
	m_currentAlbumName = "";
}

void AlbumManager::deleteAlbum()
{
	std::string userIdStr = getInputFromConsole("Enter user id: ");
	int userId = std::stoi(userIdStr);
	if (!m_dataAccess.doesUserExists(userId)) {
		throw MyException("Error: There is no user with id @" + userIdStr +"\n");
	}

	std::string albumName = getInputFromConsole("Enter album name - ");
	if ( !m_dataAccess.doesAlbumExists(albumName, userId) ) {
		throw MyException("Error: Failed to delete album, since there is no album with name:" + albumName + ".\n");
	}

	// album exist, close album if it is opened
	if ( (isCurrentAlbumSet() ) &&
		 (m_openAlbum.getOwnerId() == userId && m_openAlbum.getName() == albumName) ) {

		closeAlbum();
	}

	m_dataAccess.deleteAlbum(albumName, userId);
	std::cout << "Album [" << albumName << "] @"<< userId <<" deleted successfully." << std::endl;
}

void AlbumManager::listAlbums()
{
	m_dataAccess.printAlbums();
}

void AlbumManager::listAlbumsOfUser()
{
	std::string userIdStr = getInputFromConsole("Enter user id: ");
	int userId = std::stoi(userIdStr);
	if (!m_dataAccess.doesUserExists(userId)) {
		throw MyException("Error: There is no user with id @" + userIdStr + "\n");
	}

	const User& user = m_dataAccess.getUser(userId);
	const std::list<Album>& albums = m_dataAccess.getAlbumsOfUser(user);

	std::cout << "Albums list of user@" << user.getId() << ":" << std::endl;
	std::cout << "-----------------------" << std::endl;

	for (const auto& album : albums) {
		std::cout <<"   + [" << album.getName() <<"] - created on "<< album.getCreationDate() << std::endl;
	}
}


// ******************* Picture ******************* 
void AlbumManager::addPictureToAlbum()
{
	refreshOpenAlbum();

	std::string picName = getInputFromConsole("Enter picture name: ");
	if (m_openAlbum.doesPictureExists(picName) ) {
		throw MyException("Error: Failed to add picture, picture with the same name already exists.\n");
	}
	
	Picture picture(++m_nextPictureId, picName);
	std::string picPath = getInputFromConsole("Enter picture path: ");
	picture.setPath(picPath);

	m_dataAccess.addPictureToAlbumByName(m_openAlbum.getName(), picture);

	std::cout << "Picture [" << picture.getId() << "] successfully added to Album [" << m_openAlbum.getName() << "]." << std::endl;
}

void AlbumManager::removePictureFromAlbum()
{
	refreshOpenAlbum();

	std::string picName = getInputFromConsole("Enter picture name: ");
	if ( !m_openAlbum.doesPictureExists(picName) ) {
		throw MyException("Error: There is no picture with name <" + picName + ">.\n");
	}
	
	auto picture = m_openAlbum.getPicture(picName);
	m_dataAccess.removePictureFromAlbumByName(m_openAlbum.getName(), picture.getName());
	std::cout << "Picture <" << picName << "> successfully removed from Album [" << m_openAlbum.getName() << "]." << std::endl;
}

void AlbumManager::listPicturesInAlbum()
{
	refreshOpenAlbum();

	std::cout << "List of pictures in Album [" << m_openAlbum.getName() 
			  << "] of user@" << m_openAlbum.getOwnerId() <<":" << std::endl;
	
	const std::list<Picture>& albumPictures = m_openAlbum.getPictures();
	for (auto iter = albumPictures.begin(); iter != albumPictures.end(); ++iter) {
		std::cout << "   + Picture [" << iter->getId() << "] - " << iter->getName() << 
			"\tLocation: [" << iter->getPath() << "]\tCreation Date: [" <<
				iter->getCreationDate() << "]\tTags: [" << iter->getTagsCount() << "]" << std::endl;
	}
	std::cout << std::endl;
}

/*
	usage: shows a picture with a selected application
	in: no
	out: no
*/
void AlbumManager::showPicture()
{
	refreshOpenAlbum();

	std::string picName = getInputFromConsole("Enter picture name: ");
	if ( !m_openAlbum.doesPictureExists(picName) ) {
		throw MyException("Error: There is no picture with name <" + picName + ">.\n");
	}
	
	auto pic = m_openAlbum.getPicture(picName);
	if ( !fileExistsOnDisk(pic.getPath()) ) {
		throw MyException("Error: Can't open <" + picName+ "> since it doesnt exist on disk.\n");
	}

	std::cout << "Choose aplication to open the image with-" << std::endl
		<< "1. Paint" << std::endl
		<< "2. IrfanView" << std::endl;

	int choice = stoi(getInputFromConsole("")) - 1;

	if (choice != 0 && choice != 1)
	{
		throw MyException("Error: Ilegal choice.\n");
	}

	std::string beforeOpeningLastWriteDate = getLastModifyDateOfPicture(pic.getPath());

	STARTUPINFO startupInfo;

	ZeroMemory(&startupInfo, sizeof(startupInfo));
	startupInfo.cb = sizeof(startupInfo);
	ZeroMemory(&processInfo, sizeof(processInfo));

	std::string cmdCommand = !choice
		? std::string(PAINT_PATH)
		: std::string(IRFANVIEW_PATH);
	cmdCommand += std::string(" \"") +
		pic.getPath() +
		std::string("\"");

	if (!CreateProcessA(
		NULL,
		(LPSTR)cmdCommand.c_str(),
		NULL,
		NULL,
		FALSE,
		0,
		NULL,
		NULL,
		&startupInfo,
		&processInfo
	))
	{
		throw MyException("Error: Launch application failed.\n");
	}

	if (!SetConsoleCtrlHandler(consoleHandler, TRUE))
	{
		throw MyException("Error: Set CTRL+C handler failed.\n");
	}

	WaitForSingleObject(processInfo.hProcess, INFINITE);

	SetConsoleCtrlHandler(consoleHandler, FALSE);
	CloseHandle(processInfo.hProcess);
	CloseHandle(processInfo.hThread);

	std::string afterClosingLastWriteDate = getLastModifyDateOfPicture(pic.getPath());
	if (afterClosingLastWriteDate != beforeOpeningLastWriteDate)
	{
		std::cout << std::endl << "The picture was modified!" << std::endl;
	}
}

/*
	usage: changes a picture's read attribute
	in: no
	out: no
*/
void AlbumManager::changePictureReadAttribute()
{
	refreshOpenAlbum();

	std::string picName = getInputFromConsole("Enter picture name: ");
	if (!m_openAlbum.doesPictureExists(picName)) {
		throw MyException("Error: There is no picture with name <" + picName + ">.\n");
	}

	auto pic = m_openAlbum.getPicture(picName);
	if (!fileExistsOnDisk(pic.getPath())) {
		throw MyException("Error: Can't open <" + picName + "> since it doesnt exist on disk.\n");
	}

	std::cout << "Do you want the picture to have read-only access?-" << std::endl
		<< "1. yes" << std::endl
		<< "2. no" << std::endl;

	int choice = stoi(getInputFromConsole("")) - 1;

	if (choice != 0 && choice != 1)
	{
		throw MyException("Error: Ilegal choice!");
	}

	SetFileAttributesA(
		pic.getPath().c_str(),
		choice ? FILE_ATTRIBUTE_NORMAL : FILE_ATTRIBUTE_READONLY
	);
}

/*
	usage: creates a copy on disc and db of a picture
	in: no
	out: no
*/
void AlbumManager::createCopyOfPicture()
{
	refreshOpenAlbum();

	std::string picName = getInputFromConsole("Enter picture name: ");
	if (!m_openAlbum.doesPictureExists(picName)) {
		throw MyException("Error: There is no picture with name <" + picName + ">.\n");
	}

	auto pic = m_openAlbum.getPicture(picName);
	if (!fileExistsOnDisk(pic.getPath())) {
		throw MyException("Error: Can't open <" + picName + "> since it doesnt exist on disk.\n");
	}

	HANDLE hOriginPicture = CreateFileA(
		pic.getPath().c_str(),
		GENERIC_READ,
		FILE_SHARE_READ,
		NULL,
		OPEN_EXISTING,
		NULL,
		NULL
	);

	if (hOriginPicture == INVALID_HANDLE_VALUE)
	{
		throw MyException("Error: Opening origin picture failed");
	}

	DWORD pictureSize = GetFileSize(hOriginPicture, NULL);
	BYTE* inBuffer = new BYTE[pictureSize];
	DWORD bytesRead = 0;
	BOOL result;

	result = ReadFile(
		hOriginPicture, 
		inBuffer,
		pictureSize,
		&bytesRead,
		nullptr
	);

	CloseHandle(hOriginPicture);

	if (!result)
	{
		throw MyException("Error: Reading from origin picture failed");
	}

	std::string copyPicturePath = pic.getPath();

	while (copyPicturePath.back() != '\\')
	{
		copyPicturePath.pop_back();
	}

	std::string copyPictureName = std::string("CopyOf_") + pic.getName();
	copyPicturePath += copyPictureName + std::string(".png");

	HANDLE hCopyPicture = CreateFileA(
		copyPicturePath.c_str(),
		GENERIC_WRITE,
		FILE_SHARE_WRITE,
		NULL,
		CREATE_NEW,
		FILE_ATTRIBUTE_NORMAL,
		NULL
	);

	if (hCopyPicture == INVALID_HANDLE_VALUE)
	{
		throw MyException("Error: Creating copy picture failed (most likely because copy already exists)");
	}

	DWORD bytesWritten = 0;

	result = WriteFile(
		hCopyPicture,
		inBuffer,
		pictureSize,
		&bytesWritten,
		nullptr
	);

	CloseHandle(hCopyPicture);
	delete[] inBuffer;

	if (!result)
	{
		throw MyException("Error: Writing into copy picture failed");
	}

	Picture copyPicture(0, copyPictureName);
	copyPicture.setPath(copyPicturePath);

	this->m_dataAccess.addPictureToAlbumByName(this->m_openAlbum.getName(), copyPicture);
}

void AlbumManager::tagUserInPicture()
{
	refreshOpenAlbum();

	std::string picName = getInputFromConsole("Enter picture name: ");
	if ( !m_openAlbum.doesPictureExists(picName) ) {
		throw MyException("Error: There is no picture with name <" + picName + ">.\n");
	}
	
	Picture pic = m_openAlbum.getPicture(picName);
	
	std::string userIdStr = getInputFromConsole("Enter user id to tag: ");
	int userId = std::stoi(userIdStr);
	if ( !m_dataAccess.doesUserExists(userId) ) {
		throw MyException("Error: There is no user with id @" + userIdStr + "\n");
	}
	User user = m_dataAccess.getUser(userId);

	m_dataAccess.tagUserInPicture(m_openAlbum.getName(), pic.getName(), user.getId());
	std::cout << "User @" << userIdStr << " successfully tagged in picture <" << pic.getName() << "> in album [" << m_openAlbum.getName() << "]" << std::endl;
}

void AlbumManager::untagUserInPicture()
{
	refreshOpenAlbum();

	std::string picName = getInputFromConsole("Enter picture name: ");
	if (!m_openAlbum.doesPictureExists(picName)) {
		throw MyException("Error: There is no picture with name <" + picName + ">.\n");
	}

	Picture pic = m_openAlbum.getPicture(picName);

	std::string userIdStr = getInputFromConsole("Enter user id: ");
	int userId = stoi(userIdStr);
	if (!m_dataAccess.doesUserExists(userId)) {
		throw MyException("Error: There is no user with id @" + userIdStr + "\n");
	}
	User user = m_dataAccess.getUser(userId);

	if (! pic.isUserTagged(user)) {
		throw MyException("Error: The user was not tagged! \n");
	}

	m_dataAccess.untagUserInPicture(m_openAlbum.getName(), pic.getName(), user.getId());
	std::cout << "User @" << userIdStr << " successfully untagged in picture <" << pic.getName() << "> in album [" << m_openAlbum.getName() << "]" << std::endl;

}

void AlbumManager::listUserTags()
{
	refreshOpenAlbum();

	std::string picName = getInputFromConsole("Enter picture name: ");
	if ( !m_openAlbum.doesPictureExists(picName) ) {
		throw MyException("Error: There is no picture with name <" + picName + ">.\n");
	}
	auto pic = m_openAlbum.getPicture(picName); 

	const std::set<int> users = pic.getUserTags();

	if ( 0 == users.size() )  {
		throw MyException("Error: There is no user tegged in <" + picName + ">.\n");
	}

	std::cout << "Tagged users in picture <" << picName << ">:" << std::endl;
	for (const int user_id: users) {
		const User user = m_dataAccess.getUser(user_id);
		std::cout << user << std::endl;
	}
	std::cout << std::endl;

}


// ******************* User ******************* 
void AlbumManager::addUser()
{
	std::string name = getInputFromConsole("Enter user name: ");

	User user(++m_nextUserId,name);
	
	m_dataAccess.createUser(user);
	std::cout << "User " << name << " with id @" << user.getId() << "(incorrect id) created successfully." << std::endl;
}


void AlbumManager::removeUser()
{
	// get user name
	std::string userIdStr = getInputFromConsole("Enter user id: ");
	int userId = std::stoi(userIdStr);
	if ( !m_dataAccess.doesUserExists(userId) ) {
		throw MyException("Error: There is no user with id @" + userIdStr + "\n");
	}
	const User& user = m_dataAccess.getUser(userId);
	if (isCurrentAlbumSet() && userId == m_openAlbum.getOwnerId()) {
		closeAlbum();
	}

	std::list<Album> userAlbums = this->m_dataAccess.getAlbumsOfUser(this->m_dataAccess.getUser(userId));

	while (!userAlbums.empty())
	{
		this->m_dataAccess.deleteAlbum(userAlbums.front().getName(), userId);
		userAlbums.pop_front();
	}

	std::list<Album> allAlbums = this->m_dataAccess.getAlbums();

	while (!allAlbums.empty())
	{
		Album album = this->m_dataAccess.openAlbum(allAlbums.front().getName());
		std::list<Picture> currentAlbumPictures = album.getPictures();
		for (std::list<Picture>::iterator currentPicture = currentAlbumPictures.begin(); currentPicture != currentAlbumPictures.end(); ++currentPicture)
		{
			this->m_dataAccess.untagUserInPicture(allAlbums.front().getName(), currentPicture->getName(), userId);
		}
		allAlbums.pop_front();
	}

	m_dataAccess.deleteUser(user);
	std::cout << "User @" << userId << " deleted successfully." << std::endl;
}

void AlbumManager::listUsers()
{
	m_dataAccess.printUsers();	
}

void AlbumManager::userStatistics()
{
	std::string userIdStr = getInputFromConsole("Enter user id: ");
	int userId = std::stoi(userIdStr);
	if ( !m_dataAccess.doesUserExists(userId) ) {
		throw MyException("Error: There is no user with id @" + userIdStr + "\n");
	}

	const User& user = m_dataAccess.getUser(userId);

	std::cout << "user @" << userId << " Statistics:" << std::endl << "--------------------" << std::endl <<
		"  + Count of Albums Owned: " << this->m_dataAccess.getAlbumsOfUser(this->m_dataAccess.getUser(userId)).size() << std::endl <<
		"  + Count of Albums Tagged: " << m_dataAccess.countAlbumsTaggedOfUser(user) << std::endl <<
		"  + Count of Tags: " << m_dataAccess.countTagsOfUser(user) << std::endl <<
		"  + Avarage Tags per Alboum: " << m_dataAccess.averageTagsPerAlbumOfUser(user) << std::endl;
}


// ******************* Queries ******************* 
void AlbumManager::topTaggedUser()
{
	const User& user = m_dataAccess.getTopTaggedUser();

	std::cout << "The top tagged user is: " << user.getName() << std::endl;
}

void AlbumManager::topTaggedPicture()
{
	const Picture& picture = m_dataAccess.getTopTaggedPicture();

	std::cout << "The top tagged picture is: " << picture.getName() << std::endl;
}

void AlbumManager::picturesTaggedUser()
{
	std::string userIdStr = getInputFromConsole("Enter user id: ");
	int userId = std::stoi(userIdStr);
	if ( !m_dataAccess.doesUserExists(userId) ) {
		throw MyException("Error: There is no user with id @" + userIdStr + "\n");
	}

	auto user = m_dataAccess.getUser(userId);

	auto taggedPictures = m_dataAccess.getTaggedPicturesOfUser(user);

	std::cout << "List of pictures that User@" << user.getId() << " tagged :" << std::endl;
	for (const Picture& picture: taggedPictures) {
		std::cout <<"   + "<< picture << std::endl;
	}
	std::cout << std::endl;
}


// ******************* Help & exit ******************* 
void AlbumManager::exit()
{
	std::exit(EXIT_SUCCESS);
}

void AlbumManager::help()
{
	system("CLS");
	printHelp();
}

std::string AlbumManager::getInputFromConsole(const std::string& message)
{
	std::string input;
	do {
		std::cout << message;
		std::getline(std::cin, input);
	} while (input.empty());
	
	return input;
}

bool AlbumManager::fileExistsOnDisk(const std::string& filename)
{
	struct stat buffer;   
	return (stat(filename.c_str(), &buffer) == 0); 
}

void AlbumManager::refreshOpenAlbum() {
	if (!isCurrentAlbumSet()) {
		throw AlbumNotOpenException();
	}
    m_openAlbum = m_dataAccess.openAlbum(m_currentAlbumName);
}

bool AlbumManager::isCurrentAlbumSet() const
{
    return !m_currentAlbumName.empty();
}

/*
	usage: handles a ctrl+c event
	in: the ctrl+c type
	out: if the ctrl+c event was handled
*/
BOOL WINAPI consoleHandler(DWORD signal) 
{
	if (signal == CTRL_C_EVENT)
	{
		TerminateProcess(processInfo.hProcess, 0);
		return TRUE;
	}

	return FALSE;
}

/*
	usage: gets the last time a picture was modified
	in: the picture's path
	out: the last time the picture was modified
*/
std::string AlbumManager::getLastModifyDateOfPicture(std::string picturePath) const
{
	DWORD result;
	HANDLE hPicture = CreateFile(
		picturePath.c_str(),
		GENERIC_READ,
		FILE_SHARE_READ,
		NULL,
		OPEN_EXISTING,
		0,
		NULL
	);

	if (hPicture == INVALID_HANDLE_VALUE)
	{
		throw MyException("Error: Open picture failed.\n");
	}

	FILETIME ftPictureCreationDate, ftPictureLastAccessDate, ftPictureLastWriteDate;

	result = GetFileTime(hPicture, &ftPictureCreationDate, &ftPictureLastAccessDate, &ftPictureLastWriteDate);
	CloseHandle(hPicture);

	if (!result)
	{
		throw MyException("Error: Get last modify date failed.\n");
	}

	SYSTEMTIME utc, localTime;
	TCHAR tPictureLastWriteTime[MAX_PATH];

	FileTimeToSystemTime(&ftPictureLastWriteDate, &utc);
	SystemTimeToTzSpecificLocalTime(NULL, &utc, &localTime);

	result = StringCchPrintf(
		tPictureLastWriteTime, 
		MAX_PATH,
		TEXT("%02d/%02d/%d  %02d:%02d:%02d:%02d"),
		localTime.wMonth,
		localTime.wDay,
		localTime.wYear,
		localTime.wHour,
		localTime.wMinute,
		localTime.wSecond,
		localTime.wMilliseconds
	);

	if (result != S_OK)
	{
		throw MyException("Error: Converting failed.\n");
	}

	return std::string(tPictureLastWriteTime);
}

const std::vector<struct CommandGroup> AlbumManager::m_prompts  = {
	{
		"Supported Albums Operations:\n----------------------------",
		{
			{ CREATE_ALBUM        , "Create album" },
			{ OPEN_ALBUM          , "Open album" },
			{ CLOSE_ALBUM         , "Close album" },
			{ DELETE_ALBUM        , "Delete album" },
			{ LIST_ALBUMS         , "List albums" },
			{ LIST_ALBUMS_OF_USER , "List albums of user" }
		}
	},
	{
		"Supported Album commands (when specific album is open):",
		{
			{ ADD_PICTURE    , "Add picture." },
			{ REMOVE_PICTURE , "Remove picture." },
			{ SHOW_PICTURE   , "Show picture." },
			{ CHANGE_PICTURE_READ_ATTRIBUTE , "Change picture read attribute." },
			{ CREATE_COPY_OF_PICTURE, "Create copy of a picture." },
			{ LIST_PICTURES  , "List pictures." },
			{ TAG_USER		 , "Tag user." },
			{ UNTAG_USER	 , "Untag user." },
			{ LIST_TAGS		 , "List tags." }
		}
	},
	{
		"Supported Users commands: ",
		{
			{ ADD_USER         , "Add user." },
			{ REMOVE_USER      , "Remove user." },
			{ LIST_OF_USER     , "List of users." },
			{ USER_STATISTICS  , "User statistics." },
		}
	},
	{
		"Supported Queries:",
		{
			{ TOP_TAGGED_USER      , "Top tagged user." },
			{ TOP_TAGGED_PICTURE   , "Top tagged picture." },
			{ PICTURES_TAGGED_USER , "Pictures tagged user." },
		}
	},
	{
		"Supported Operations:",
		{
			{ HELP , "Help (clean screen)" },
			{ EXIT , "Exit." },
		}
	}
};

const std::map<CommandType, AlbumManager::handler_func_t> AlbumManager::m_commands = {
	{ CREATE_ALBUM, &AlbumManager::createAlbum },
	{ OPEN_ALBUM, &AlbumManager::openAlbum },
	{ CLOSE_ALBUM, &AlbumManager::closeAlbum },
	{ DELETE_ALBUM, &AlbumManager::deleteAlbum },
	{ LIST_ALBUMS, &AlbumManager::listAlbums },
	{ LIST_ALBUMS_OF_USER, &AlbumManager::listAlbumsOfUser },
	{ ADD_PICTURE, &AlbumManager::addPictureToAlbum },
	{ REMOVE_PICTURE, &AlbumManager::removePictureFromAlbum },
	{ LIST_PICTURES, &AlbumManager::listPicturesInAlbum },
	{ SHOW_PICTURE, &AlbumManager::showPicture },
	{ CHANGE_PICTURE_READ_ATTRIBUTE, &AlbumManager::changePictureReadAttribute },
	{ CREATE_COPY_OF_PICTURE, &AlbumManager::createCopyOfPicture },
	{ TAG_USER, &AlbumManager::tagUserInPicture, },
	{ UNTAG_USER, &AlbumManager::untagUserInPicture },
	{ LIST_TAGS, &AlbumManager::listUserTags },
	{ ADD_USER, &AlbumManager::addUser },
	{ REMOVE_USER, &AlbumManager::removeUser },
	{ LIST_OF_USER, &AlbumManager::listUsers },
	{ USER_STATISTICS, &AlbumManager::userStatistics },
	{ TOP_TAGGED_USER, &AlbumManager::topTaggedUser },
	{ TOP_TAGGED_PICTURE, &AlbumManager::topTaggedPicture },
	{ PICTURES_TAGGED_USER, &AlbumManager::picturesTaggedUser },
	{ HELP, &AlbumManager::help },
	{ EXIT, &AlbumManager::exit }
};
