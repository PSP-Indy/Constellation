#include "DataValues.hpp"

DataValues::DataValues()
    : db("flight_data.db3", SQLite::OPEN_READWRITE | SQLite::OPEN_CREATE)
{
    try {
        db.exec("CREATE TABLE IF NOT EXISTS runs ("
                "run_id INTEGER PRIMARY KEY AUTOINCREMENT, "
                "started_at TEXT NOT NULL"
                ")");

        db.exec("INSERT INTO runs (started_at) VALUES (datetime('now'))");
        run_id = db.getLastInsertRowid();

        db.exec("CREATE TABLE IF NOT EXISTS logs ("
                "run_id INTEGER NOT NULL REFERENCES runs(run_id) ON DELETE CASCADE, "
                "timestamp REAL NOT NULL, "
                "a_value REAL, "
                "v_value REAL, "
                "x_value REAL, "
                "y_value REAL, "
                "z_value REAL, "
                "x_rot_value REAL, "
                "y_rot_value REAL, "
                "z_rot_value REAL, "
                "PRIMARY KEY (run_id, timestamp)"
                ") WITHOUT ROWID");
        
        insert_query_statement = std::make_unique<SQLite::Statement>(db,
                "INSERT INTO logs (run_id, timestamp, a_value, v_value, x_value, y_value, "
                "z_value, x_rot_value, y_rot_value, z_rot_value) "
                "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?)");

        std::cout << "Database started successfully with id: " << run_id << std::endl;
    }
    catch (std::exception& e) {
        std::cerr << "SQLite exception: " << e.what() << std::endl;
    }
}

void DataValues::setValueLock(std::mutex* valueLock)
{
	this->valueLock = valueLock;
}

void DataValues::InsertDataSnapshot(float time, DataValueSnapshot data)
{
    SQLite::Statement insert(db, "INSERT INTO logs (run_id, timestamp, a_value, v_value, x_value, y_value, z_value, x_rot_value, y_rot_value, z_rot_value) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?)");

    int i = 1;

    insert_query_statement->bind(i++, run_id);
    insert_query_statement->bind(i++, time); 
    insert_query_statement->bind(i++, data.a_value);
    insert_query_statement->bind(i++, data.v_value);
    insert_query_statement->bind(i++, data.x_value);
    insert_query_statement->bind(i++, data.y_value);
    insert_query_statement->bind(i++, data.z_value);
    insert_query_statement->bind(i++, data.x_rot_value);
    insert_query_statement->bind(i++, data.y_rot_value);
    insert_query_statement->bind(i++, data.z_rot_value);
    insert_query_statement->exec();
    insert_query_statement->reset();
}

DataValues::DataValueList DataValues::getDataValueList()
{
    SQLite::Statement query(db, "SELECT * FROM logs WHERE run_id = ? ORDER BY timestamp");
    query.bind(1, "run_id");

    DataValues::DataValueList values;

    while (query.executeStep()) {
        int i = 0;

        values.t_values.push_back(query.getColumn(i++).getDouble());
        values.a_values.push_back(query.getColumn(i++).getDouble());
        values.x_values.push_back(query.getColumn(i++).getDouble());
        values.y_values.push_back(query.getColumn(i++).getDouble());
        values.z_values.push_back(query.getColumn(i++).getDouble());
        values.x_rot_values.push_back(query.getColumn(i++).getDouble());
        values.y_rot_values.push_back(query.getColumn(i++).getDouble());
        values.z_rot_values.push_back(query.getColumn(i++).getDouble());
    }

    return values;
}

DataValues::~DataValues()
{
}

DataValues *DataValues::Get()
{
	if (dataValues == nullptr)
		dataValues = new DataValues();
	return dataValues;
}
