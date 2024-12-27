#include "database.h"
#include <iostream>
#include <sstream>

DatabaseManager::DatabaseManager(const std::string &dbName) : db(nullptr)
{
    int rc = sqlite3_open(dbName.c_str(), &db);
    if (rc)
    {
        std::cerr << "Can't open database: " << sqlite3_errmsg(db) << std::endl;
        sqlite3_close(db);
        db = nullptr;
    }
    else
    {
        std::cout << "Opened database successfully" << std::endl;
    }
}

DatabaseManager::~DatabaseManager()
{
    if (db)
    {
        sqlite3_close(db);
        std::cout << "Closed database connection" << std::endl;
    }
}

bool DatabaseManager::executeQuery(const std::string &query)
{
    if (!db)
    {
        std::cerr << "Database is not open." << std::endl;
        return false;
    }

    char *errMsg = nullptr;
    int rc = sqlite3_exec(db, query.c_str(), nullptr, nullptr, &errMsg);
    if (rc != SQLITE_OK)
    {
        std::cerr << "SQL error: " << errMsg << std::endl;
        sqlite3_free(errMsg);
        return false;
    }
    return true;
}

sqlite3 *DatabaseManager::getDB() const { return db; }

bool DatabaseManager::addUser(const std::string &username,
                              const std::string &password,
                              const std::string &type)
{
    if (!db)
    {
        std::cerr << "Database is not open." << std::endl;
        return false;
    }
    std::string query =
        "INSERT INTO users (username, password, type) VALUES ('" + username +
        "', '" + password + "', '" + type + "');";

    char *errMsg = nullptr;
    int rc = sqlite3_exec(db, query.c_str(), nullptr, nullptr, &errMsg);
    if (rc != SQLITE_OK)
    {
        std::cerr << "SQL error: " << errMsg << std::endl;
        sqlite3_free(errMsg);
        return false;
    }
    return true;
}

bool DatabaseManager::updateUserPassword(const std::string &username,
                                         const std::string &newPassword)
{
    if (!db)
    {
        std::cerr << "Database is not open." << std::endl;
        return false;
    }
    std::string query = "UPDATE users SET password = '" + newPassword +
                        "' WHERE username = '" + username + "';";

    char *errMsg = nullptr;
    int rc = sqlite3_exec(db, query.c_str(), nullptr, nullptr, &errMsg);
    if (rc != SQLITE_OK)
    {
        std::cerr << "SQL error: " << errMsg << std::endl;
        sqlite3_free(errMsg);
        return false;
    }
    return true;
}

int DatabaseManager::countAdmins()
{
    if (!db)
    {
        std::cerr << "Database is not open." << std::endl;
        return 0;
    }
    std::string query = "SELECT COUNT(*) FROM users WHERE type = 'admin';";
    sqlite3_stmt *stmt;
    int count = 0;
    int rc = sqlite3_prepare_v2(db, query.c_str(), -1, &stmt, nullptr);
    if (rc == SQLITE_OK)
    {
        if (sqlite3_step(stmt) == SQLITE_ROW)
        {
            count = sqlite3_column_int(stmt, 0);
        }
        sqlite3_finalize(stmt);
    }
    else
    {
        std::cerr << "SQL error: " << sqlite3_errmsg(db) << std::endl;
    }
    return count;
}

bool DatabaseManager::isAdmin(const std::string &username)
{
    if (!db)
    {
        std::cerr << "Database is not open." << std::endl;
        return false;
    }
    std::string query = "SELECT 1 FROM users WHERE username = '" + username +
                        "' AND type = 'admin';";
    sqlite3_stmt *stmt;
    bool isAdmin = false;
    int rc = sqlite3_prepare_v2(db, query.c_str(), -1, &stmt, nullptr);
    if (rc == SQLITE_OK)
    {
        if (sqlite3_step(stmt) == SQLITE_ROW)
        {
            isAdmin = true;
        }
        sqlite3_finalize(stmt);
    }
    else
    {
        std::cerr << "错误: " << sqlite3_errmsg(db) << std::endl;
    }
    return isAdmin;
}

bool DatabaseManager::deleteUser(const std::string &username)
{
    if (!db)
    {
        std::cerr << "Database is not open." << std::endl;
        return false;
    }

    if (isAdmin(username) && countAdmins() <= 1)
    {
        std::cout << "[禁止操作] 如果删除该账户，系统将没有任何可用的管理员"
                  << std::endl;
        std::cout << "回车后继续..." << std::endl;
        std::cin.ignore(); // Wait for Enter key
        return false;
    }

    std::string query =
        "DELETE FROM users WHERE username = '" + username + "';";
    char *errMsg = nullptr;
    int rc = sqlite3_exec(db, query.c_str(), nullptr, nullptr, &errMsg);
    if (rc != SQLITE_OK)
    {
        std::cerr << "SQL error: " << errMsg << std::endl;
        sqlite3_free(errMsg);
        return false;
    }
    return true;
}

bool DatabaseManager::selectUser(const std::string &username,
                                 std::string &password, std::string &type)
{
    if (!db)
    {
        std::cerr << "Database is not open." << std::endl;
        return false;
    }
    std::string query =
        "SELECT password, type FROM users WHERE username = '" + username + "';";
    sqlite3_stmt *stmt;
    int rc = sqlite3_prepare_v2(db, query.c_str(), -1, &stmt, nullptr);
    if (rc == SQLITE_OK)
    {
        if (sqlite3_step(stmt) == SQLITE_ROW)
        {
            password =
                reinterpret_cast<const char *>(sqlite3_column_text(stmt, 0));
            type = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 1));
            sqlite3_finalize(stmt);
            return true;
        }
        sqlite3_finalize(stmt);
    }
    else
    {
        std::cerr << "SQL error: " << sqlite3_errmsg(db) << std::endl;
        sqlite3_finalize(stmt);
    }
    return false;
}

std::vector<BorrowingInfo> DatabaseManager::getBorrowedMediaForUser(int userId)
{
    std::vector<BorrowingInfo> borrowedMedia;
    if (!db)
    {
        std::cerr << "Database is not open." << std::endl;
        return borrowedMedia;
    }
    std::string query =
        "SELECT m.id, m.title, m.type FROM borrowing b JOIN media m ON "
        "b.media_id = m.id WHERE b.user_id = " +
        std::to_string(userId) + ";";
    sqlite3_stmt *stmt;
    int rc = sqlite3_prepare_v2(db, query.c_str(), -1, &stmt, nullptr);
    if (rc == SQLITE_OK)
    {
        while (sqlite3_step(stmt) == SQLITE_ROW)
        {
            BorrowingInfo borrowInfo;
            borrowInfo.mediaId = sqlite3_column_int(stmt, 0);
            borrowInfo.mediaTitle =
                reinterpret_cast<const char *>(sqlite3_column_text(stmt, 1));
            borrowInfo.mediaType =
                reinterpret_cast<const char *>(sqlite3_column_text(stmt, 2));
            borrowedMedia.push_back(borrowInfo);
        }
        sqlite3_finalize(stmt);
    }
    else
    {
        std::cerr << "SQL error: " << sqlite3_errmsg(db) << std::endl;
    }
    return borrowedMedia;
}

bool DatabaseManager::borrowMedia(int userId, int mediaId)
{
    if (!db)
    {
        std::cerr << "Database is not open." << std::endl;
        return false;
    }

    // 检查是否已经借阅
    std::string checkQuery =
        "SELECT 1 FROM borrowing WHERE user_id = " + std::to_string(userId) +
        " AND media_id = " + std::to_string(mediaId) + ";";
    sqlite3_stmt *checkStmt;
    sqlite3_prepare_v2(db, checkQuery.c_str(), -1, &checkStmt, nullptr);
    bool alreadyBorrowed = sqlite3_step(checkStmt) == SQLITE_ROW;
    sqlite3_finalize(checkStmt);

    if (alreadyBorrowed)
    {
        std::cout << "该用户已借阅此媒体。" << std::endl;
        return false;
    }

    std::string insertQuery =
        "INSERT INTO borrowing (user_id, media_id) VALUES (" +
        std::to_string(userId) + ", " + std::to_string(mediaId) + ");";
    char *errMsg = nullptr;
    int rc = sqlite3_exec(db, insertQuery.c_str(), nullptr, nullptr, &errMsg);
    if (rc != SQLITE_OK)
    {
        std::cerr << "SQL error: " << errMsg << std::endl;
        sqlite3_free(errMsg);
        return false;
    }
    return true;
}

bool DatabaseManager::returnMedia(int userId, int mediaId)
{
    if (!db)
    {
        std::cerr << "Database is not open." << std::endl;
        return false;
    }
    std::string deleteQuery =
        "DELETE FROM borrowing WHERE user_id = " + std::to_string(userId) +
        " AND media_id = " + std::to_string(mediaId) + ";";
    char *errMsg = nullptr;
    int rc = sqlite3_exec(db, deleteQuery.c_str(), nullptr, nullptr, &errMsg);
    if (rc != SQLITE_OK)
    {
        std::cerr << "SQL error: " << errMsg << std::endl;
        sqlite3_free(errMsg);
        return false;
    }
    return true;
}

std::vector<UserWithBorrowingInfo>
DatabaseManager::getAllUsersWithBorrowingInfo()
{
    std::vector<UserWithBorrowingInfo> usersWithBorrowingInfo;
    if (!db)
    {
        std::cerr << "Database is not open." << std::endl;
        return usersWithBorrowingInfo;
    }
    std::string query =
        "SELECT u.id, u.username, u.password, u.type, "
        "m.id AS media_id, m.title AS media_title, m.type AS media_type "
        "FROM users u LEFT JOIN borrowing b ON u.id = b.user_id "
        "LEFT JOIN media m ON b.media_id = m.id;";

    sqlite3_stmt *stmt;
    int rc = sqlite3_prepare_v2(db, query.c_str(), -1, &stmt, nullptr);
    if (rc != SQLITE_OK)
    {
        std::cerr << "SQL error: " << sqlite3_errmsg(db) << std::endl;
        sqlite3_finalize(stmt);
        return usersWithBorrowingInfo;
    }

    int currentUserId = -1;
    UserWithBorrowingInfo currentUserInfo;

    while (sqlite3_step(stmt) == SQLITE_ROW)
    {
        int userId = sqlite3_column_int(stmt, 0);
        if (userId != currentUserId)
        {
            if (currentUserId != -1)
            {
                usersWithBorrowingInfo.push_back(currentUserInfo);
            }
            currentUserId = userId;
            currentUserInfo.user.id = userId;
            currentUserInfo.user.username =
                reinterpret_cast<const char *>(sqlite3_column_text(stmt, 1));
            currentUserInfo.user.password =
                reinterpret_cast<const char *>(sqlite3_column_text(stmt, 2));
            currentUserInfo.user.type =
                reinterpret_cast<const char *>(sqlite3_column_text(stmt, 3));
            currentUserInfo.borrowedMedia.clear();
        }
        if (sqlite3_column_text(stmt, 4) != nullptr)
        {
            BorrowingInfo borrowingInfo;
            borrowingInfo.mediaId = sqlite3_column_int(stmt, 4);
            borrowingInfo.mediaTitle =
                reinterpret_cast<const char *>(sqlite3_column_text(stmt, 5));
            borrowingInfo.mediaType =
                reinterpret_cast<const char *>(sqlite3_column_text(stmt, 6));
            currentUserInfo.borrowedMedia.push_back(borrowingInfo);
        }
    }
    if (currentUserId != -1)
    {
        usersWithBorrowingInfo.push_back(currentUserInfo);
    }
    sqlite3_finalize(stmt);
    return usersWithBorrowingInfo;
}
