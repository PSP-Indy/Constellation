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

        get_data_list_statement = std::make_unique<SQLite::Statement>(db, 
                "SELECT timestamp, a_value, v_value, x_value, y_value, z_value, "
                "x_rot_value, y_rot_value, z_rot_value "
                "FROM logs WHERE run_id = ? AND timestamp > ? ORDER BY timestamp");

        std::cout << "Database started successfully with id: " << run_id << std::endl;
    }
    catch (std::exception& e) {
        std::cerr << "SQLite exception: " << e.what() << std::endl;
        throw;
    }
}

void DataValues::setValueLock(std::mutex* valueLock)
{
	this->valueLock = valueLock;
}

void DataValues::InsertDataSnapshot(float time, DataValueSnapshot data)
{
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
    get_data_list_statement->bind(1, run_id);
    get_data_list_statement->bind(2, values.t_values.back());

    while (get_data_list_statement->executeStep()) {
        int i = 0;

        values.t_values.push_back(get_data_list_statement->getColumn("timestamp").getDouble());
        values.a_values.push_back(get_data_list_statement->getColumn("a_value").getDouble());
        values.v_values.push_back(get_data_list_statement->getColumn("v_value").getDouble());
        values.x_values.push_back(get_data_list_statement->getColumn("x_value").getDouble());
        values.y_values.push_back(get_data_list_statement->getColumn("y_value").getDouble());
        values.z_values.push_back(get_data_list_statement->getColumn("z_value").getDouble());
        values.x_rot_values.push_back(get_data_list_statement->getColumn("x_rot_value").getDouble());
        values.y_rot_values.push_back(get_data_list_statement->getColumn("y_rot_value").getDouble());
        values.z_rot_values.push_back(get_data_list_statement->getColumn("z_rot_value").getDouble());
    }
    get_data_list_statement->reset();

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
