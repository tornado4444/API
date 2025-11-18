#include "Database.hpp"
#include <regex>

Database db;

std::string escapeSql(const std::string& str) {
    std::string result;
    for (const char c : str) {
        if (c == '\'') result += "''";
        else result += c;
    }
    return result;
}

bool isValidEmail(const std::string& email) {
    const std::regex pattern(R"([a-zA-Z0-9._%+-]+@[a-zA-Z0-9.-]+\.[a-zA-Z]{2,})");
    return std::regex_match(email, pattern);
}

bool isValidUsername(const std::string& username) {
    return !username.empty() && username.length() >= 3 && username.length() <= 50;
}

bool isValidPassword(const std::string& password) {
    return !password.empty() && password.length() >= 6;
}

bool isValidTitle(const std::string& title) {
    return !title.empty() && title.length() <= 200;
}

bool isValidContent(const std::string& content) {
    return !content.empty() && content.length() <= 1000;
}

int main()
{
    if (!db.connect()) {
        std::cerr << "Failed to connect to database" << std::endl;
        return 1;
    }

    std::cout << "Database connected successfully!" << std::endl;

    crow::SimpleApp app;

    CROW_ROUTE(app, "/")
        ([]() {
        return crow::response(200, R"(<html><head><title>API Server</title><style>body{font-family:Arial;max-width:800px;margin:50px auto;padding:20px}h1{color:#333}.endpoint{background:#f4f4f4;padding:10px;margin:10px 0;border-radius:5px}.method{font-weight:bold;color:#0066cc}</style></head><body><h1>REST API Server</h1><p>Server is running on port 8080</p><h2>Available Endpoints:</h2><h3>Users</h3><div class="endpoint"><span class="method">GET</span> /api/users - Get all users</div><div class="endpoint"><span class="method">GET</span> /api/users/{id} - Get user by ID</div><div class="endpoint"><span class="method">POST</span> /api/users - Create new user</div><div class="endpoint"><span class="method">PUT</span> /api/users/{id} - Update user</div><div class="endpoint"><span class="method">DELETE</span> /api/users/{id} - Delete user</div><h3>Posts</h3><div class="endpoint"><span class="method">GET</span> /api/posts - Get all posts</div><div class="endpoint"><span class="method">GET</span> /api/posts/{id} - Get post by ID</div><div class="endpoint"><span class="method">POST</span> /api/posts - Create new post</div><div class="endpoint"><span class="method">PUT</span> /api/posts/{id} - Update post</div><div class="endpoint"><span class="method">DELETE</span> /api/posts/{id} - Delete post</div><h3>Comments</h3><div class="endpoint"><span class="method">GET</span> /api/comments - Get all comments</div><div class="endpoint"><span class="method">GET</span> /api/comments/{id} - Get comment by ID</div><div class="endpoint"><span class="method">GET</span> /api/posts/{id}/comments - Get comments for post</div><div class="endpoint"><span class="method">POST</span> /api/comments - Create new comment</div><div class="endpoint"><span class="method">PUT</span> /api/comments/{id} - Update comment</div><div class="endpoint"><span class="method">DELETE</span> /api/comments/{id} - Delete comment</div></body></html>)");
            });

    CROW_ROUTE(app, "/api/users").methods("POST"_method)
        ([](const crow::request& req) {
        auto body = crow::json::load(req.body);
        if (!body) return crow::response(400, "{\"error\":\"Invalid JSON\"}");
        if (!body.has("username") || !body.has("email") || !body.has("password")) return crow::response(400, "{\"error\":\"Missing required fields: username, email, password\"}");
        std::string username = body["username"].s();
        std::string email = body["email"].s();
        std::string password = body["password"].s();
        if (!isValidUsername(username)) return crow::response(400, "{\"error\":\"Username must be between 3 and 50 characters\"}");
        if (!isValidEmail(email)) return crow::response(400, "{\"error\":\"Invalid email format\"}");
        if (!isValidPassword(password)) return crow::response(400, "{\"error\":\"Password must be at least 6 characters long\"}");
        std::string checkQuery = "SELECT IdUser FROM Users WHERE Username = N'" + escapeSql(username) + "' OR Email = N'" + escapeSql(email) + "'";
        auto existing = db.executeQuery(checkQuery);
        if (existing.size() > 0) return crow::response(409, "{\"error\":\"Username or email already exists\"}");
        std::string query = "INSERT INTO Users (Username, Email, Password) VALUES (N'" + escapeSql(username) + "', N'" + escapeSql(email) + "', N'" + escapeSql(password) + "')";
        if (db.executeNonQuery(query)) return crow::response(201, "{\"message\":\"User created successfully\"}");
        return crow::response(500, "{\"error\":\"Failed to create user\"}");
            });

    CROW_ROUTE(app, "/api/users").methods("GET"_method)
        ([]() {
        std::string query = "SELECT IdUser, Username, Email, CreatedAt FROM Users";
        auto users = db.executeQuery(query);
        return crow::response{ users };
            });

    CROW_ROUTE(app, "/api/users/<int>").methods("GET"_method)
        ([](int id) {
        if (id <= 0) return crow::response(400, "{\"error\":\"Invalid user ID\"}");
        std::string query = "SELECT IdUser, Username, Email, CreatedAt FROM Users WHERE IdUser = " + std::to_string(id);
        auto user = db.executeQuery(query);
        if (user.size() > 0) return crow::response{ user[0] };
        return crow::response(404, "{\"error\":\"User not found\"}");
            });

    CROW_ROUTE(app, "/api/users/<int>").methods("PUT"_method)
        ([](const crow::request& req, int id) {
        if (id <= 0) return crow::response(400, "{\"error\":\"Invalid user ID\"}");
        auto body = crow::json::load(req.body);
        if (!body) return crow::response(400, "{\"error\":\"Invalid JSON\"}");
        std::string checkQuery = "SELECT IdUser FROM Users WHERE IdUser = " + std::to_string(id);
        auto existing = db.executeQuery(checkQuery);
        if (existing.size() == 0) return crow::response(404, "{\"error\":\"User not found\"}");
        std::string query = "UPDATE Users SET ";
        bool hasFields = false;
        if (body.has("username")) {
            std::string username = body["username"].s();
            if (!isValidUsername(username)) return crow::response(400, "{\"error\":\"Username must be between 3 and 50 characters\"}");
            query += "Username = N'" + escapeSql(username) + "'";
            hasFields = true;
        }
        if (body.has("email")) {
            std::string email = body["email"].s();
            if (!isValidEmail(email)) return crow::response(400, "{\"error\":\"Invalid email format\"}");
            if (hasFields) query += ", ";
            query += "Email = N'" + escapeSql(email) + "'";
            hasFields = true;
        }
        if (body.has("password")) {
            std::string password = body["password"].s();
            if (!isValidPassword(password)) return crow::response(400, "{\"error\":\"Password must be at least 6 characters long\"}");
            if (hasFields) query += ", ";
            query += "Password = N'" + escapeSql(password) + "'";
            hasFields = true;
        }
        if (!hasFields) return crow::response(400, "{\"error\":\"No fields to update\"}");
        query += " WHERE IdUser = " + std::to_string(id);
        if (db.executeNonQuery(query)) return crow::response(200, "{\"message\":\"User updated successfully\"}");
        return crow::response(500, "{\"error\":\"Failed to update user\"}");
            });

    CROW_ROUTE(app, "/api/users/<int>").methods("DELETE"_method)
        ([](int id) {
        if (id <= 0) return crow::response(400, "{\"error\":\"Invalid user ID\"}");
        std::string checkQuery = "SELECT IdUser FROM Users WHERE IdUser = " + std::to_string(id);
        auto existing = db.executeQuery(checkQuery);
        if (existing.size() == 0) return crow::response(404, "{\"error\":\"User not found\"}");
        std::string query = "DELETE FROM Users WHERE IdUser = " + std::to_string(id);
        if (db.executeNonQuery(query)) return crow::response(200, "{\"message\":\"User deleted successfully\"}");
        return crow::response(500, "{\"error\":\"Failed to delete user\"}");
            });

    CROW_ROUTE(app, "/api/posts").methods("POST"_method)
        ([](const crow::request& req) {
        auto body = crow::json::load(req.body);
        if (!body) return crow::response(400, "{\"error\":\"Invalid JSON\"}");
        if (!body.has("idUser") || !body.has("title") || !body.has("content")) return crow::response(400, "{\"error\":\"Missing required fields: idUser, title, content\"}");
        int idUser = body["idUser"].i();
        std::string title = body["title"].s();
        std::string content = body["content"].s();
        if (idUser <= 0) return crow::response(400, "{\"error\":\"Invalid user ID\"}");
        if (!isValidTitle(title)) return crow::response(400, "{\"error\":\"Title must not be empty and max 200 characters\"}");
        if (!isValidContent(content)) return crow::response(400, "{\"error\":\"Content must not be empty and max 1000 characters\"}");
        std::string checkUser = "SELECT IdUser FROM Users WHERE IdUser = " + std::to_string(idUser);
        auto userExists = db.executeQuery(checkUser);
        if (userExists.size() == 0) return crow::response(404, "{\"error\":\"User not found\"}");
        std::string query = "INSERT INTO Posts (IdUser, Title, Content) VALUES (" + std::to_string(idUser) + ", N'" + escapeSql(title) + "', N'" + escapeSql(content) + "')";
        if (db.executeNonQuery(query)) return crow::response(201, "{\"message\":\"Post created successfully\"}");
        return crow::response(500, "{\"error\":\"Failed to create post\"}");
            });

    CROW_ROUTE(app, "/api/posts").methods("GET"_method)
        ([]() {
        std::string query = "SELECT p.IdPost, p.IdUser, p.Title, p.Content, p.CreatedAt, p.UpdatedAt, u.Username FROM Posts p JOIN Users u ON p.IdUser = u.IdUser";
        auto posts = db.executeQuery(query);
        return crow::response{ posts };
            });

    CROW_ROUTE(app, "/api/posts/<int>").methods("GET"_method)
        ([](int id) {
        if (id <= 0) return crow::response(400, "{\"error\":\"Invalid post ID\"}");
        std::string query = "SELECT p.IdPost, p.IdUser, p.Title, p.Content, p.CreatedAt, p.UpdatedAt, u.Username FROM Posts p JOIN Users u ON p.IdUser = u.IdUser WHERE p.IdPost = " + std::to_string(id);
        auto post = db.executeQuery(query);
        if (post.size() > 0) return crow::response{ post[0] };
        return crow::response(404, "{\"error\":\"Post not found\"}");
            });

    CROW_ROUTE(app, "/api/posts/<int>").methods("PUT"_method)
        ([](const crow::request& req, int id) {
        if (id <= 0) return crow::response(400, "{\"error\":\"Invalid post ID\"}");
        auto body = crow::json::load(req.body);
        if (!body) return crow::response(400, "{\"error\":\"Invalid JSON\"}");
        std::string checkQuery = "SELECT IdPost FROM Posts WHERE IdPost = " + std::to_string(id);
        auto existing = db.executeQuery(checkQuery);
        if (existing.size() == 0) return crow::response(404, "{\"error\":\"Post not found\"}");
        std::string query = "UPDATE Posts SET ";
        bool hasFields = false;
        if (body.has("title")) {
            std::string title = body["title"].s();
            if (!isValidTitle(title)) return crow::response(400, "{\"error\":\"Title must not be empty and max 200 characters\"}");
            query += "Title = N'" + escapeSql(title) + "'";
            hasFields = true;
        }
        if (body.has("content")) {
            std::string content = body["content"].s();
            if (!isValidContent(content)) return crow::response(400, "{\"error\":\"Content must not be empty and max 1000 characters\"}");
            if (hasFields) query += ", ";
            query += "Content = N'" + escapeSql(content) + "'";
            hasFields = true;
        }
        if (!hasFields) return crow::response(400, "{\"error\":\"No fields to update\"}");
        query += ", UpdatedAt = GETDATE() WHERE IdPost = " + std::to_string(id);
        if (db.executeNonQuery(query)) return crow::response(200, "{\"message\":\"Post updated successfully\"}");
        return crow::response(500, "{\"error\":\"Failed to update post\"}");
            });

    CROW_ROUTE(app, "/api/posts/<int>").methods("DELETE"_method)
        ([](int id) {
        if (id <= 0) return crow::response(400, "{\"error\":\"Invalid post ID\"}");
        std::string checkQuery = "SELECT IdPost FROM Posts WHERE IdPost = " + std::to_string(id);
        auto existing = db.executeQuery(checkQuery);
        if (existing.size() == 0) return crow::response(404, "{\"error\":\"Post not found\"}");
        std::string query = "DELETE FROM Posts WHERE IdPost = " + std::to_string(id);
        if (db.executeNonQuery(query)) return crow::response(200, "{\"message\":\"Post deleted successfully\"}");
        return crow::response(500, "{\"error\":\"Failed to delete post\"}");
            });

    CROW_ROUTE(app, "/api/comments").methods("GET"_method)
        ([]() {
        std::string query = "SELECT c.IdComment, c.IdPost, c.IdUser, c.Content, c.CreatedAt, u.Username, p.Title as PostTitle FROM Comments c JOIN Users u ON c.IdUser = u.IdUser JOIN Posts p ON c.IdPost = p.IdPost ORDER BY c.IdComment";
        auto comments = db.executeQuery(query);
        return crow::response{ comments };
            });

    CROW_ROUTE(app, "/api/comments").methods("POST"_method)
        ([](const crow::request& req) {
        auto body = crow::json::load(req.body);
        if (!body) return crow::response(400, "{\"error\":\"Invalid JSON\"}");
        if (!body.has("idPost") || !body.has("idUser") || !body.has("content")) return crow::response(400, "{\"error\":\"Missing required fields: idPost, idUser, content\"}");
        int idPost = body["idPost"].i();
        int idUser = body["idUser"].i();
        std::string content = body["content"].s();
        if (idPost <= 0) return crow::response(400, "{\"error\":\"Invalid post ID\"}");
        if (idUser <= 0) return crow::response(400, "{\"error\":\"Invalid user ID\"}");
        if (!isValidContent(content)) return crow::response(400, "{\"error\":\"Content must not be empty and max 1000 characters\"}");
        std::string checkPost = "SELECT IdPost FROM Posts WHERE IdPost = " + std::to_string(idPost);
        auto postExists = db.executeQuery(checkPost);
        if (postExists.size() == 0) return crow::response(404, "{\"error\":\"Post not found\"}");
        std::string checkUser = "SELECT IdUser FROM Users WHERE IdUser = " + std::to_string(idUser);
        auto userExists = db.executeQuery(checkUser);
        if (userExists.size() == 0) return crow::response(404, "{\"error\":\"User not found\"}");
        std::string query = "INSERT INTO Comments (IdPost, IdUser, Content) VALUES (" + std::to_string(idPost) + ", " + std::to_string(idUser) + ", N'" + escapeSql(content) + "')";
        if (db.executeNonQuery(query)) return crow::response(201, "{\"message\":\"Comment created successfully\"}");
        return crow::response(500, "{\"error\":\"Failed to create comment\"}");
            });

    CROW_ROUTE(app, "/api/posts/<int>/comments").methods("GET"_method)
        ([](int postId) {
        if (postId <= 0) return crow::response(400, "{\"error\":\"Invalid post ID\"}");
        std::string query = "SELECT c.IdComment, c.IdPost, c.IdUser, c.Content, c.CreatedAt, u.Username FROM Comments c JOIN Users u ON c.IdUser = u.IdUser WHERE c.IdPost = " + std::to_string(postId);
        auto comments = db.executeQuery(query);
        return crow::response{ comments };
            });

    CROW_ROUTE(app, "/api/comments/<int>").methods("GET"_method)
        ([](int id) {
        if (id <= 0) return crow::response(400, "{\"error\":\"Invalid comment ID\"}");
        std::string query = "SELECT c.IdComment, c.IdPost, c.IdUser, c.Content, c.CreatedAt, u.Username, p.Title as PostTitle FROM Comments c JOIN Users u ON c.IdUser = u.IdUser JOIN Posts p ON c.IdPost = p.IdPost WHERE c.IdComment = " + std::to_string(id);
        auto comment = db.executeQuery(query);
        if (comment.size() > 0) return crow::response{ comment[0] };
        return crow::response(404, "{\"error\":\"Comment not found\"}");
            });

    CROW_ROUTE(app, "/api/comments/<int>").methods("PUT"_method)
        ([](const crow::request& req, int id) {
        if (id <= 0) return crow::response(400, "{\"error\":\"Invalid comment ID\"}");
        auto body = crow::json::load(req.body);
        if (!body) return crow::response(400, "{\"error\":\"Invalid JSON\"}");
        if (!body.has("content")) return crow::response(400, "{\"error\":\"Missing required field: content\"}");
        std::string checkQuery = "SELECT IdComment FROM Comments WHERE IdComment = " + std::to_string(id);
        auto existing = db.executeQuery(checkQuery);
        if (existing.size() == 0) return crow::response(404, "{\"error\":\"Comment not found\"}");
        std::string content = body["content"].s();
        if (!isValidContent(content)) return crow::response(400, "{\"error\":\"Content must not be empty and max 1000 characters\"}");
        std::string query = "UPDATE Comments SET Content = N'" + escapeSql(content) + "' WHERE IdComment = " + std::to_string(id);
        if (db.executeNonQuery(query)) return crow::response(200, "{\"message\":\"Comment updated successfully\"}");
        return crow::response(500, "{\"error\":\"Failed to update comment\"}");
            });

    CROW_ROUTE(app, "/api/comments/<int>").methods("DELETE"_method)
        ([](int id) {
        if (id <= 0) return crow::response(400, "{\"error\":\"Invalid comment ID\"}");
        std::string checkQuery = "SELECT IdComment FROM Comments WHERE IdComment = " + std::to_string(id);
        auto existing = db.executeQuery(checkQuery);
        if (existing.size() == 0) return crow::response(404, "{\"error\":\"Comment not found\"}");
        std::string query = "DELETE FROM Comments WHERE IdComment = " + std::to_string(id);
        if (db.executeNonQuery(query)) return crow::response(200, "{\"message\":\"Comment deleted successfully\"}");
        return crow::response(500, "{\"error\":\"Failed to delete comment\"}");
            });

    std::cout << "Server running on http://localhost:8080" << std::endl;
    app.port(8080).multithreaded().run();

    return 0;
}