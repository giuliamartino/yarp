/*
 * Copyright (C) 2006-2020 Istituto Italiano di Tecnologia (IIT)
 * Copyright (C) 2006-2010 RobotCub Consortium
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms of the
 * BSD-3-Clause license. See the accompanying LICENSE file for details.
 */

#include <yarp/serversql/yarpserversql.h>

#include <yarp/os/Network.h>
#include <yarp/serversql/Server.h>
#include <yarp/serversql/impl/NameServerContainer.h>
#include <yarp/serversql/impl/LogComponent.h>

using yarp::os::Network;
using yarp::serversql::impl::NameServerContainer;

yarp::os::NameStore *yarpserver_create(yarp::os::Searchable& options)
{
    auto* nc = new NameServerContainer;
    if (!nc) {
        return nullptr;
    }
    yarp::serversql::impl::LogComponent::setMinumumLogType(yarp::os::Log::WarningType);
    if (!nc->open(options)) {
        delete nc;
        return nullptr;
    }
    nc->goPublic();
    return nc;
}

int yarpserver_main(int argc, char *argv[])
{
    Network yarp(yarp::os::YARP_CLOCK_SYSTEM);
    yarp::serversql::Server yServer;
    return yServer.run(argc, argv);
}
