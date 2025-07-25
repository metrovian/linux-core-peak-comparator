#include "database_spectrum.h"
#include "wrapper_openbabel.h"
#include "wrapper_spdlog.h"
#include "predefined.h"

static PGconn *database_spectrum = NULL;

extern int8_t database_spectrum_open() {
	char command_open[512];
	snprintf(
	    command_open,
	    sizeof(command_open),
	    "host=localhost "
	    "port=%d "
	    "dbname=spectrum "
	    "user=postgres ",
	    NET_DATABASE_PORT);

	database_spectrum = PQconnectdb(command_open);
	if (PQstatus(database_spectrum) != CONNECTION_OK) {
		PQfinish(database_spectrum);
		log_error("failed to connect database");
		return -1;
	}

	log_info("database service started");
	return 0;
}

extern int8_t database_spectrum_close() {
	if (database_spectrum) {
		PQfinish(database_spectrum);
		database_spectrum = NULL;
		log_info("database service terminated");
	}

	return 0;
}

extern int8_t database_spectrum_status() {
	return database_spectrum != NULL;
}

extern int8_t database_spectrum_insert_molecule(const char *name, const char *inchi) {
	const char *params_query[2] = {name, inchi};
	PGresult *result_query =
	    PQexecParams(
		database_spectrum,
		"INSERT INTO molecule (name, inchi) VALUES ($1, $2)",
		2,
		NULL,
		params_query,
		NULL,
		NULL,
		0);

	if (PQresultStatus(result_query) != PGRES_COMMAND_OK) {
		PQclear(result_query);
		log_error("failed to insert molecule (%s, %s)", name, mol_hash(inchi));
		return -1;
	}

	PQclear(result_query);
	log_info("molecule insert success (%s, %s)", name, mol_hash(inchi));
	return 0;
}

extern int8_t database_spectrum_insert_mass(const char *name, const char *inchi, float *peaks_data, int32_t peaks_number) {
	PGresult *result_query = PQexec(database_spectrum, "BEGIN");
	if (PQresultStatus(result_query) != PGRES_COMMAND_OK) {
		PQclear(result_query);
		log_error("failed to begin transaction");
		log_error("failed to insert mass spectrum (%s, %s)", name, mol_hash(inchi));
		return -1;
	}

	const char *params_query[2] = {name, inchi};
	result_query =
	    PQexecParams(
		database_spectrum,
		"SELECT id FROM molecule WHERE name = $1 AND inchi = $2",
		2,
		NULL,
		params_query,
		NULL,
		NULL,
		0);

	if (PQresultStatus(result_query) != PGRES_TUPLES_OK) {
		PQclear(result_query);
		PQexec(database_spectrum, "ROLLBACK");
		log_error("failed to select molecule (%s, %s)", name, mol_hash(inchi));
		log_error("failed to insert mass spectrum (%s, %s)", name, mol_hash(inchi));
		return -1;
	}

	if (!PQntuples(result_query)) {
		PQclear(result_query);
		result_query =
		    PQexecParams(
			database_spectrum,
			"INSERT INTO molecule (name, inchi) VALUES ($1, $2) RETURNING id",
			2,
			NULL,
			params_query,
			NULL,
			NULL,
			0);

		if (PQresultStatus(result_query) != PGRES_TUPLES_OK) {
			PQclear(result_query);
			PQexec(database_spectrum, "ROLLBACK");
			log_error("failed to insert molecule (%s, %s)", name, mol_hash(inchi));
			log_error("failed to insert mass spectrum (%s, %s)", name, mol_hash(inchi));
			return -1;
		}

		log_info("molecule insert success (%s, %s)", name, mol_hash(inchi));
	}

	char molecule_id[32];
	char molecule_vector[4096];
	char *ptr_vector = molecule_vector;
	snprintf(molecule_id, sizeof(molecule_id), "%d", atoi(PQgetvalue(result_query, 0, 0)));
	ptr_vector += sprintf(ptr_vector, "[");
	for (int32_t i = 0; i < peaks_number; i++) {
		ptr_vector += sprintf(ptr_vector, "%.0f", peaks_data[i]);
		if (i < peaks_number - 1) {
			ptr_vector += sprintf(ptr_vector, ",");
		}
	}

	sprintf(ptr_vector, "]");
	const char *param_query_mol[2] = {(const char *)(molecule_id), (const char *)(molecule_vector)};
	int32_t length_query_mol[2] = {strlen(molecule_id), strlen(molecule_vector)};
	int32_t format_query_mol[2] = {0, 0};
	Oid type_query_mol[2] = {23, 0};
	PQclear(result_query);
	result_query =
	    PQexecParams(
		database_spectrum,
		"INSERT INTO mass_spectrum (molecule_id, spectrum_vector) VALUES ($1, $2)",
		2,
		type_query_mol,
		param_query_mol,
		length_query_mol,
		format_query_mol,
		0);

	if (PQresultStatus(result_query) != PGRES_COMMAND_OK) {
		PQclear(result_query);
		PQexec(database_spectrum, "ROLLBACK");
		log_error("failed to insert mass spectrum (%s, %s)", name, mol_hash(inchi));
		return -1;
	}

	PQclear(result_query);
	result_query = PQexec(database_spectrum, "COMMIT");
	if (PQresultStatus(result_query) != PGRES_COMMAND_OK) {
		PQclear(result_query);
		log_error("failed to commit transaction");
		log_error("failed to insert mass spectrum (%s, %s)", name, mol_hash(inchi));
		return -1;
	}

	PQclear(result_query);
	log_info("mass spectrum insert success (%s, %s)", name, mol_hash(inchi));
	return 0;
}

extern int8_t database_spectrum_insert_nmr(const char *name, const char *inchi, float *peaks_data, int32_t peaks_number) {
	PGresult *result_query = PQexec(database_spectrum, "BEGIN");
	if (PQresultStatus(result_query) != PGRES_COMMAND_OK) {
		PQclear(result_query);
		log_error("failed to begin transaction");
		log_error("failed to insert nmr spectrum (%s, %s)", name, mol_hash(inchi));
		return -1;
	}

	const char *params_query[2] = {name, inchi};
	result_query =
	    PQexecParams(
		database_spectrum,
		"SELECT id FROM molecule WHERE name = $1 AND inchi = $2",
		2,
		NULL,
		params_query,
		NULL,
		NULL,
		0);

	if (PQresultStatus(result_query) != PGRES_TUPLES_OK) {
		PQclear(result_query);
		PQexec(database_spectrum, "ROLLBACK");
		log_error("failed to select molecule (%s, %s)", name, mol_hash(inchi));
		log_error("failed to insert nmr spectrum (%s, %s)", name, mol_hash(inchi));
		return -1;
	}

	if (!PQntuples(result_query)) {
		PQclear(result_query);
		result_query =
		    PQexecParams(
			database_spectrum,
			"INSERT INTO molecule (name, inchi) VALUES ($1, $2) RETURNING id",
			2,
			NULL,
			params_query,
			NULL,
			NULL,
			0);

		if (PQresultStatus(result_query) != PGRES_TUPLES_OK) {
			PQclear(result_query);
			PQexec(database_spectrum, "ROLLBACK");
			log_error("failed to insert molecule (%s, %s)", name, mol_hash(inchi));
			log_error("failed to insert nmr spectrum (%s, %s)", name, mol_hash(inchi));
			return -1;
		}

		log_info("molecule insert success (%s, %s)", name, mol_hash(inchi));
	}

	char molecule_id[32];
	char molecule_vector[4096];
	char *ptr_vector = molecule_vector;
	snprintf(molecule_id, sizeof(molecule_id), "%d", atoi(PQgetvalue(result_query, 0, 0)));
	ptr_vector += sprintf(ptr_vector, "[");
	for (int32_t i = 0; i < peaks_number; i++) {
		ptr_vector += sprintf(ptr_vector, "%.0f", peaks_data[i]);
		if (i < peaks_number - 1) {
			ptr_vector += sprintf(ptr_vector, ",");
		}
	}

	sprintf(ptr_vector, "]");
	const char *param_query_mol[2] = {(const char *)(molecule_id), (const char *)(molecule_vector)};
	int32_t length_query_mol[2] = {strlen(molecule_id), strlen(molecule_vector)};
	int32_t format_query_mol[2] = {0, 0};
	Oid type_query_mol[2] = {23, 0};
	PQclear(result_query);
	result_query =
	    PQexecParams(
		database_spectrum,
		"INSERT INTO nmr_spectrum (molecule_id, spectrum_vector) VALUES ($1, $2)",
		2,
		type_query_mol,
		param_query_mol,
		length_query_mol,
		format_query_mol,
		0);

	if (PQresultStatus(result_query) != PGRES_COMMAND_OK) {
		PQclear(result_query);
		PQexec(database_spectrum, "ROLLBACK");
		log_error("failed to insert nmr spectrum (%s, %s)", name, mol_hash(inchi));
		return -1;
	}

	PQclear(result_query);
	result_query = PQexec(database_spectrum, "COMMIT");
	if (PQresultStatus(result_query) != PGRES_COMMAND_OK) {
		PQclear(result_query);
		log_error("failed to commit transaction");
		log_error("failed to insert nmr spectrum (%s, %s)", name, mol_hash(inchi));
		return -1;
	}

	PQclear(result_query);
	log_info("nmr spectrum insert success (%s, %s)", name, mol_hash(inchi));
	return 0;
}

extern int8_t database_spectrum_insert_optics(const char *name, const char *inchi, float *peaks_data, int32_t peaks_number) {
	PGresult *result_query = PQexec(database_spectrum, "BEGIN");
	if (PQresultStatus(result_query) != PGRES_COMMAND_OK) {
		PQclear(result_query);
		log_error("failed to begin transaction");
		log_error("failed to insert optics spectrum (%s, %s)", name, mol_hash(inchi));
		return -1;
	}

	const char *params_query[2] = {name, inchi};
	result_query =
	    PQexecParams(
		database_spectrum,
		"SELECT id FROM molecule WHERE name = $1 AND inchi = $2",
		2,
		NULL,
		params_query,
		NULL,
		NULL,
		0);

	if (PQresultStatus(result_query) != PGRES_TUPLES_OK) {
		PQclear(result_query);
		PQexec(database_spectrum, "ROLLBACK");
		log_error("failed to select molecule (%s, %s)", name, mol_hash(inchi));
		log_error("failed to insert optics spectrum (%s, %s)", name, mol_hash(inchi));
		return -1;
	}

	if (!PQntuples(result_query)) {
		PQclear(result_query);
		result_query =
		    PQexecParams(
			database_spectrum,
			"INSERT INTO molecule (name, inchi) VALUES ($1, $2) RETURNING id",
			2,
			NULL,
			params_query,
			NULL,
			NULL,
			0);

		if (PQresultStatus(result_query) != PGRES_TUPLES_OK) {
			PQclear(result_query);
			PQexec(database_spectrum, "ROLLBACK");
			log_error("failed to insert molecule (%s, %s)", name, mol_hash(inchi));
			log_error("failed to insert optics spectrum (%s, %s)", name, mol_hash(inchi));
			return -1;
		}

		log_info("molecule insert success (%s, %s)", name, mol_hash(inchi));
	}

	char molecule_id[32];
	char molecule_vector[8192];
	char *ptr_vector = molecule_vector;
	snprintf(molecule_id, sizeof(molecule_id), "%d", atoi(PQgetvalue(result_query, 0, 0)));
	ptr_vector += sprintf(ptr_vector, "[");
	for (int32_t i = 0; i < peaks_number; i++) {
		ptr_vector += sprintf(ptr_vector, "%.0f", peaks_data[i]);
		if (i < peaks_number - 1) {
			ptr_vector += sprintf(ptr_vector, ",");
		}
	}

	sprintf(ptr_vector, "]");
	const char *param_query_mol[2] = {(const char *)(molecule_id), (const char *)(molecule_vector)};
	int32_t length_query_mol[2] = {strlen(molecule_id), strlen(molecule_vector)};
	int32_t format_query_mol[2] = {0, 0};
	Oid type_query_mol[2] = {23, 0};
	PQclear(result_query);
	result_query =
	    PQexecParams(
		database_spectrum,
		"INSERT INTO optics_spectrum (molecule_id, spectrum_vector) VALUES ($1, $2)",
		2,
		type_query_mol,
		param_query_mol,
		length_query_mol,
		format_query_mol,
		0);

	if (PQresultStatus(result_query) != PGRES_COMMAND_OK) {
		log_error("%s", PQerrorMessage(result_query));
		PQclear(result_query);
		PQexec(database_spectrum, "ROLLBACK");
		log_error("failed to insert optics spectrum (%s, %s)", name, mol_hash(inchi));
		return -1;
	}

	PQclear(result_query);
	result_query = PQexec(database_spectrum, "COMMIT");
	if (PQresultStatus(result_query) != PGRES_COMMAND_OK) {
		PQclear(result_query);
		log_error("failed to commit transaction");
		log_error("failed to insert optics spectrum (%s, %s)", name, mol_hash(inchi));
		return -1;
	}

	PQclear(result_query);
	log_info("optics spectrum insert success (%s, %s)", name, mol_hash(inchi));
	return 0;
}

extern float database_spectrum_select_molecule(char *name, char *inchi, int32_t name_size, int32_t inchi_size) {
	static int32_t cursor_query = 0;
	static int32_t count_query = 0;
	static int32_t total_query = 0;
	PGresult *result_query;
	if (!cursor_query) {
		result_query = PQexec(database_spectrum, "SELECT COUNT(*) FROM molecule");
		if (PQresultStatus(result_query) != PGRES_TUPLES_OK) {
			PQclear(result_query);
			log_error("failed to count molecules");
			return -1;
		}

		total_query = atoi(PQgetvalue(result_query, 0, 0));
		PQclear(result_query);
		result_query = PQexec(database_spectrum, "BEGIN");
		if (PQresultStatus(result_query) != PGRES_COMMAND_OK) {
			PQclear(result_query);
			log_error("failed to begin transaction");
			return -1;
		}

		PQclear(result_query);
		result_query = PQexec(database_spectrum, "DECLARE molcursor CURSOR FOR SELECT name, inchi FROM molecule");
		if (PQresultStatus(result_query) != PGRES_COMMAND_OK) {
			PQclear(result_query);
			log_error("failed to select next molecule");
			return -1;
		}

		PQclear(result_query);
		cursor_query = 1;
	}

	result_query = PQexec(database_spectrum, "FETCH NEXT FROM molcursor");
	if (PQresultStatus(result_query) != PGRES_TUPLES_OK) {
		PQclear(result_query);
		log_error("failed to fetch next molecule");
		return -1;
	}

	int32_t nrows = PQntuples(result_query);
	if (!nrows) {
		cursor_query = 0;
		count_query = 0;
		PQclear(result_query);
		PQexec(database_spectrum, "CLOSE molcursor");
		PQexec(database_spectrum, "END");
		return 0;
	}

	const char *result_name = PQgetvalue(result_query, 0, 0);
	const char *result_inchi = PQgetvalue(result_query, 0, 1);
	snprintf(name, name_size, "%s", result_name);
	snprintf(inchi, inchi_size, "%s", result_inchi);
	PQclear(result_query);
	log_info("molecule select success (%s, %s, %.3lf%%)", name, mol_hash(inchi), 100.000 * (float)(++count_query) / (float)total_query);
	return (float)count_query / (float)total_query;
}

extern float database_spectrum_select_mass(char *name, char *inchi, float *peaks_data, int32_t name_size, int32_t inchi_size, int32_t peaks_number) {
	char molecule_vector[4096];
	char *ptr_vector = molecule_vector;
	ptr_vector += sprintf(ptr_vector, "[");
	for (int32_t i = 0; i < peaks_number; i++) {
		ptr_vector += sprintf(ptr_vector, "%.0f", peaks_data[i]);
		if (i < peaks_number - 1) {
			ptr_vector += sprintf(ptr_vector, ",");
		}
	}

	sprintf(ptr_vector, "]");
	const char *param_query[1] = {molecule_vector};
	PGresult *result_query = PQexecParams(
	    database_spectrum,
	    "SELECT molecule.name, molecule.inchi, (mass_spectrum.spectrum_vector <-> $1) AS distance "
	    "FROM mass_spectrum "
	    "JOIN molecule ON mass_spectrum.molecule_id = molecule.id "
	    "ORDER BY mass_spectrum.spectrum_vector <-> $1 "
	    "LIMIT 1",
	    1,
	    NULL,
	    param_query,
	    NULL,
	    NULL,
	    0);

	if (PQresultStatus(result_query) != PGRES_TUPLES_OK) {
		PQclear(result_query);
		log_error("failed to select mass spectrum");
		return -1;
	}

	if (PQntuples(result_query) < 1) {
		PQclear(result_query);
		log_error("failed to select mass spectrum");
		return -1;
	}

	const char *result_name = PQgetvalue(result_query, 0, 0);
	const char *result_inchi = PQgetvalue(result_query, 0, 1);
	const char *result_similarity = PQgetvalue(result_query, 0, 2);
	snprintf(name, name_size, "%s", result_name);
	snprintf(inchi, inchi_size, "%s", result_inchi);
	float distance = strtof(result_similarity, NULL);
	float similarity = 1.0 / (1.0 + distance / (float)peaks_number);
	PQclear(result_query);
	log_info("mass specturm select success (%s, %s, %.03f%%)", name, mol_hash(inchi), similarity * 100);
	return similarity;
}

extern float database_spectrum_select_nmr(char *name, char *inchi, float *peaks_data, int32_t name_size, int32_t inchi_size, int32_t peaks_number) {
	char molecule_vector[4096];
	char *ptr_vector = molecule_vector;
	ptr_vector += sprintf(ptr_vector, "[");
	for (int32_t i = 0; i < peaks_number; i++) {
		ptr_vector += sprintf(ptr_vector, "%.0f", peaks_data[i]);
		if (i < peaks_number - 1) {
			ptr_vector += sprintf(ptr_vector, ",");
		}
	}

	sprintf(ptr_vector, "]");
	const char *param_query[1] = {molecule_vector};
	PGresult *result_query = PQexecParams(
	    database_spectrum,
	    "SELECT molecule.name, molecule.inchi, (nmr_spectrum.spectrum_vector <-> $1) AS distance "
	    "FROM nmr_spectrum "
	    "JOIN molecule ON nmr_spectrum.molecule_id = molecule.id "
	    "ORDER BY nmr_spectrum.spectrum_vector <-> $1 "
	    "LIMIT 1",
	    1,
	    NULL,
	    param_query,
	    NULL,
	    NULL,
	    0);

	if (PQresultStatus(result_query) != PGRES_TUPLES_OK) {
		PQclear(result_query);
		log_error("failed to select nmr spectrum");
		return -1;
	}

	if (PQntuples(result_query) < 1) {
		PQclear(result_query);
		log_error("failed to select nmr spectrum");
		return -1;
	}

	const char *result_name = PQgetvalue(result_query, 0, 0);
	const char *result_inchi = PQgetvalue(result_query, 0, 1);
	const char *result_similarity = PQgetvalue(result_query, 0, 2);
	snprintf(name, name_size, "%s", result_name);
	snprintf(inchi, inchi_size, "%s", result_inchi);
	float distance = strtof(result_similarity, NULL);
	float similarity = 1.0 / (1.0 + distance);
	PQclear(result_query);
	log_info("nmr specturm select success (%s, %s, %.03f%%)", name, mol_hash(inchi), similarity * 100);
	return similarity;
}

extern float database_spectrum_select_optics(char *name, char *inchi, float *peaks_data, int32_t name_size, int32_t inchi_size, int32_t peaks_number) {
	char molecule_vector[8192];
	char *ptr_vector = molecule_vector;
	ptr_vector += sprintf(ptr_vector, "[");
	for (int32_t i = 0; i < peaks_number; i++) {
		ptr_vector += sprintf(ptr_vector, "%.0f", peaks_data[i]);
		if (i < peaks_number - 1) {
			ptr_vector += sprintf(ptr_vector, ",");
		}
	}

	sprintf(ptr_vector, "]");
	const char *param_query[1] = {molecule_vector};
	PGresult *result_query = PQexecParams(
	    database_spectrum,
	    "SELECT molecule.name, molecule.inchi, (optics_spectrum.spectrum_vector <-> $1) AS distance "
	    "FROM optics_spectrum "
	    "JOIN molecule ON optics_spectrum.molecule_id = molecule.id "
	    "ORDER BY optics_spectrum.spectrum_vector <-> $1 "
	    "LIMIT 1",
	    1,
	    NULL,
	    param_query,
	    NULL,
	    NULL,
	    0);

	if (PQresultStatus(result_query) != PGRES_TUPLES_OK) {
		PQclear(result_query);
		log_error("failed to select optics spectrum");
		return -1;
	}

	if (PQntuples(result_query) < 1) {
		PQclear(result_query);
		log_error("failed to select optics spectrum");
		return -1;
	}

	const char *result_name = PQgetvalue(result_query, 0, 0);
	const char *result_inchi = PQgetvalue(result_query, 0, 1);
	const char *result_similarity = PQgetvalue(result_query, 0, 2);
	snprintf(name, name_size, "%s", result_name);
	snprintf(inchi, inchi_size, "%s", result_inchi);
	float distance = strtof(result_similarity, NULL);
	float similarity = 1.0 / (1.0 + distance / 5.0);
	PQclear(result_query);
	log_info("optics specturm select success (%s, %s, %.03f%%)", name, mol_hash(inchi), similarity * 100);
	return similarity;
}