/******************************************************************************
 * THE OMEGA LIB PROJECT
 *-----------------------------------------------------------------------------
 * Copyright 2010-2015		Electronic Visualization Laboratory, 
 *							University of Illinois at Chicago
 * Authors:										
 *  Daniele Donghi			d.donghi@gmail.com
 *  Alessandro Febretti		febret@gmail.com
 *-----------------------------------------------------------------------------
 * Copyright (c) 2010-2015, Electronic Visualization Laboratory,  
 * University of Illinois at Chicago
 * All rights reserved.
 * Redistribution and use in source and binary forms, with or without modification, 
 * are permitted provided that the following conditions are met:
 * 
 * Redistributions of source code must retain the above copyright notice, this 
 * list of conditions and the following disclaimer. Redistributions in binary 
 * form must reproduce the above copyright notice, this list of conditions and 
 * the following disclaimer in the documentation and/or other materials provided 
 * with the distribution. 
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" 
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO THE 
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE 
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE 
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL 
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE  GOODS OR 
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER 
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, 
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE 
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 ******************************************************************************/
#include "PortholeService.h"
#include "ServerThread.h"
#include <modulesConfig.h>

using namespace omega;
using namespace omicron;

///////////////////////////////////////////////////////////////////////////////
PortholeService::PortholeService():
myPointerBounds(Vector2i::Zero()), myPointerSpeed(1), myBinder(NULL),
myDynamicStreamQualityEnabled(false)
{
    //myStreamEncoderType = "png";
	myStreamEncoderType = "ffenc";
}

///////////////////////////////////////////////////////////////////////////////
PortholeService::~PortholeService()
{
    portholeServer->stop();
    delete portholeServer;
}

///////////////////////////////////////////////////////////////////////////////
void PortholeService::start(int port, const String& defaultPage)
{
    // TODO read config/porthole/hardwareEncoderEnabled from system config to

    myBinder = new PortholeFunctionsBinder();
    myBinder->clear();

    portholeServer = new ServerThread(this, defaultPage);
    portholeServer->setPort(port);
    portholeServer->start();
}

///////////////////////////////////////////////////////////////////////////////
void PortholeService::clearCache()
{
    myBinder->clear();
    portholeServer->clearCache();
}

///////////////////////////////////////////////////////////////////////////////
void PortholeService::notifyServerStarted()
{
    if(!myServerStartedCommand.empty())
    {
        PythonInterpreter* i = SystemManager::instance()->getScriptInterpreter();
        i->queueCommand(myServerStartedCommand);
    }
}

///////////////////////////////////////////////////////////////////////////////
void PortholeService::notifyConnected(const String& id)
{
    if(!myConnectedCommand.empty())
    {
        PythonInterpreter* i = SystemManager::instance()->getScriptInterpreter();
        String cmd = StringUtils::replaceAll(myConnectedCommand, "%id%", id);
        i->queueCommand(cmd);
    }
}

///////////////////////////////////////////////////////////////////////////////
void PortholeService::notifyDisconnected(const String& id)
{
    if(!myDisconnectedCommand.empty())
    {
        PythonInterpreter* i = SystemManager::instance()->getScriptInterpreter();
        String cmd = StringUtils::replaceAll(myDisconnectedCommand, "%id%", id);
        i->queueCommand(cmd);
    }
}

///////////////////////////////////////////////////////////////////////////////
void PortholeService::postPointerEvent(Event::Type type, int sourceId, float x, float y, uint flags, unsigned int userId)
{
    lockEvents();
    Event* evt = writeHead();
    evt->reset(type, Service::Pointer, sourceId, getServiceId(), userId);
    evt->setPosition(x, y);
    evt->setFlags(flags);
    unlockEvents();
}

///////////////////////////////////////////////////////////////////////////////
void PortholeService::postKeyEvent(Event::Type type, char key, uint flags, unsigned int userId)
{
    lockEvents();
    Event* evt = writeHead();
    evt->reset(type, Service::Keyboard, key, getServiceId(), userId);
    evt->setFlags(flags);
    unlockEvents();
}

///////////////////////////////////////////////////////////////////////////////
PortholeClient* PortholeService::createClient(const String& name, libwebsocket* wsi)
{
    PortholeClient* cli = new PortholeClient(this, name, wsi);
    myClients.push_back(cli);
    return cli;
}

///////////////////////////////////////////////////////////////////////////////
void PortholeService::destroyClient(PortholeClient* gui)
{
    oassert(gui != NULL);
    myClients.remove(gui);
}

///////////////////////////////////////////////////////////////////////////////
PortholeClient* PortholeService::findClient(const String& name)
{
    foreach(PortholeClient* cli, myClients)
    {
        if(cli->getId() == name) return cli;
    }
    return NULL;
}

///////////////////////////////////////////////////////////////////////////////
void PortholeService::sendjs(const String& js, const String& destination)
{
    foreach(PortholeClient* cli, myClients)
    {
        if(cli->getId() == destination)
        {
            cli->calljs(js);
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
void PortholeService::broadcastjs(const String& js, const String& origin)
{
    if(origin.empty())
    {
        foreach(PortholeClient* cli, myClients)
        {
            cli->calljs(js);
        }
    }
    else
    {
        foreach(PortholeClient* cli, myClients)
        {
            if(cli->getId() != origin) cli->calljs(js);
        }
    }
}
