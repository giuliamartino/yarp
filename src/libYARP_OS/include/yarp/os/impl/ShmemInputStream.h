/*
 * Copyright (C) 2006-2018 Istituto Italiano di Tecnologia (IIT)
 * Copyright (C) 2006-2010 RobotCub Consortium
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms of the
 * BSD-3-Clause license. See the accompanying LICENSE file for details.
 */

#ifndef YARP_OS_IMPL_SHMEMINPUTSTREAM_H
#define YARP_OS_IMPL_SHMEMINPUTSTREAM_H

#include <ace/config.h>
#include <ace/Mutex.h>
#include <ace/Process_Mutex.h>
#include <ace/SOCK_Stream.h>
#include <ace/SOCK_Acceptor.h>
#include <ace/SOCK_Connector.h>
#include <ace/Log_Msg.h>
#if defined(ACE_LACKS_SYSV_SHMEM)
#include <ace/Shared_Memory_MM.h>
#else
#include <ace/Shared_Memory_SV.h>
#endif

#include <yarp/os/Semaphore.h>
#include <yarp/os/Thread.h>
#include <yarp/os/Time.h>
#include <yarp/os/impl/Logger.h>

#include <yarp/os/InputStream.h>
#include <yarp/os/impl/ShmemTypes.h>
#include <yarp/os/impl/PlatformSize.h>

namespace yarp {
    namespace os {
        namespace impl {
            class ShmemInputStreamImpl;
        }
    }
}

class yarp::os::impl::ShmemInputStreamImpl {
public:
    ShmemInputStreamImpl()
    {
        m_bOpen=false;

        m_pAccessMutex=m_pWaitDataMutex=nullptr;
        m_pMap=nullptr;
        m_pData=nullptr;
        m_pHeader=nullptr;
        m_ResizeNum=0;
        m_Port = -1;
        m_pSock = nullptr;
    }

    ~ShmemInputStreamImpl()
    {
        close();
    }

    bool isOk() { return m_bOpen; }
    bool open(int port, ACE_SOCK_Stream *pSock, int size=SHMEM_DEFAULT_SIZE);
    YARP_SSIZE_T read(const Bytes& b);
    void close();

protected:
    int read(char *data, int len);
    bool Resize();
    bool m_bOpen;

    int m_ResizeNum;
    int m_Port;

#if defined(_ACE_USE_SV_SEM)
    ACE_Mutex *m_pAccessMutex, *m_pWaitDataMutex;
#else
    ACE_Process_Mutex *m_pAccessMutex, *m_pWaitDataMutex;
#endif

    yarp::os::Semaphore m_ReadSerializerMutex;

    ACE_Shared_Memory *m_pMap;
    char *m_pData;
    ShmemHeader_t *m_pHeader;

    ACE_SOCK_Stream *m_pSock;
};

#endif
