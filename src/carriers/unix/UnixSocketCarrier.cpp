/*
 * Copyright (C) 2006-2020 Istituto Italiano di Tecnologia (IIT)
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms of the
 * BSD-3-Clause license. See the accompanying LICENSE file for details.
 */

#include "UnixSocketCarrier.h"
#include <yarp/conf/filesystem.h>

#include <yarp/os/ConnectionState.h>
#include <yarp/os/Log.h>
#include <yarp/os/LogStream.h>
#include <yarp/os/Os.h>

#include "UnixSocketLogComponent.h"

using namespace yarp::os;
namespace fs = yarp::conf::filesystem;

namespace {

// FIXME: This method should be available somewhere in YARP
std::string getYARPRuntimeDir()
{
    static std::mutex m;
    std::lock_guard<std::mutex> lock(m);

    static std::string yarp_runtime_dir;
    bool found = false;

    // If already populated, there is nothing to do
    if (!yarp_runtime_dir.empty()) {
        return yarp_runtime_dir;
    }

    // Check YARP_RUNTIME_DIR
    yarp_runtime_dir = NetworkBase::getEnvironment("YARP_RUNTIME_DIR", &found);
    if (found) {
        return yarp_runtime_dir;
    }

    // Check XDG_RUNTIME_DIR
    std::string xdg_runtime_dir = NetworkBase::getEnvironment("XDG_RUNTIME_DIR", &found);
    if (found) {
        yarp_runtime_dir = xdg_runtime_dir + fs::preferred_separator + "yarp";
        return yarp_runtime_dir;
    }

    // Use /tmp/runtime-user
    std::string user = NetworkBase::getEnvironment("USER", &found);
    if (found) {
        yarp_runtime_dir = "/tmp/runtime-" + user + fs::preferred_separator + "yarp";
        return yarp_runtime_dir;
    }

    // ERROR
    return {};
}

/**
 * @brief isUnixSockSupported
 * @param proto, contains the information of the connection
 * @return true if the remote and the local port are on the same host
 */
bool isUnixSockSupported(ConnectionState& proto) // FIXME Why is this method unused?
{
    yarp::os::Contact remote = proto.getStreams().getRemoteAddress();
    yarp::os::Contact local = proto.getStreams().getLocalAddress();

    if (remote.getHost() != local.getHost()) {
        yCError(UNIXSOCK_CARRIER,
                "The ports are on different machines, unix socket not supported...");
        return false;
    }
    return true;
}

} // namespace

UnixSocketCarrier::UnixSocketCarrier(bool requireAckFlag) :
        requireAckFlag(requireAckFlag)
{
}

yarp::os::Carrier* UnixSocketCarrier::create() const
{
    return new UnixSocketCarrier(requireAckFlag);
}

std::string UnixSocketCarrier::getName() const
{
    return requireAckFlag ? name_ack : name;
}

bool UnixSocketCarrier::requireAck() const
{
    return requireAckFlag;
}

bool UnixSocketCarrier::isConnectionless() const
{
    return false;
}

bool UnixSocketCarrier::checkHeader(const Bytes& header)
{
    if (header.length() != headerSize) {
        return false;
    }
    const char* target = requireAckFlag ? headerCode_ack : headerCode;
    for (size_t i = 0; i < headerSize; i++) {
        if (header.get()[i] != target[i]) {
            return false;
        }
    }
    return true;
}

void UnixSocketCarrier::getHeader(Bytes& header) const
{
    const char* target = requireAckFlag ? headerCode_ack : headerCode;
    for (size_t i = 0; i < headerSize && i < header.length(); i++) {
        header.get()[i] = target[i];
    }
}

bool UnixSocketCarrier::sendIndex(ConnectionState& proto, SizedWriter& writer)
{
    YARP_UNUSED(proto);
    YARP_UNUSED(writer);

    return true;
}

bool UnixSocketCarrier::expectIndex(ConnectionState& proto)
{
    YARP_UNUSED(proto);

    return true;
}

bool UnixSocketCarrier::sendAck(ConnectionState& proto)
{
    yCTrace(UNIXSOCK_CARRIER);
    if (requireAckFlag) {
        const Bytes ack_bytes(const_cast<char*>(ack_string), ack_string_size);
        proto.os().write(ack_bytes);
    }
    return true;
}

bool UnixSocketCarrier::expectAck(ConnectionState& proto)
{
    yCTrace(UNIXSOCK_CARRIER);
    if (requireAckFlag) {
        std::array<char, ack_string_size> buf;
        Bytes ack(buf.data(), buf.size());
        yarp::conf::ssize_t hdr = proto.is().readFull(ack);
        if (static_cast<size_t>(hdr) != ack.length()) {
            yCDebug(UNIXSOCK_CARRIER, "Did not get ack");
            return false;
        }

        const char* target = ack_string;
        for (size_t i = 0; i < ack_string_size; i++) {
            if (ack.get()[i] != target[i]) {
                yCDebug(UNIXSOCK_CARRIER, "Bad ack");
                return false;
            }
        }
    }
    return true;
}

bool UnixSocketCarrier::respondToHeader(ConnectionState& proto)
{
    // I am the receiver
    return becomeUnixSocket(proto, false);
}

bool UnixSocketCarrier::expectReplyToHeader(ConnectionState& proto)
{
    // I am the sender
    return becomeUnixSocket(proto, true);
}

bool UnixSocketCarrier::becomeUnixSocket(ConnectionState& proto, bool sender)
{
    if (!isUnixSockSupported(proto)) {
        return false;
    }

    Contact remote = proto.getStreams().getRemoteAddress();
    Contact local = proto.getStreams().getLocalAddress();

    proto.takeStreams(nullptr); // free up port from tcp

    std::string runtime_dir = getYARPRuntimeDir();

    // Make sure that the path exists
    if (runtime_dir.empty() || yarp::os::mkdir_p(runtime_dir.c_str(), 0) != 0) {
        yCError(UNIXSOCK_CARRIER, "Failed to create directory %s", runtime_dir.c_str());
        return false;
    }

    if (sender) {
        socketPath = runtime_dir + fs::preferred_separator + std::to_string(remote.getPort()) + "_" + std::to_string(local.getPort()) + ".sock";
    } else {
        socketPath = runtime_dir + fs::preferred_separator + std::to_string(local.getPort()) + "_" + std::to_string(remote.getPort()) + ".sock";
    }

    stream = new UnixSockTwoWayStream(socketPath);
    stream->setLocalAddress(local);
    stream->setRemoteAddress(remote);

    if (!stream->open(sender)) {
        delete stream;
        stream = nullptr;
        yCError(UNIXSOCK_CARRIER, "Failed to open stream on socket %s as %s", socketPath.c_str(), (sender ? "sender" : "receiver"));
        return false;
    }
    yAssert(stream != nullptr);

    proto.takeStreams(stream);

    yCDebug(UNIXSOCK_CARRIER, "Connected on socket %s as %s", socketPath.c_str(), (sender ? "sender" : "receiver"));
    return true;
}
