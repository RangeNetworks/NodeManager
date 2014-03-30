/*
* Copyright 2013, 2014 Range Networks, Inc.
*
* This software is distributed under the terms of the GNU Affero Public License.
* See the COPYING file in the main directory for details.
*
* This use of this software may be subject to additional restrictions.
* See the LEGAL file in the main directory for details.

	This program is free software: you can redistribute it and/or modify
	it under the terms of the GNU Affero General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU Affero General Public License for more details.

	You should have received a copy of the GNU Affero General Public License
	along with this program.  If not, see <http://www.gnu.org/licenses/>.

*/

#include "sqlite3util.h"
#include "sqlite3.h"
#include "JSONDB.h"

#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <sstream>



bool JSONDB::connect(std::string path)
{
	if (!sqlite3_open(path.c_str(), &mDB)) {
		return true;
	}

	disconnect();

	return false;
}

bool JSONDB::disconnect()
{
	sqlite3_close(mDB);
	mDB = NULL;

	return true;
}

std::string JSONDB::generateWhereClause(JsonBox::Object request)
{
	std::stringstream ss;
	std::stringstream ssss;
	JsonBox::Object match = request["match"].getObject();

	if (match.size() > 0) {
		JsonBox::Object::iterator jit;
		std::vector<std::string> pairs;
		for (jit = match.begin(); jit != match.end(); jit++) {
			ssss.str("");
			ssss << jit->first << "=\"" << jit->second.getString() << "\"";
			pairs.push_back(ssss.str());
		}
		ss << " where " << implode(" and ", pairs);
	}

	return ss.str();
}

std::string JSONDB::implode(std::string delimiter, std::vector<std::string> pieces)
{
	std::stringstream ss;

	for (unsigned i = 0; i < pieces.size(); i++) {
		if (i != 0) {
			ss << delimiter;
		}
		ss << pieces[i];
	}

	return ss.str();
}

JsonBox::Object JSONDB::read(std::string query, unsigned retries)
{
	JsonBox::Object response;
	sqlite3_stmt *stmt;

	// bail if prep fails
	if (sqlite3_prepare_statement(mDB, &stmt, query.c_str(), retries)) {
		response["code"] = JsonBox::Value(406);
		response["data"] = JsonBox::Value("query prep failed");
		return response;
	}

	JsonBox::Object o;
	JsonBox::Array a;
	int state;
	int col;
	int colMax = sqlite3_column_count(stmt);
	const char* colName;
	const char* colText;
	while (1) {
		state = sqlite3_step(stmt);
		if (state == SQLITE_ROW) {
			for (col = 0; col < colMax; col++) {
				colName = (const char*)sqlite3_column_name(stmt, col);
				colText = (const char*)sqlite3_column_text(stmt, col);
				if (colText == NULL) {
					o[std::string(colName)] = JsonBox::Value();
				} else {
					o[std::string(colName)] = JsonBox::Value(std::string(colText));
				}
			}
		} else if (state == SQLITE_DONE) {
			break;
		// TODO : handle more SQLITE_FAILURECODES here
		} else {
			response["code"] = JsonBox::Value(406);
			response["data"] = JsonBox::Value("query run failed");
			return response;
		}
		a.push_back(o);
	}
	sqlite3_finalize(stmt);

	if (a.size() == 0) {
		response["code"] = JsonBox::Value(404);
	} else {
		response["code"] = JsonBox::Value(200);
		response["data"] = a;
	}

	return response;
}

JsonBox::Object JSONDB::execOnly(std::string query, unsigned retries)
{
	JsonBox::Object response;
	sqlite3_stmt *stmt;

	// bail if prep fails
	if (sqlite3_prepare_statement(mDB, &stmt, query.c_str(), retries)) {
		response["code"] = JsonBox::Value(406);
		response["data"] = JsonBox::Value("query prep failed");
		return response;
	}

	int state = sqlite3_step(stmt);
	if (state == SQLITE_ROW) {
		response["code"] = JsonBox::Value(200);
		response["data"] = JsonBox::Value("SQLITE_ROW");
	} else if (state == SQLITE_DONE) {
		response["code"] = JsonBox::Value(200);
		response["data"] = JsonBox::Value("SQLITE_DONE");
	} else if (state == SQLITE_BUSY) {
		response["code"] = JsonBox::Value(503);
		response["data"] = JsonBox::Value("SQLITE_BUSY");
	} else if (state == SQLITE_LOCKED) {
		response["code"] = JsonBox::Value(503);
		response["data"] = JsonBox::Value("SQLITE_LOCKED");
	} else if (state == SQLITE_CONSTRAINT) {
		response["code"] = JsonBox::Value(503);
		response["data"] = JsonBox::Value("SQLITE_CONSTRAINT");
	} else {
		response["code"] = JsonBox::Value(503);
		response["data"] = JsonBox::Value(state);
	}
	sqlite3_finalize(stmt);

	return response;
}

JsonBox::Object JSONDB::query(JsonBox::Object request, unsigned retries)
{
	JsonBox::Object response;
	std::stringstream q;
	std::string table;
	std::string dbPath;
	std::string action;

	// check if invalid table
	// TODO : these path mappings should be pulled from the configuration file
	table = request["table"].getString();
	if (table.length() == 0) {
		response["code"] = JsonBox::Value(406);
		response["data"] = JsonBox::Value("missing table");
		return response;
	} else if (table.compare("sip_buddies") == 0 || table.compare("dialdata_table") == 0 || table.compare("rates") == 0 || table.compare("subscribers") == 0) {
		dbPath = "/var/lib/asterisk/sqlite3dir/sqlite3.db";
	// TODO : should query OpenBTS directly because this table may dissappear
	} else if (table.compare("neighbor_table") == 0) {
		dbPath = "/var/run/NeighborTable.db";
	// TODO : should query OpenBTS directly because this table may dissappear
	} else if (table.compare("phystatus") == 0) {
		dbPath = "/var/run/ChannelTable.db";
	// TODO : should query OpenBTS directly because this table may dissappear
	} else if (table.compare("tmsi_table") == 0) {
		dbPath = "/var/run/TMSITable.db";
	// TODO : should query OpenBTS directly because this table may dissappear
	} else if (table.compare("transaction_table") == 0) {
		dbPath = "/var/run/TransactionTable.db";
	} else if (table.compare("spectrum_map") == 0) {
		dbPath = "/var/run/PowerScannerResults.db";
	} else {
		response["code"] = JsonBox::Value(406);
		response["data"] = JsonBox::Value("invalid table");
		return response;
	}

	// bail if missing action
	action = request["action"].getString();
	if (action.length() == 0) {
		response["code"] = JsonBox::Value(406);
		response["data"] = JsonBox::Value("missing action");
		return response;
	}

	// connect to database
	if (!connect(dbPath)) {
		response["code"] = JsonBox::Value(500);
		response["data"] = JsonBox::Value("db connect failed");
		return response;
	}

	// abstract sip_buddies and dialdata_tables into a single logical endpoint
	if (table.compare("subscribers") == 0) {
		if (action.compare("create") == 0) {
			JsonBox::Object fields = request["fields"].getObject();
			JsonBox::Object::iterator jit;

			jit = fields.find("name");
			if (jit == fields.end()) {
				response["code"] = JsonBox::Value(406);
				response["data"] = JsonBox::Value("missing name");
				return response;
			}
			std::string name = jit->second.getString();

			jit = fields.find("imsi");
			if (jit == fields.end()) {
				response["code"] = JsonBox::Value(406);
				response["data"] = JsonBox::Value("missing imsi");
				return response;
			}
			std::string imsi = jit->second.getString();

			jit = fields.find("msisdn");
			if (jit == fields.end()) {
				response["code"] = JsonBox::Value(406);
				response["data"] = JsonBox::Value("missing msisdn");
				return response;
			}
			std::string msisdn = jit->second.getString();

			jit = fields.find("ki");
			if (jit == fields.end()) {
				response["code"] = JsonBox::Value(406);
				response["data"] = JsonBox::Value("missing ki");
				return response;
			}
			std::string ki = jit->second.getString();

			std::stringstream queryTMP;
			JsonBox::Object responseSIP;
			JsonBox::Object responseDIAL;

			queryTMP << "insert into sip_buddies (username, name, callerid, ki, host, allow, ipaddr)";
			queryTMP << " values (\"" << imsi << "\",\"" << name << "\",\"" << msisdn << "\",\"" << ki << "\",\"dynamic\",\"gsm\",\"127.0.0.1\")";
			responseSIP = execOnly(queryTMP.str());

			queryTMP.str("");
			queryTMP << "insert into dialdata_table (dial, exten) values (\"" << imsi << "\",\"" << msisdn << "\")";
			responseDIAL = execOnly(queryTMP.str());

			if (responseSIP["code"].getInt() == 200 && responseDIAL["code"].getInt() == 200) {
				response["code"] = JsonBox::Value(200);
				response["data"] = JsonBox::Value("both ok");
			} else {
				response["code"] = JsonBox::Value(503);
				response["data"]["sip_buddies"] = responseSIP;
				response["data"]["dialdata_table"] = responseDIAL;
			}

		} else if (action.compare("read") == 0) {
			q << "select sip_buddies.username as \"imsi\", sip_buddies.name as \"name\", dialdata_table.exten as \"msisdn\" from sip_buddies left outer join dialdata_table on sip_buddies.username = dialdata_table.dial";
			return read(q.str());

		} else if (action.compare("update") == 0) {
			JsonBox::Object match = request["match"].getObject();
			JsonBox::Object fields = request["fields"].getObject();
			JsonBox::Object::iterator jit;

			jit = match.find("imsi");
			if (jit == match.end()) {
				response["code"] = JsonBox::Value(406);
				response["data"] = JsonBox::Value("missing imsi to match on");
				return response;
			}
			std::string imsi = jit->second.getString();

			jit = fields.find("name");
			if (jit == fields.end()) {
				response["code"] = JsonBox::Value(406);
				response["data"] = JsonBox::Value("missing name to update");
				return response;
			}
			std::string name = jit->second.getString();

			jit = fields.find("msisdn");
			if (jit == fields.end()) {
				response["code"] = JsonBox::Value(406);
				response["data"] = JsonBox::Value("missing msisdn to update");
				return response;
			}
			std::string msisdn = jit->second.getString();

			std::stringstream queryTMP;
			JsonBox::Object responseSIP;
			JsonBox::Object responseDIAL;

			queryTMP << "update sip_buddies set name=\"" << name << "\", callerid=\"" << msisdn << "\" where username=\"" << imsi << "\"";
			responseSIP = execOnly(queryTMP.str());

			queryTMP.str("");
			queryTMP << "update dialdata_table set exten=\"" << msisdn << "\" where dial=\"" << imsi << "\"";
			responseDIAL = execOnly(queryTMP.str());

			if (responseSIP["code"].getInt() == 200 && responseDIAL["code"].getInt() == 200) {
				response["code"] = JsonBox::Value(200);
				response["data"] = JsonBox::Value("both ok");
			} else {
				response["code"] = JsonBox::Value(503);
				response["data"]["sip_buddies"] = responseSIP;
				response["data"]["dialdata_table"] = responseDIAL;
			}

		} else if (action.compare("delete") == 0) {
			JsonBox::Object match = request["match"].getObject();
			JsonBox::Object::iterator jit = match.find("imsi");

			if (jit == match.end()) {
				response["code"] = JsonBox::Value(406);
				response["data"] = JsonBox::Value("missing imsi");
				return response;
			}
			std::string imsi = jit->second.getString();

			std::stringstream queryTMP;
			JsonBox::Object responseSIP;
			JsonBox::Object responseDIAL;

			queryTMP << "delete from sip_buddies where username=\"" << imsi << "\"";
			responseSIP = execOnly(queryTMP.str());

			queryTMP.str("");
			queryTMP << "delete from dialdata_table where dial=\"" << imsi << "\"";
			responseDIAL = execOnly(queryTMP.str());

			if (responseSIP["code"].getInt() == 200 && responseDIAL["code"].getInt() == 200) {
				response["code"] = JsonBox::Value(200);
				response["data"] = JsonBox::Value("both ok");
			} else {
				response["code"] = JsonBox::Value(503);
				response["data"]["sip_buddies"] = responseSIP;
				response["data"]["dialdata_table"] = responseDIAL;
			}

		}

	// CREATE
	} else if (action.compare("create") == 0) {
		// start query
		q << "insert into " << table;

		// extract new fields
		JsonBox::Object fields = request["fields"].getObject();

		// bail if fields is missing
		if (fields.size() == 0) {
			response["code"] = JsonBox::Value(406);
			response["data"] = JsonBox::Value("missing fields data");
			return response;
		}

		// convert JSON object array into query string
		JsonBox::Object::iterator jit;
		std::vector<std::string> colNames;
		std::vector<std::string> colValues;
		for (jit = fields.begin(); jit != fields.end(); jit++) {
			colNames.push_back(jit->first);
			colValues.push_back(jit->second.getString());
		}
		q << " (" << implode(",", colNames) << ") values (" << "\"" << implode("\",\"", colValues) << "\")";

		return execOnly(q.str());

	// READ
	} else if (action.compare("read") == 0) {
		// start query
		q << "select * from " << table;

		// add match clause
		q << generateWhereClause(request);

		return read(q.str());

	// UPDATE
	} else if (action.compare("update") == 0) {
		// start query
		q << "update " << table << " set";

		// extract new fields
		JsonBox::Object fields = request["fields"].getObject();

		// bail if fields is missing
		if (fields.size() == 0) {
			response["code"] = JsonBox::Value(406);
			response["data"] = JsonBox::Value("missing new fields data");
			return response;
		}

		// convert JSON object array into new fields string
		std::stringstream ssss;
		JsonBox::Object::iterator jit;
		std::vector<std::string> pairs;
		for (jit = fields.begin(); jit != fields.end(); jit++) {
			ssss.str("");
			ssss << jit->first << "=\"" << jit->second.getString() << "\"";
			pairs.push_back(ssss.str());
		}
		q << " " << implode(",", pairs);

		// add match clause
		q << generateWhereClause(request);

		return execOnly(q.str());

	// DELETE
	} else if (action.compare("delete") == 0) {
		// start query
		q << "delete from " << table;

		// add match clause
		q << generateWhereClause(request);

		return execOnly(q.str());

	} else {
		response["code"] = JsonBox::Value(406);
		response["data"] = JsonBox::Value("invalid action");
		return response;
	}

	disconnect();

	return response;
}
