#include <stdio.h>
#include <sqlite3.h>

int main() {
    sqlite3 *db;
    char *err_message = 0;
    
    int rc = sqlite3_open("login_database.db", &db);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Cannot open database: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        
        return 1;
    }

    const char *sql = "CREATE TABLE IF NOT EXISTS users(" \
                      "id INTEGER PRIMARY KEY AUTOINCREMENT," \
                      "username TEXT NOT NULL UNIQUE," \
                      "password TEXT NOT NULL);";
                      
    rc = sqlite3_exec(db, sql, 0, 0, &err_message);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "SQL error: %s\n", err_message);
        sqlite3_free(err_message);
        sqlite3_close(db);
        
        return 1;
    }

    printf("Table created successfully\n");
    sqlite3_close(db);

    return 0;
}