#include <iostream>
#include <ostream>
#include <string>
#include <vector>
#include <limits>
#include <sstream>
#include <algorithm>
#include "media.h"
#include "database.h"
#ifdef _WIN32
#define CLEAR "cls"
#else
#define CLEAR "clear"
#endif

UserInfo now_user_info;

// 用户类型枚举
enum class UserType
{
    ADMIN,
    USER,
    NONE
};

// 媒体类型枚举
enum class MediaType
{
    MOVIE,
    SONG,
    BOOK
};

// 全局数据库管理器
DatabaseManager dbManager("media.db");

// 函数声明
UserType login();
void adminMenu();
void userMenu();
void handleAdminInput(const std::string& input);
void handleUserInput(const std::string& input);
void addMedia();
void listMedia();
void updateMedia();
void searchMedia();
void manageBorrowing();
void borrowMedia();
void returnMedia();
void viewBorrowedMedia();
void saveChanges();
void logout();
void addUser();
void updateUserPassword();
void updateUserPassword_NotAdmin();
void deleteUser();
void listUsers();

// 当前用户类型
UserType currentUserType = UserType::NONE;
std::string currentUserName = "";

int main()
{
    // 初始化数据库
    dbManager.executeQuery(
        "CREATE TABLE IF NOT EXISTS media (id INTEGER PRIMARY KEY "
        "AUTOINCREMENT, type TEXT, title TEXT, creator TEXT, detail TEXT);");
    dbManager.executeQuery(
        "CREATE TABLE IF NOT EXISTS users (id INTEGER PRIMARY KEY "
        "AUTOINCREMENT, username TEXT, password TEXT, type TEXT);");
    dbManager.executeQuery(
        "CREATE TABLE IF NOT EXISTS borrowing (user_id INTEGER, media_id "
        "INTEGER, PRIMARY KEY (user_id, media_id));");

    // 初始化默认管理员账号
    std::string checkAdminQuery =
        "SELECT COUNT(*) FROM users WHERE username = 'admin';";
    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(dbManager.getDB(), checkAdminQuery.c_str(), -1,
                                &stmt, nullptr);
    int adminCount = 0;

    if (rc == SQLITE_OK)
    {
        if (sqlite3_step(stmt) == SQLITE_ROW)
        {
            adminCount = sqlite3_column_int(stmt, 0);
        }
        sqlite3_finalize(stmt);
    }
    else
    {
        std::cerr << "错误: " << sqlite3_errmsg(dbManager.getDB()) << std::endl;
        sqlite3_finalize(stmt);
    }
    if (adminCount == 0)
    {
        dbManager.executeQuery("INSERT INTO users (username, password, type) "
                               "VALUES ('admin', 'admin', 'admin');");
    }

    while (true)
    {
        currentUserType = login();
        if (currentUserType == UserType::ADMIN)
        {
            adminMenu();
        }
        else if (currentUserType == UserType::USER)
        {
            userMenu();
        }
        else
        {
            std::cout << "登录失败或退出。" << std::endl;
            return 0;
        }
    }

    return 0;
}

void nextProcess(bool clear, bool extra = true)
{
    std::cout << "回车继续...";
    std::cin.get();
    if (extra) std::cin.get();
    if (clear) system(CLEAR);
}

UserType login()
{
    std::string username, password;
    std::string type = "";
    std::cout << "如果您是第一次使用本系统，可以使用默认的管理员账户登陆："
              << std::endl;
    std::cout << "账户名：admin 密码：admin" << std::endl;
    while (true)
    {
        std::cout << "请输入用户名 (输入 'exit' 退出): ";
        std::cin >> username;
        if (username == "exit")
        {
            return UserType::NONE;
        }
        std::cout << "请输入密码: ";
        std::cin >> password;

        std::string dbPassword;
        if (dbManager.selectUser(username, dbPassword, type))
        {
            if (dbPassword == password)
            {
                // 写入全局变量 now_user_info
                std::vector<UserWithBorrowingInfo> usersInfo =
                    dbManager.getAllUsersWithBorrowingInfo();
                for (const auto& userWithBorrowing : usersInfo)
                {
                    if (userWithBorrowing.user.username == username)
                    {
                        now_user_info = userWithBorrowing.user;
                    }
                }

                if (type == "admin")
                {
                    currentUserName = username;
                    return UserType::ADMIN;
                }
                else if (type == "user")
                {
                    currentUserName = username;
                    return UserType::USER;
                }
            }
        }
        std::cout << "用户名或密码错误，请重试。" << std::endl;
    }
}

void adminMenu()
{
    std::string input;
    while (true)
    {
        std::cout << "\n--- 管理员菜单 ---" << std::endl;
        std::cout << "1. 添加媒体" << std::endl;
        std::cout << "2. 列出所有媒体" << std::endl; // 新增选项
        std::cout << "3. 更新媒体" << std::endl;
        std::cout << "4. 搜索媒体" << std::endl;
        std::cout << "5. 管理借阅" << std::endl;
        std::cout << "6. 添加用户" << std::endl;
        std::cout << "7. 修改密码" << std::endl;
        std::cout << "8. 删除用户" << std::endl;
        std::cout << "9. 列出用户" << std::endl;
        std::cout << "10. 登出" << std::endl;
        std::cout << "请输入您的选择: ";

        std::cin >> input;
        if (input == "10")
        { // 修改了登出的编号
            saveChanges();
            logout();
            break;
        }
        handleAdminInput(input);
    }
}

void userMenu()
{
    std::string input;
    while (true)
    {
        std::cout << "\n--- 用户菜单 ---" << std::endl;
        std::cout << "1. 列出媒体" << std::endl;
        std::cout << "2. 搜索媒体" << std::endl;
        std::cout << "3. 借阅媒体" << std::endl;
        std::cout << "4. 归还媒体" << std::endl;
        std::cout << "5. 查看已借阅媒体" << std::endl;
        std::cout << "6. 修改密码" << std::endl;
        std::cout << "7. 登出" << std::endl;
        std::cout << "请输入您的选择: ";

        std::cin >> input;
        if (input == "7")
        {
            saveChanges();
            logout();
            break;
        }
        handleUserInput(input);
    }
}

void handleAdminInput(const std::string& input)
{
    switch (input[0])
    {
        case '1': addMedia(); break;
        case '2': // 新增 case
            listMedia();
            break;
        case '3': updateMedia(); break;
        case '4': searchMedia(); break;
        case '5': manageBorrowing(); break;
        case '6': addUser(); break;
        case '7': updateUserPassword(); break;
        case '8': deleteUser(); break;
        case '9': listUsers(); break;
        default: std::cout << "无效的输入，请重试。" << std::endl;
    }
}

void handleUserInput(const std::string& input)
{
    if (input == "1")
    {
        listMedia();
    }
    else if (input == "2")
    {
        searchMedia();
    }
    else if (input == "3")
    {
        borrowMedia();
    }
    else if (input == "4")
    {
        returnMedia();
    }
    else if (input == "5")
    {
        viewBorrowedMedia();
    }
    else if (input == "6")
    {
        updateUserPassword_NotAdmin();
    }
    else
    {
        std::cout << "无效的输入，请重试。" << std::endl;
    }
}

void addMedia()
{
    std::string type, title, creator, detail;
    int typeChoice;

    while (true)
    {
        std::cout << "请选择媒体类型:" << std::endl;
        std::cout << "1. 电影" << std::endl;
        std::cout << "2. 歌曲" << std::endl;
        std::cout << "3. 书籍" << std::endl;
        std::cout << "请输入您的选择: ";
        std::cin >> typeChoice;
        if (typeChoice > 0 && typeChoice < 4)
        {
            break;
        }
        else
        {
            std::cout << "无效的输入，请重试。" << std::endl;
        }
    }

    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    std::cout << "请输入标题: ";
    std::getline(std::cin, title);
    std::cout << "请输入创建者: ";
    std::getline(std::cin, creator);
    std::cout << "请输入详情: ";
    std::getline(std::cin, detail);

    switch (typeChoice)
    {
        case 1: type = "Movie"; break;
        case 2: type = "Song"; break;
        case 3: type = "Book"; break;
        default: std::cout << "无效的类型选择。" << std::endl; return;
    }

    std::string insertQuery =
        "INSERT INTO media (type, title, creator, detail) VALUES ('" + type +
        "', '" + title + "', '" + creator + "', '" + detail + "');";
    if (dbManager.executeQuery(insertQuery))
    {
        std::cout << "媒体添加成功。" << std::endl;
    }
    else
    {
        std::cout << "媒体添加失败。" << std::endl;
    }
    nextProcess(true, false);
}

void listMedia()
{
    std::string query = "SELECT id, type, title, creator, detail FROM media;";
    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(dbManager.getDB(), query.c_str(), -1, &stmt,
                                nullptr);

    if (rc == SQLITE_OK)
    {
        std::cout << "---------------------------------------------------------"
                     "--------------------------------------------"
                  << std::endl;
        std::cout << "所有媒体信息:" << std::endl;
        std::cout << "---------------------------------------------------------"
                     "--------------------------------------------"
                  << std::endl;
        while (sqlite3_step(stmt) == SQLITE_ROW)
        {
            int id = sqlite3_column_int(stmt, 0);
            const char* type =
                reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
            const char* title =
                reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
            const char* creator =
                reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
            const char* detail =
                reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));

            std::cout << "ID: " << id << ", 类型: " << type
                      << ", 标题: " << title << ", 创建者: " << creator
                      << ", 详情: " << detail << std::endl;
        }
        std::cout << "---------------------------------------------------------"
                     "--------------------------------------------"
                  << std::endl;
        sqlite3_finalize(stmt);
    }
    else
    {
        std::cerr << "错误: " << sqlite3_errmsg(dbManager.getDB()) << std::endl;
    }

    nextProcess(true);
}

void updateMedia()
{
    int mediaId;
    std::cout << "请输入要更新的媒体ID: ";
    std::cin >> mediaId;
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

    // 查询媒体是否存在
    std::string selectQuery =
        "SELECT type, title, creator, detail FROM media WHERE id = " +
        std::to_string(mediaId) + ";";
    sqlite3_stmt* selectStmt;
    int rc = sqlite3_prepare_v2(dbManager.getDB(), selectQuery.c_str(), -1,
                                &selectStmt, nullptr);
    if (rc != SQLITE_OK)
    {
        std::cerr << "错误: " << sqlite3_errmsg(dbManager.getDB()) << std::endl;
        return;
    }

    if (sqlite3_step(selectStmt) != SQLITE_ROW)
    {
        std::cout << "未找到该ID的媒体。" << std::endl;
        sqlite3_finalize(selectStmt);
        return;
    }

    const char* currentType =
        reinterpret_cast<const char*>(sqlite3_column_text(selectStmt, 0));
    const char* currentTitle =
        reinterpret_cast<const char*>(sqlite3_column_text(selectStmt, 1));
    const char* currentCreator =
        reinterpret_cast<const char*>(sqlite3_column_text(selectStmt, 2));
    const char* currentDetail =
        reinterpret_cast<const char*>(sqlite3_column_text(selectStmt, 3));

    std::cout << "\n当前媒体信息:" << std::endl;
    std::cout << "ID: " << mediaId << std::endl;
    std::cout << "类型: " << currentType << std::endl;
    std::cout << "标题: " << currentTitle << std::endl;
    std::cout << "创建者: " << currentCreator << std::endl;
    std::cout << "详情: " << currentDetail << std::endl;
    std::cout << std::endl;

    sqlite3_finalize(selectStmt);

    std::string choice;
    std::string newValue;

    std::cout << "请选择要修改的属性:" << std::endl;
    std::cout << "1. 类型" << std::endl;
    std::cout << "2. 标题" << std::endl;
    std::cout << "3. 创建者" << std::endl;
    std::cout << "4. 详情" << std::endl;
    std::cout << "5. 取消" << std::endl;
    std::cout << "请输入您的选择: ";
    std::cin >> choice;
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

    std::string columnName;
    switch (choice[0])
    {
        case '0':
            columnName = "type";
            std::cout << "请输入新的类型(Movie, Song, Book): ";
            break;
        case '2':
            columnName = "title";
            std::cout << "请输入新的标题: ";
            break;
        case '3':
            columnName = "creator";
            std::cout << "请输入新的创建者: ";
            break;
        case '4':
            columnName = "detail";
            std::cout << "请输入新的详情: ";
            break;
        case '5':
            std::cout << "您已取消" << std::endl;
            nextProcess(true);
            return;
        default: std::cout << "无效的选择。" << std::endl; return;
    }
    while (true)
    {
        std::cin >> newValue;
        if (choice[0] == '1' &&
            !(!newValue.compare("Movie") || !newValue.compare("Song") ||
              !newValue.compare("Book")))
        {
            std::cout << "无效的输入。" << std::endl;
            std::cout << "请输入新的类型(Movie, Song, Book): ";
        }
        else
        {
            break;
        }
    }

    std::string updateQuery = "UPDATE media SET " + columnName + " = '" +
                              newValue +
                              "' WHERE id = " + std::to_string(mediaId) + ";";
    if (dbManager.executeQuery(updateQuery))
    {
        std::cout << "媒体信息更新成功。" << std::endl;
    }
    else
    {
        std::cout << "媒体信息更新失败。" << std::endl;
    }

    nextProcess(true, true);
}

void searchMedia()
{
    std::string type, name;
    int typeChoice;

    while (true)
    {
        std::cout << "请选择搜索媒体类型:" << std::endl;
        std::cout << "1. 电影" << std::endl;
        std::cout << "2. 歌曲" << std::endl;
        std::cout << "3. 书籍" << std::endl;
        std::cout << "请输入您的选择: ";
        std::cin >> typeChoice;
        if (typeChoice > 0 && typeChoice < 4)
        {
            break;
        }
        else
        {
            std::cout << "无效的输入，请重试。" << std::endl;
        }
    }

    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    std::cout << "请输入媒体名称: ";
    std::getline(std::cin, name);

    switch (typeChoice)
    {
        case 1: type = "Movie"; break;
        case 2: type = "Song"; break;
        case 3: type = "Book"; break;
        default: std::cout << "无效的类型选择。" << std::endl; return;
    }

    std::string query =
        "SELECT id, title, creator, detail FROM media WHERE type = '" + type +
        "' AND title LIKE '%" + name + "%';";

    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(dbManager.getDB(), query.c_str(), -1, &stmt,
                                nullptr);

    if (rc == SQLITE_OK)
    {
        while (sqlite3_step(stmt) == SQLITE_ROW)
        {
            int id = sqlite3_column_int(stmt, 0);
            const char* title =
                reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
            const char* creator =
                reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
            const char* detail =
                reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));

            std::cout << "ID: " << id << ", 标题: " << title
                      << ", 创建者: " << creator << ", 详情: " << detail
                      << std::endl;
        }
        sqlite3_finalize(stmt);
    }
    else
    {
        std::cerr << "错误: " << sqlite3_errmsg(dbManager.getDB()) << std::endl;
        sqlite3_finalize(stmt);
    }

    nextProcess(true, false);
}

void manageBorrowing()
{
    std::cout << "\n--- 管理借阅 ---" << std::endl;
    std::vector<UserWithBorrowingInfo> users =
        dbManager.getAllUsersWithBorrowingInfo();
    if (users.empty())
    {
        std::cout << "当前没有用户信息。" << std::endl;
        return;
    }

    std::cout << "请选择要管理借阅的用户ID:" << std::endl;
    for (const auto& user : users)
    {
        std::cout << "ID: " << user.user.id
                  << ", 用户名: " << user.user.username << std::endl;
    }

    int userId;
    std::cout << "请输入用户ID: ";
    std::cin >> userId;

    // 查找选择的用户
    auto userIt = std::find_if(
        users.begin(), users.end(),
        [&](const UserWithBorrowingInfo& u) { return u.user.id == userId; });

    if (userIt == users.end())
    {
        std::cout << "未找到该用户ID。" << std::endl;
        return;
    }

    std::cout << "\n--- " << userIt->user.username << " 的借阅信息 ---"
              << std::endl;
    if (userIt->borrowedMedia.empty())
    {
        std::cout << "该用户没有借阅任何媒体。" << std::endl;
        std::cout << "回车继续...";
        std::cin.get();
        std::cin.get();
        system(CLEAR);
        return;
    }
    else
    {
        for (const auto& media : userIt->borrowedMedia)
        {
            std::cout << "媒体ID: " << media.mediaId
                      << ", 标题: " << media.mediaTitle
                      << ", 类型: " << media.mediaType << std::endl;
        }
    }

    std::cout << "\n请选择操作:" << std::endl;
    std::cout << "1. 归还媒体" << std::endl;
    std::cout << "请输入您的选择: ";
    int actionChoice;
    std::cin >> actionChoice;

    if (actionChoice == 1)
    {
        int mediaIdToReturn;
        std::cout << "请输入要归还的媒体ID: ";
        std::cin >> mediaIdToReturn;
        bool found = false;
        for (const auto& media : userIt->borrowedMedia)
        {
            if (media.mediaId == mediaIdToReturn)
            {
                found = true;
                break;
            }
        }
        if (!found)
        {
            std::cout << "该用户没有借阅此ID的媒体。" << std::endl;
            return;
        }

        if (dbManager.returnMedia(userId, mediaIdToReturn))
        {
            std::cout << "媒体归还成功。" << std::endl;
        }
        else
        {
            std::cout << "媒体归还失败。" << std::endl;
        }
    }
    else
    {
        std::cout << "无效的操作选择。" << std::endl;
    }
    nextProcess(true);
}

void borrowMedia()
{
    std::cout << "\n--- 借阅媒体 ---" << std::endl;
    std::string mediaTitleToBorrow;
    std::cout << "请输入需要借阅的媒体id: ";
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    int mediaId = -1;
    std::cin >> mediaId;

    if (mediaId != -1)
    {
        if (dbManager.borrowMedia(now_user_info.id, mediaId))
        {
            std::cout << "借阅成功。" << std::endl;
        }
        else
        {
            std::cout << "借阅失败。" << std::endl; // 可能是因为已经借阅
        }
    }
    else
    {
        // TODO: 没有时间来优化这个部分了
        std::cout << "未找到名为 '" << mediaTitleToBorrow
                  << "' 的媒体，借阅失败。" << std::endl;
    }
    nextProcess(true);
}

void returnMedia()
{
    std::cout << "\n--- 归还媒体 ---" << std::endl;
    std::string mediaTitleToBorrow;
    std::cout << "请输入需要归还的媒体id: ";
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    int mediaId = -1;
    std::cin >> mediaId;

    if (mediaId != -1)
    {
        if (dbManager.returnMedia(now_user_info.id, mediaId))
        {
            std::cout << "归还成功。" << std::endl;
        }
        else
        {
            std::cout << "归还失败。" << std::endl; // 可能是因为已经借阅
        }
    }
    else
    {
        // TODO: 没有时间来优化这个部分了
        std::cout << "未找到名为 '" << mediaTitleToBorrow
                  << "' 的媒体，借阅失败。" << std::endl;
    }

    nextProcess(true);
}

void viewBorrowedMedia()
{
    std::cout << "\n--- 查看已经借阅的媒体 ---" << std::endl;
    std::vector<BorrowingInfo> infos =
        dbManager.getBorrowedMediaForUser(now_user_info.id);
    for (auto i : infos)
    {
        std::cout << "  - 媒体ID: " << i.mediaId << ", 标题: " << i.mediaTitle
                  << ", 类型: " << i.mediaType << std::endl;
    }
    nextProcess(true);
}

void addUser()
{
    std::string username, password, type;
    int typeChoice;
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

    std::cout << "请输入用户名: ";
    std::getline(std::cin, username);

    std::cout << "请输入密码: ";
    std::getline(std::cin, password);

    while (true)
    {
        std::cout << "请选择用户类型:" << std::endl;
        std::cout << "1. 管理员" << std::endl;
        std::cout << "2. 用户" << std::endl;
        std::cout << "请输入您的选择: ";
        std::cin >> typeChoice;
        if (typeChoice > 0 && typeChoice < 3)
        {
            break;
        }
        else
        {
            std::cout << "无效的输入，请重试。" << std::endl;
        }
    }

    switch (typeChoice)
    {
        case 1: type = "admin"; break;
        case 2: type = "user"; break;
        default: std::cout << "无效的类型选择。" << std::endl; return;
    }

    if (dbManager.addUser(username, password, type))
    {
        std::cout << "用户添加成功。" << std::endl;
    }
    else
    {
        std::cout << "用户添加失败。" << std::endl;
    }
    nextProcess(true);
}

void updateUserPassword()
{
    std::string username, oldPassword, newPassword;
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    std::cout << "请输入用户名: ";
    std::getline(std::cin, username);

    int fail_count = 0;
    while (fail_count != 3)
    {
        std::cout << "请输入旧密码: ";
        std::getline(std::cin, oldPassword);

        std::string type = "";
        std::string dbPassword = "";

        if (!dbManager.selectUser(username, dbPassword, type) ||
            dbPassword != oldPassword)
        {
            std::cout << "用户名或旧密码错误，请重试。" << std::endl;
        }
        else
        {
            break;
        }
        ++fail_count;
    }

    if (fail_count == 3)
    {
        std::cout << "重试达到3次，自动返回菜单";
        nextProcess(true, false);
        return;
    }

    std::cout << "请输入新密码: ";
    std::getline(std::cin, newPassword);

    if (dbManager.updateUserPassword(username, newPassword))
    {
        std::cout << "密码修改成功。" << std::endl;
    }
    else
    {
        std::cout << "密码修改失败。" << std::endl;
    }
}

void updateUserPassword_NotAdmin()
{
    std::string username = now_user_info.username, oldPassword, newPassword;
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

    int fail_count = 0;
    while (fail_count != 3)
    {
        std::cout << "请输入旧密码: ";
        std::getline(std::cin, oldPassword);

        std::string type = "";
        std::string dbPassword = "";

        if (!dbManager.selectUser(username, dbPassword, type) ||
            dbPassword != oldPassword)
        {
            std::cout << "用户名或旧密码错误，请重试。" << std::endl;
        }
        else
        {
            break;
        }
        ++fail_count;
    }

    if (fail_count == 3)
    {
        std::cout << "重试达到3次，自动返回菜单";
        nextProcess(true, false);
        return;
    }

    std::cout << "请输入新密码: ";
    std::getline(std::cin, newPassword);

    if (dbManager.updateUserPassword(username, newPassword))
    {
        std::cout << "密码修改成功。" << std::endl;
    }
    else
    {
        std::cout << "密码修改失败。" << std::endl;
    }
    nextProcess(true, false);
    return;
}

void deleteUser()
{
    std::string username;
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    std::cout << "请输入要删除的用户名: ";
    std::getline(std::cin, username);

    if (dbManager.deleteUser(username))
    {
        std::cout << "用户删除成功。" << std::endl;
    }
    else
    {
        std::cout << "用户删除失败。" << std::endl;
    }
}

void listUsers()
{
    std::vector<UserWithBorrowingInfo> usersInfo =
        dbManager.getAllUsersWithBorrowingInfo();
    if (usersInfo.empty())
    {
        std::cout << "当前没有用户信息。" << std::endl;
        nextProcess(true);
        return;
    }

    std::cout << "-------------------------------------------------------------"
                 "----------------------------------------"
              << std::endl;
    std::cout << "所有用户信息:" << std::endl;
    std::cout << "-------------------------------------------------------------"
                 "----------------------------------------"
              << std::endl;

    for (const auto& userWithBorrowing : usersInfo)
    {
        std::cout << "用户ID: " << userWithBorrowing.user.id << std::endl;
        std::cout << "用户名: " << userWithBorrowing.user.username << std::endl;
        std::cout << "用户类型: " << userWithBorrowing.user.type << std::endl;
        std::cout << "借阅信息:" << std::endl;
        if (userWithBorrowing.borrowedMedia.empty())
        {
            std::cout << "  该用户没有借阅任何媒体。" << std::endl;
        }
        else
        {
            for (const auto& borrowing : userWithBorrowing.borrowedMedia)
            {
                std::cout << "  - 媒体ID: " << borrowing.mediaId
                          << ", 标题: " << borrowing.mediaTitle
                          << ", 类型: " << borrowing.mediaType << std::endl;
            }
        }
        std::cout << "---------------------------------------------------------"
                     "--------------------------------------------"
                  << std::endl;
    }

    nextProcess(true);
}

void saveChanges() { std::cout << "更改已保存。" << std::endl; }

void logout()
{
    currentUserName = "";
    currentUserType = UserType::NONE;
    std::cout << "登出成功。" << std::endl;
}