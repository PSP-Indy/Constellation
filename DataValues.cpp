#include "DataValues.hpp"

DataValues::DataValues()
    : cx{"postgresql://constellation:pswd@172.31.238.159:5432/flightdata"}
{
    try {
        pqxx::work tx{cx};

        tx.exec("CREATE TABLE IF NOT EXISTS runs ("
                "run_id INTEGER GENERATED ALWAYS AS IDENTITY PRIMARY KEY, "
                "started_at TIMESTAMPTZ NOT NULL DEFAULT now()"
                ")");

        tx.exec("DROP TABLE IF EXISTS antennas;")
        tx.exec("CREATE TABLE antennas (uuid INTEGER, rssi DOUBLE PRECISION)");

        tx.exec("CREATE TABLE IF NOT EXISTS logs ("
                "run_id INTEGER NOT NULL REFERENCES runs(run_id) ON DELETE CASCADE, "
                "timestamp DOUBLE PRECISION NOT NULL, "
                "a_value DOUBLE PRECISION, "
                "v_value DOUBLE PRECISION, "
                "x_value DOUBLE PRECISION, "
                "y_value DOUBLE PRECISION, "
                "z_value DOUBLE PRECISION, "
                "x_rot_value DOUBLE PRECISION, "
                "y_rot_value DOUBLE PRECISION, "
                "z_rot_value DOUBLE PRECISION, "
                "rssi DOUBLE PRECISION, "
                "PRIMARY KEY (run_id, timestamp)"
                ")");

        auto row = tx.exec1("INSERT INTO runs DEFAULT VALUES RETURNING run_id");
        run_id = row[0].as<int64_t>();
        tx.commit();

        cx.prepare("insert_data_value_snapshot",
                "INSERT INTO logs (run_id, timestamp, a_value, v_value, x_value, y_value, "
                "z_value, x_rot_value, y_rot_value, z_rot_value, rssi) "
                "VALUES ($1, $2, $3, $4, $5, $6, $7, $8, $9, $10, $11) "
                "ON CONFLICT (run_id, timestamp) "
                "DO UPDATE SET "
                "a_value = EXCLUDED.a_value, v_value = EXCLUDED.v_value, x_value = EXCLUDED.x_value, "
                "y_value = EXCLUDED.y_value, z_value = EXCLUDED.z_value, x_rot_value = EXCLUDED.x_rot_value, "
                "y_rot_value = EXCLUDED.y_rot_value, z_rot_value = EXCLUDED.z_rot_value, rssi = EXCLUDED.rssi "
                "WHERE EXCLUDED.rssi > logs.rssi");

        cx.prepare("get_data_value_list", 
                "SELECT timestamp, a_value, v_value, x_value, y_value, z_value, "
                "x_rot_value, y_rot_value, z_rot_value "
                "FROM logs WHERE run_id = $1 AND timestamp > $2 ORDER BY timestamp");

        std::cout << "Database started successfully with id: " << run_id << std::endl;
    }
    catch (std::exception& e) {
        std::cerr << "Postgres exception: " << e.what() << std::endl;
        throw;
    }
}

void DataValues::setValueLock(std::mutex* valueLock)
{
	this->valueLock = valueLock;
}

void DataValues::InsertDataSnapshot(float time, DataValueSnapshot data)
{
    std::lock_guard<std::mutex> lock(*valueLock);

    pqxx::work tx{cx};
    tx.exec_prepared("insert_data_value_snapshot", run_id, time, data.a_value, data.v_value,
                    data.x_value, data.y_value, data.z_value, data.x_rot_value, data.y_rot_value, 
                    data.z_rot_value, 0.0);
    tx.commit();
}

DataValues::DataValueList DataValues::getDataValueList()
{
    std::lock_guard<std::mutex> lock(*valueLock);

    pqxx::work tx{cx};
    pqxx::result result = tx.exec_prepared("get_data_value_list", run_id, values.t_values.back());

    for (auto const& row : result) {
        int i = 0;

        values.t_values.push_back(row[i++].as<float>());
        values.a_values.push_back(row[i++].as<float>());
        values.v_values.push_back(row[i++].as<float>());
        values.x_values.push_back(row[i++].as<float>());
        values.y_values.push_back(row[i++].as<float>());
        values.z_values.push_back(row[i++].as<float>());
        values.x_rot_values.push_back(row[i++].as<float>());
        values.y_rot_values.push_back(row[i++].as<float>());
        values.z_rot_values.push_back(row[i++].as<float>());
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
