/*
 * Copyright (C) 2006-2020 Istituto Italiano di Tecnologia (IIT)
 * Copyright (C) 2006-2010 RobotCub Consortium
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms of the
 * BSD-3-Clause license. See the accompanying LICENSE file for details.
 */

#include <yarp/os/Network.h>
#include <yarp/os/Property.h>

#include <yarp/sig/Image.h>

#include <yarp/dev/Drivers.h>
#include <yarp/dev/FrameGrabberInterfaces.h>
#include <yarp/dev/PolyDriver.h>

#include <cstdio>

using yarp::dev::IFrameGrabberImage;
using yarp::dev::PolyDriver;
using yarp::os::Network;
using yarp::os::Property;
using yarp::sig::ImageOf;
using yarp::sig::PixelRgb;

/*
 * Read an image from a remote source using the "device" view of
 * camera-like sources.
 *
 * Remote source could be, for example:
 *   yarpdev --device fakeFrameGrabber --name /fakey
 */
int main(int argc, char* argv[])
{
    YARP_UNUSED(argc);
    YARP_UNUSED(argv);

    Network yarp;

    Property config;
    config.put("device", "remote_grabber"); // device type
    config.put("local", "/client");         // name of local port to use
    config.put("remote", "/fakey");         // name of remote port to connect to

    PolyDriver dd(config);
    if (!dd.isValid()) {
        printf("Failed to create and configure device\n");
        return 1;
    }
    IFrameGrabberImage* grabberInterface;
    if (!dd.view(grabberInterface)) {
        printf("Failed to view device through IFrameGrabberImage interface\n");
        return 1;
    }

    ImageOf<PixelRgb> img;
    grabberInterface->getImage(img);
    printf("Got a %zux%zu image\n", img.width(), img.height());

    dd.close();

    return 0;
}
