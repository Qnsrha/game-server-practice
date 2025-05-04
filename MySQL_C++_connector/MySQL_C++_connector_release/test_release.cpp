
#include <iostream>
#include <mysql/jdbc.h>

using namespace std;
int main() {
    try {
        sql::mysql::MySQL_Driver* driver;
        sql::Connection* con;

        driver = sql::mysql::get_mysql_driver_instance();
        if (driver == nullptr) {
            cerr << "Failed to get MySQL driver instance." << endl;
            return 1;
        }

        con = driver->connect("tcp://127.0.0.1:3306", "root", "");
        if (con == nullptr) {
            cerr << "Failed to connect to MySQL server." << endl;
            return 1;
        }

        con->setSchema("test");

        sql::Statement* stmt;
        stmt = con->createStatement();

        string createTableSQL = "CREATE TABLE IF NOT EXISTS GFGCourses (id INT NOT NULL AUTO_INCREMENT PRIMARY KEY, courses VARCHAR(255) NOT NULL)";
        stmt->execute(createTableSQL);

        string insertDataSQL = "INSERT INTO GFGCourses (courses) VALUES ('DSA'),('C++'),('JAVA'),('PYTHON')";
        stmt->execute(insertDataSQL);

        string selectDataSQL = "SELECT * FROM GFGCourses";
        sql::ResultSet* res = stmt->executeQuery(selectDataSQL);

        int count = 0;
        while (res->next()) {
            cout << " Course " << ++count << ": " << res->getString("courses") << endl;
        }

        delete res;
        delete stmt;
        delete con;
    }
    catch (sql::SQLException& e) {
        cerr << "SQL Error: " << e.what() << endl;
    }

    return 0;
}
