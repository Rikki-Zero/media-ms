#pragma once
#include <string>
#include <vector>
#include "../sqlite3/sqlite3.h"

struct UserInfo
{
    int id;
    std::string username;
    std::string password;
    std::string type;
};

struct BorrowingInfo
{
    int mediaId;
    std::string mediaTitle;
    std::string mediaType;
};

struct BorrowingTransaction
{
    int userId;
    int mediaId;
};

struct UserWithBorrowingInfo
{
    UserInfo user;
    std::vector<BorrowingInfo> borrowedMedia;
};

class DatabaseManager
{
public:
    DatabaseManager(const std::string& dbName);
    ~DatabaseManager();

    bool executeQuery(const std::string& query);
    sqlite3* getDB() const;
    bool addUser(const std::string& username, const std::string& password,
                 const std::string& type);
    bool updateUserPassword(const std::string& username,
                            const std::string& newPassword);
    bool deleteUser(const std::string& username);
    bool selectUser(const std::string& username, std::string& password,
                    std::string& type);
    std::vector<UserWithBorrowingInfo> getAllUsersWithBorrowingInfo();
    std::vector<BorrowingInfo> getBorrowedMediaForUser(int userId);
    bool borrowMedia(int userId, int mediaId);
    bool returnMedia(int userId, int mediaId);

private:
    sqlite3* db;
    int countAdmins();
    bool isAdmin(const std::string& username);
};