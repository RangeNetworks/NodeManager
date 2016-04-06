/*
* Copyright 2013, 2014 Range Networks, Inc.
*
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

#include "NodeManager.h"
#include <stdio.h>

// TODO : not a member of NodeManager, fails to compile...
void* NodeManager::commandsLoop(void*)
{
	// TODO : i hate this, need to figure out a way to satisfy the compiler and pass
	// NodeManager::worker directly to mServer.start();
	// (pat) You make it a static method and call it via NodeManager::commandLoop.
	// In general, 'this' would be passed as the void* arg, which allows multiple instances, instead of using gNodeManager.

	extern NodeManager gNodeManager;
	try {
		// This errors out when OpenBTS exits.
		gNodeManager.commandsWorker(NULL);
	} catch (...) {
		LOG(INFO) << "Exception occurred in NodeManager";	// (pat 3-2014) added.  Evidently this happens regularly, so make it an INFO message.
	}
	return NULL;
}

void* NodeManager::commandsWorker(void *)
{
	while(1) {
		JsonBox::Value in;
		JsonBox::Value out;
		JsonBox::Object jsonIn;
		JsonBox::Object jsonOut;

		in.loadFromString(readRequest());
		jsonIn = in.getObject();
		std::string command = jsonIn["command"].getString();

		if (command.compare("version") == 0) {
			jsonOut = version(jsonIn);
		} else if (command.compare("config") == 0) {
			jsonOut = config(jsonIn);
		} else if (mAppLogicHandler != NULL) {
			jsonOut = mAppLogicHandler(jsonIn);
		} else {
			jsonOut["code"] = JsonBox::Value(501);
		}

		std::stringstream jsonResponse;
		JsonBox::Value(jsonOut).writeToStream(jsonResponse);

		writeResponse(jsonResponse.str());
	}

	return NULL;
}

void NodeManager::start(int commandsPort, int eventsPort)
{
	char commandsAddress[24];
	char eventsAddress[24];

	signal(SIGPIPE, SIG_IGN);
	snprintf(commandsAddress, sizeof(commandsAddress), "tcp://127.0.0.1:%d", commandsPort);
	// (pat 3-2014) The zmq library throws a standard exception if it cannot bind, with no useful description in the exception whatsoever.
	try {
		mCommandsSocket.bind(commandsAddress);
	} catch (...) {
		LOG(EMERG) << "Could not bind commandsAddress: " << commandsAddress << " (possibly in use?)  Exiting...";
		exit(0);	// Thats it.
	}
	mCommandsServer.start(NodeManager::commandsLoop, NULL);

	if (eventsPort) {
		snprintf(eventsAddress, sizeof(eventsAddress), "tcp://127.0.0.1:%d", eventsPort);
		try {
			mEventsSocket.bind(eventsAddress);
		} catch (...) {
			LOG(EMERG) << "Could not bind eventsAddress: " << eventsAddress << " (possibly in use?)  Exiting...";
			exit(0);
		}
	}
}

std::string NodeManager::readRequest()
{
	zmq::message_t payload;
	mCommandsSocket.recv(&payload);
	return std::string(static_cast<char*>(payload.data()), payload.size());
}

void NodeManager::writeResponse(const std::string& message)
{
	zmq::message_t payload(message.length());
	memcpy((void *) payload.data(), message.c_str(), message.length());
	mCommandsSocket.send(payload);
}

void NodeManager::publishEvent(const std::string& name, const std::string& version, const JsonBox::Object& data)
{
	ScopedLock lock(mLock);
	std::stringstream ss;
	JsonBox::Object e;
	struct timeval tv;
	std::string timestamp;

	gettimeofday(&tv, NULL);
	unsigned long long tmp = tv.tv_sec * 1000LL + tv.tv_usec / 1000;
	ss << tmp;
	timestamp = ss.str();
	ss.str("");

	e["name"] = JsonBox::Value(name);
	e["version"] = JsonBox::Value(version);
	e["timestamp"] = JsonBox::Value(timestamp);
	e["data"] = JsonBox::Object(data);

	JsonBox::Value(e).writeToStream(ss);
	std::string message = ss.str();

	zmq::message_t payload(message.length());
	memcpy((void *) payload.data(), message.c_str(), message.length());
	mEventsSocket.send(payload);
}

JsonBox::Object NodeManager::version(JsonBox::Object command)
{
	JsonBox::Object response;

	response["code"] = JsonBox::Value(200);
	response["data"] = JsonBox::Value(gVersionString);

	return response;
}

JsonBox::Object NodeManager::configKeyToJSON(const std::string& key)
{
	JsonBox::Object o;

	o["key"] = JsonBox::Value(key);
	o["value"] = JsonBox::Value(gConfig.getStr(key));
	o["description"] = gConfig.mSchema[key].getDescription();
	o["units"] = gConfig.mSchema[key].getUnits();
	o["type"] = ConfigurationKey::typeToString(gConfig.mSchema[key].getType());
	o["defaultValue"] = gConfig.mSchema[key].getDefaultValue();
	o["validValues"] = gConfig.mSchema[key].getValidValues();
	o["visibility"] = ConfigurationKey::visibilityLevelToString(gConfig.mSchema[key].getVisibility());
	o["static"] = gConfig.mSchema[key].isStatic();
	o["scope"] = JsonBox::Value((int)gConfig.mSchema[key].getScope());

	if (gConfig.mSchema[key].getType() == ConfigurationKey::VALRANGE) {
		std::string min;
		std::string max;
		std::string stepping;

		ConfigurationKey::getMinMaxStepping(gConfig.mSchema[key], min, max, stepping);
		o["min"] = min;
		o["max"] = max;
		o["stepping"] = stepping;
	}

	return o;
}

JsonBox::Object NodeManager::config(JsonBox::Object command)
{
	JsonBox::Object response;
	std::string action = command["action"].getString();
	std::string key = command["key"].getString();

	if (action.compare("read") == 0) {
		if (key.length() == 0) {
			ConfigurationKeyMap::iterator mp;
			JsonBox::Array a;
			JsonBox::Object o;

			mp = gConfig.mSchema.begin();
			while (mp != gConfig.mSchema.end()) {
				a.push_back(configKeyToJSON(mp->first));
				mp++;
			}
			response["code"] = JsonBox::Value(200);
			response["data"] = a;
		} else if (!gConfig.keyDefinedInSchema(key)) {
			response["code"] = JsonBox::Value(404);
		} else {
			response["code"] = JsonBox::Value(200);
			response["data"] = configKeyToJSON(key);
		}

	} else if (action.compare("update") == 0) {
		std::string value = command["value"].getString();

		// bail if we don't have this key in the schema
		if (!gConfig.keyDefinedInSchema(key)) {
			response["code"] = JsonBox::Value(404);
			return response;
		}

		// bail if the new value is invalid
		if (!gConfig.isValidValue(key, value)) {
			response["code"] = JsonBox::Value(406);
			return response;
		}

		// we're going to try to update the value, grab the old one
		std::string oldValue = gConfig.getStr(key);

		// bail if we're not actually changing anything here
		if (value.compare(oldValue) == 0) {
			response["code"] = JsonBox::Value(304);
			return response;
		}

		if (!gConfig.set(key, value)) {
			response["code"] = JsonBox::Value(500);
		} else {
			if (gConfig.mSchema[key].isStatic()) {
				addDirtyConfigurationKey(key, oldValue, value);
			}
			if (mAppConfigChangeHandler != NULL) {
				mAppConfigChangeHandler(key, oldValue, value);
			}
			response["dirty"] = JsonBox::Value(getDirtyConfigurationKeyCount());
			response["code"] = JsonBox::Value(204);
		}

	} else if (action.compare("dirty") == 0) {
		response["code"] = JsonBox::Value(200);
		response["data"] = JsonBox::Value(getDirtyConfigurationKeys());

	} else {
		response["code"] = JsonBox::Value(501);
	}

	return response;
}

void NodeManager::setAppLogicHandler(JsonBox::Object (*wAppLogicHandler)(JsonBox::Object&))
{
	mAppLogicHandler = wAppLogicHandler;
}

void NodeManager::setAppConfigChangeHandler(bool (*wAppConfigChangeHandler)(std::string&, std::string&, std::string&))
{
	mAppConfigChangeHandler = wAppConfigChangeHandler;
}

int NodeManager::addDirtyConfigurationKey(std::string& name, std::string& oldValue, std::string& newValue)
{
	std::map<std::string, std::string>::iterator it = mDirtyConfigurationKeys.find(name);

	// key is new to us, just add to map
	if (it == mDirtyConfigurationKeys.end()) {
		mDirtyConfigurationKeys[name] = oldValue;

	// key was already marked dirty, lets see if we can mark it clean
	} else if (mDirtyConfigurationKeys[name] == newValue) {
		mDirtyConfigurationKeys.erase(it);
	}

	return mDirtyConfigurationKeys.size();
}

JsonBox::Object NodeManager::getDirtyConfigurationKeys()
{
	JsonBox::Object o;
	std::map<std::string, std::string>::iterator it = mDirtyConfigurationKeys.begin();

	while (it != mDirtyConfigurationKeys.end()) {
		o[it->first] = it->second;
		it++;
	}

	return o;
}
