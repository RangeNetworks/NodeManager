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
		// TODO : post WebUI NG MVP
		//} else if (command.compare("trace") == 0) {
		//	jsonOut = trace(jsonIn);
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

// TODO : not a member of NodeManager, fails to compile...
// A very fine but unused function.
static void* eventsLoop(void*)
{
	// TODO : i hate this, need to figure out a way to satisfy the compiler and pass
	// NodeManager::worker directly to mServer.start();
	extern NodeManager gNodeManager;
	gNodeManager.eventsWorker(NULL);
	return NULL;
}

void* NodeManager::eventsWorker(void *)
{
	while (1) {
		if (connectedSockets[connectedSocketIndex] != -1) {
			std::cout << "NodeManager::eventsWorker : connectedSockets[" << connectedSocketIndex << "] is taken" << std::endl;
			sleep(1);
		} else {
			if ((connectedSockets[connectedSocketIndex] = accept(listeningSocket, NULL, NULL)) < 0) {
				std::cerr << "NodeManager::eventsWorker : connectedSockets[" << connectedSocketIndex << "] error on accept()" << std::endl;
			} else {
				std::cout << "NodeManager::eventsWorker : connectedSockets[" << connectedSocketIndex << "] new connection" << std::endl;
			}
		}

		connectedSocketIndex++;
		connectedSocketIndex = connectedSocketIndex % maxConnectedSockets;
	}

	return NULL;
}

// TODO : post WebUI NG MVP
void NodeManager::start(int commandsPort)//, int eventsPort)
{
	char address[24];

	signal(SIGPIPE, SIG_IGN);
	snprintf(address, sizeof(address), "tcp://127.0.0.1:%d", commandsPort);
	// (pat 3-2014) The zmq library throws a standard exception if it cannot bind, with no useful description in the exception whatsoever.
	try {
		mCommandsSocket.bind(address);
	} catch (...) {
		LOG(EMERG) << "Could not bind address: " << address << " (possibly in use?)  Exiting...";
		exit(0);	// Thats it.
	}
	mCommandsServer.start(NodeManager::commandsLoop, NULL);

	// TODO : post WebUI NG MVP
	//// create the listening socket
	//if ((listeningSocket = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
	//	std::cerr << "Events Publisher : error " << listeningSocket << ":" << errno << " on socket()" << std::endl;
	//	exit(EXIT_FAILURE);
	//}
    //
	//// allow immediate re-binding if kernel's still hanging in CLOSE_WAIT
	//int optFlag = 1;
	//setsockopt(listeningSocket, SOL_SOCKET, SO_REUSEADDR, (char*)&optFlag, sizeof(optFlag));
    //
	//// setup bind options
	//memset(&eventssrvaddr, 0, sizeof(eventssrvaddr));
	//eventssrvaddr.sin_family = AF_INET;
	//eventssrvaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	//eventssrvaddr.sin_port = htons(eventsPort);
    //
	//// keep trying to bind() annoyingly
	//int ret;
	//while (true) {
	//	if ((ret = bind(listeningSocket, (struct sockaddr *) &eventssrvaddr, sizeof(eventssrvaddr))) < 0) {
	//		std::cerr << "Events Publisher : error " << ret << ":" << errno << " on bind()" << std::endl;
	//		sleep(1);
	//		continue;
	//	} else {
	//		break;
	//	}
	//}
	//// keep trying to listen() annoyingly
	//while (true) {
	//	if ((ret = listen(listeningSocket, 128)) < 0) {
	//		std::cerr << "Events Publisher : error " << ret << ":" << errno << " on listen()" << std::endl;
	//		sleep(1);
	//		continue;
	//	} else {
	//		break;
	//	}
	//}
    //
	//mEventsServer.start(eventsLoop, NULL);
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

// TODO : post WebUI NG MVP
//void NodeManager::publishEvent(const std::string& message)
//{
//	int remaining;
//	int written;
//	const char *buffer;
//
//	std::string delimited = std::string(message + "\r\n\r\n");
//
//	for (int i = 0; i < maxConnectedSockets; i++) {
//		if (connectedSockets[i] == -1) {
//			continue;
//		}
//
//		written = 0;
//		buffer = delimited.c_str();
//		remaining  = delimited.length();
//
//		while (remaining > 0) {
//			if ((written = write(connectedSockets[i], buffer, remaining)) <= 0) {
//				if (errno == EINTR) {
//					std::cout << "NodeManager::publishEvent : connectedSockets[" << connectedSocketIndex << "] EINTR" << std::endl;
//					written = 0;
//				} else {
//					std::cout << "NodeManager::publishEvent : connectedSockets[" << connectedSocketIndex << "] big error (" << errno << "), closing" << std::endl;
//					if (close(connectedSockets[i]) < 0) {
//						std::cerr << "NodeManager::publishEvent : connectedSockets[" << connectedSocketIndex << "] error on close()" << std::endl;
//					}
//					connectedSockets[i] = -1;
//					return;
//				}
//			}
//			remaining -= written;
//			buffer += written;
//		}
//	}
//
//	return;
//}

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

// TODO : this should actually be in nmcgi.cpp but the webserver does not run as root, permissions problem needs to be solved
// TODO : add action=start and return UUID of file, follow-up command of action=stop with UUID as reference stops trace
// TODO : post WebUI NG MVP
//JsonBox::Object NodeManager::trace(JsonBox::Object command)
//{
//	JsonBox::Object response;
//	std::stringstream buf;
//
//	std::string filename = command["filename"].getString();
//	std::string interface = command["interface"].getString();
//	std::string filter = command["filter"].getString();
//	std::string seconds = command["seconds"].getString();
//
//	// TODO : redirect stdout and stderr, for now leave on console for easier debugging
//	buf << "tcpdump -s0 -nn -w " << filename << " -i " << interface << " -G " << seconds << " -W 1 " << filter << " &";
//	system(buf.str().c_str());
//
//	response["code"] = JsonBox::Value(204);
//
//	return response;
//}

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
