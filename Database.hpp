#pragma once
#include <windows.h>
#ifdef _WIN64
typedef INT64 SQLLEN;
typedef UINT64 SQLULEN;
typedef UINT64 SQLSETPOSIROW;
#else
typedef long SQLLEN;
typedef unsigned long SQLULEN;
typedef unsigned short SQLSETPOSIROW;
#endif
#define ODBCVER 0x0380

#include <sql.h>
#include <sqlext.h>
#include <string>
#include <crow.h>

class Database {
public:
    Database();
    ~Database();

    bool connect();
    crow::json::wvalue executeQuery(const std::string& query);
    bool executeNonQuery(const std::string& query);
    void disconnect();
private:
    SQLHENV hEnv;
    SQLHDBC hDbc;

    std::wstring stringToWString(const std::string& str);
    std::string getExecutableDirectory();
};