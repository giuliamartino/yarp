/*
 * Copyright (C) 2006-2020 Istituto Italiano di Tecnologia (IIT)
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms of the
 * BSD-3-Clause license. See the accompanying LICENSE file for details.
 */

#ifndef YARP_OS_IMPL_LOGCOMPONENT_H
#define YARP_OS_IMPL_LOGCOMPONENT_H

#include <yarp/os/api.h>
#include <yarp/os/LogComponent.h>

namespace yarp {
namespace serversql {
namespace impl {
namespace LogComponent {

void print_callback(yarp::os::Log::LogType type,
                    const char* msg,
                    const char* file,
                    const unsigned int line,
                    const char* func,
                    double systemtime,
                    double networktime,
                    const char* comp_name);

void setMinumumLogType(yarp::os::Log::LogType minumumLogType);

} // namespace LogComponent
} // namespace impl
} // namespace serversql
} // namespace yarp

#define YARP_SERVERSQL_LOG_COMPONENT(name, name_string) \
    const yarp::os::LogComponent& name() \
    { \
        static const yarp::os::LogComponent component(name_string, \
                                                      yarp::os::Log::TraceType, \
                                                      yarp::os::Log::DebugType, \
                                                      yarp::serversql::impl::LogComponent::print_callback, \
                                                      yarp::os::Log::defaultForwardCallback()); \
        return component; \
    }

#endif // YARP_OS_IMPL_LOGCOMPONENT_H
