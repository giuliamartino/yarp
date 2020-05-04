/*
 * Copyright (C) 2006-2020 Istituto Italiano di Tecnologia (IIT)
 * Copyright (C) 2006-2010 RobotCub Consortium
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms of the
 * BSD-3-Clause license. See the accompanying LICENSE file for details.
 */

#include <yarp/serversql/impl/ConnectThread.h>
#include <yarp/serversql/impl/LogComponent.h>

#include <yarp/os/Network.h>

using yarp::serversql::impl::ConnectThread;

namespace {
YARP_SERVERSQL_LOG_COMPONENT(CONNECTTHREAD, "yarp.serversql.impl.ConnectThread")
} // namespace


ConnectThread::ConnectThread(std::mutex& mutex) : mutex(mutex)
{
}

void ConnectThread::run()
{
    do {
        mutex.lock();
        if (ct==0) {
            needed = false;
        }
        ct--;
        mutex.unlock();

        yCTrace(CONNECTTHREAD,
                " ]]] con %s %s / %d %d",
                src.c_str(),
                dest.c_str(),
                ct,
                needed);
        if (!needed) { break; }
        if (positive) {
            if (!yarp::os::NetworkBase::isConnected(src,dest)) {
                yCTrace(CONNECTTHREAD,
                        "   (((Trying to connect %s and %s)))",
                        src.c_str(),
                        dest.c_str());
                yarp::os::NetworkBase::connect(src,dest);
            }
        } else {
            if (yarp::os::NetworkBase::isConnected(src,dest)) {
                yCTrace(CONNECTTHREAD,
                        "   (((Trying to disconnect %s and %s)))\n",
                        src.c_str(),
                        dest.c_str());
                yarp::os::NetworkBase::disconnect(src,dest);
            }
        }
    } while (true);
}
