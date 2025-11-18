#include "ConfigManager.hpp"
#include "Database.hpp"
#include <locale>
#include <codecvt>
#include <filesystem>

#pragma warning(once : 4996)

std::wstring Database::stringToWString(const std::string& str) {
    std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
    return converter.from_bytes(str);
}

Database::Database() {
    SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &hEnv);
    SQLSetEnvAttr(hEnv, SQL_ATTR_ODBC_VERSION, (void*)SQL_OV_ODBC3, 0);
    SQLAllocHandle(SQL_HANDLE_DBC, hEnv, &hDbc);
}

Database::~Database() {
    disconnect();
}

std::string Database::getExecutableDirectory() {
    char buffer[MAX_PATH];
    GetModuleFileNameA(NULL, buffer, MAX_PATH);
    std::string::size_type pos = std::string(buffer).find_last_of("\\/");
    return std::string(buffer).substr(0, pos);
}

bool Database::connect() {
    try {
        ConfigManager* config = ConfigManager::getInstance();
        std::string baseConnStr = config->getConnectionString();

        std::string mdfPath = "C:\\API\\x64\\Debug\\Connect.mdf";
        if (!std::filesystem::exists(mdfPath)) {
            std::cerr << "Connect.mdf not found: " << mdfPath << std::endl;
            return false;
        }

        std::string connStr = "Driver={ODBC Driver 17 for SQL Server};"
            "Server=(LocalDB)\\MSSQLLocalDB;"
            "AttachDbFilename=C:\\API\\x64\\Debug\\Connect.mdf;"
            "Database=Connect;"
            "Trusted_Connection=yes;";

        std::cout << "Found Connect.mdf: " << mdfPath << std::endl;
        std::cout << "Connection String: " << connStr << std::endl;

        std::wstring wConnStr = stringToWString(connStr);
        SQLWCHAR outConnStr[1024];
        SQLSMALLINT outConnStrLen;

        SQLRETURN ret = SQLDriverConnect(
            hDbc,
            NULL,
            (SQLWCHAR*)wConnStr.c_str(),
            SQL_NTS,
            outConnStr,
            1024,
            &outConnStrLen,
            SQL_DRIVER_NOPROMPT
        );

        if (!SQL_SUCCEEDED(ret)) {
            SQLWCHAR sqlState[6], message[256];
            SQLINTEGER nativeError;
            SQLSMALLINT msgLen;

            SQLGetDiagRec(SQL_HANDLE_DBC, hDbc, 1, sqlState, &nativeError,
                message, 256, &msgLen);

            std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
            std::cerr << "SQL Error: " << converter.to_bytes(sqlState)
                << " - " << converter.to_bytes(message) << std::endl;
            return false;
        }

        std::cout << "Connected successfully!" << std::endl;
        return true;

    }
    catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return false;
    }
}

void Database::disconnect() {
    if (hDbc) {
        SQLDisconnect(hDbc);
        SQLFreeHandle(SQL_HANDLE_DBC, hDbc);
        hDbc = NULL;
    }
    if (hEnv) {
        SQLFreeHandle(SQL_HANDLE_ENV, hEnv);
        hEnv = NULL;
    }
}

crow::json::wvalue Database::executeQuery(const std::string& query) {
    SQLHSTMT hStmt;
    SQLAllocHandle(SQL_HANDLE_STMT, hDbc, &hStmt);

    std::wstring wQuery = stringToWString(query);

    std::cout << "\n=== EXECUTING QUERY ===" << std::endl;
    std::cout << query << std::endl;

    SQLRETURN ret = SQLExecDirect(hStmt, (SQLWCHAR*)wQuery.c_str(), SQL_NTS);

    crow::json::wvalue result;
    result = crow::json::wvalue::list();

    if (!SQL_SUCCEEDED(ret)) {
        SQLWCHAR sqlState[6], message[256];
        SQLINTEGER nativeError;
        SQLSMALLINT msgLen;
        SQLGetDiagRec(SQL_HANDLE_STMT, hStmt, 1, sqlState, &nativeError, message, 256, &msgLen);

        std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
        std::cerr << "!!! SQL ERROR !!!" << std::endl;
        std::cerr << "State: " << converter.to_bytes(sqlState) << std::endl;
        std::cerr << "Message: " << converter.to_bytes(message) << std::endl;

        SQLFreeHandle(SQL_HANDLE_STMT, hStmt);
        return result;
    }

    SQLSMALLINT columns;
    SQLNumResultCols(hStmt, &columns);

    std::cout << "Columns: " << columns << std::endl;

    int rowIndex = 0;
    while (SQLFetch(hStmt) == SQL_SUCCESS) {
        crow::json::wvalue row;

        for (SQLSMALLINT i = 1; i <= columns; i++) {
            SQLWCHAR buffer[65536];
            SQLLEN indicator;

            SQLRETURN getDataRet = SQLGetData(hStmt, i, SQL_C_WCHAR, buffer, sizeof(buffer), &indicator);

            SQLWCHAR colName[256];
            SQLSMALLINT nameLen;
            SQLDescribeColW(hStmt, i, colName, 256, &nameLen, NULL, NULL, NULL, NULL);

            std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
            std::string columnName = converter.to_bytes(colName);

            if (indicator == SQL_NULL_DATA) {
                row[columnName] = nullptr;
            }
            else {
                std::string value = converter.to_bytes(buffer);
                row[columnName] = value;
            }
        }
        result[rowIndex++] = std::move(row);
    }

    std::cout << "Rows fetched: " << rowIndex << std::endl;
    std::cout << "========================\n" << std::endl;

    SQLFreeHandle(SQL_HANDLE_STMT, hStmt);
    return result;
}

bool Database::executeNonQuery(const std::string& query) {
    SQLHSTMT hStmt;
    SQLAllocHandle(SQL_HANDLE_STMT, hDbc, &hStmt);

    std::wstring wQuery = stringToWString(query);
    SQLRETURN ret = SQLExecDirect(hStmt, (SQLWCHAR*)wQuery.c_str(), SQL_NTS);

    SQLFreeHandle(SQL_HANDLE_STMT, hStmt);
    return SQL_SUCCEEDED(ret);
}