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


#ifndef NODEMANAGER_H
#define NODEMANAGER_H


#include <Globals.h>
#include <Logger.h>
#include <algorithm>
#include <JsonBox.h>
#include <sys/signal.h>
#include <sys/time.h>	// For gettimeofday
#include <string>
#include <map>
#include <Threads.h>
#include <zmq.hpp>

class NodeManager {

	private:

	zmq::context_t mContext;
	zmq::socket_t mCommandsSocket;
	zmq::socket_t mEventsSocket;
	Thread mCommandsServer;
	mutable Mutex mLock;
	JsonBox::Object (*mAppLogicHandler)(JsonBox::Object&);
 	bool (*mAppConfigChangeHandler)(std::string&, std::string&, std::string&);
	std::map<std::string, std::string> mDirtyConfigurationKeys;

	std::string readRequest();
	void writeResponse(const std::string& message);
	JsonBox::Object configKeyToJSON(const std::string& key);

	public:

	NodeManager()
		:mContext(4),
		mCommandsSocket(mContext, ZMQ_REP),
		mEventsSocket(mContext, ZMQ_PUB),
		mAppLogicHandler(NULL),
		mAppConfigChangeHandler(NULL)
	{ }

	void start(int commandsPort, int eventsPort=0);
	void* commandsWorker(void*);
	static void* commandsLoop(void*);
	void publishEvent(const std::string& name, const std::string& version, const JsonBox::Object& data);

	JsonBox::Object version(JsonBox::Object command);
	JsonBox::Object config(JsonBox::Object command);
	void setAppLogicHandler(JsonBox::Object (*wAppLogicHandler)(JsonBox::Object&));
	void setAppConfigChangeHandler(bool (*wAppConfigChangeHandler)(std::string&, std::string&, std::string&));
	int addDirtyConfigurationKey(std::string&, std::string&, std::string&);
	JsonBox::Object getDirtyConfigurationKeys();
	std::map<std::string, std::string> getDirtyConfigurationKeysMap() { return mDirtyConfigurationKeys; }
	int getDirtyConfigurationKeyCount() { return mDirtyConfigurationKeys.size(); }
};

#endif

