/*
 * Copyright (C) 2006-2021 Istituto Italiano di Tecnologia (IIT)
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms of the
 * BSD-3-Clause license. See the accompanying LICENSE file for details.
 */

#include <yarp/companion/impl/Companion.h>

#include <stdio.h>
#include <fstream>
#include <string>
#include <array>
#include <vector>
#include <yarp/os/Time.h>
#include <yarp/os/all.h>

using namespace yarp::os;
using namespace std;
using yarp::companion::impl::Companion;

//------------------------------------------------------------------------------------------------------------------------------

enum client_return_code_t { CLIENT_END_TEST = 0};
enum server_return_code_t { SERVER_END_TEST = 0, SERVER_QUIT = 1,SERVER_ERROR =2 };

server_return_code_t server(double server_wait, bool verbose = false);
client_return_code_t client(int nframes, int payload_size, double pause, string logfilename = "log_", bool verbose = false);

//------------------------------------------------------------------------------------------------------------------------------

int Companion::cmdLatencyTest(int argc, char* argv[])
{
    Property p;
    p.fromCommand(argc, argv, false);

    if (p.check("help") || argc==0)
    {
        yCInfo(COMPANION, "This is yarp latency-test");
        yCInfo(COMPANION, "Syntax for the server:");
        yCInfo(COMPANION, "yarp latency-test --server [--details]");
        yCInfo(COMPANION, "--server_wait <time>");
        yCInfo(COMPANION, " ");
        yCInfo(COMPANION, "Syntax for the client:");
        yCInfo(COMPANION, "yarp latency-test --client [--details]");
        yCInfo(COMPANION, "--nframes <X> --payload_size <Y> --client_wait <Z> [--logfile <filename_prefix>]");
        yCInfo(COMPANION, "--nframes <X> --multitest --client_wait <Z> [--logfile <filename_prefix>]");
        yCInfo(COMPANION, "Default value for filename_prefix: log_");
        return -1;
    }

    bool verbose = false;
    if (p.check("details")) verbose=true;

    if (p.check("server"))
    {
        double server_wait = p.find("server_wait").asDouble();
        size_t servercounter=0;
        while(1)
        {
            int returncode = server(server_wait, verbose);
            if      (returncode == SERVER_END_TEST) { yCInfo(COMPANION, "Test %d complete", servercounter++);}
            else if (returncode == SERVER_QUIT)     { yCInfo(COMPANION, "Test %d complete, quitting"); break;}
            else if (returncode == SERVER_ERROR)    { yCError(COMPANION, "Test %d error"); }
        }
    }
    else if (p.check("client"))
    {
        if (p.check("nframes") == false)
        {  yCError(COMPANION) << "Missing mandatory parameter nframes. See available options with yarp latency-test";  return -1; }
        int frames = p.find("nframes").asInt();

        double client_wait = 0;
        if (p.check("client_wait")) client_wait = p.find("client_wait").asFloat64();

        string logfilename = "log_";
        if (p.check("logfile")) logfilename = p.find("logfile").asString();

        if (p.check("payload_size") && !p.check ("multitest"))
        {
            int payload = p.find("payload_size").asInt();
            return client(frames, payload, client_wait, logfilename, verbose);
        }
        else if (!p.check("payload_size") && p.check("multitest"))
        {
            std::array<int,7> psizes {1e3,1e4,1e5,1e6,1e7,1e8,1e9};
            for (size_t i = 0; i < psizes.size(); i++)
            {
               client(frames, psizes[i], client_wait, logfilename, verbose);
            }
            return 0;
        }
        else
        {
            yCError(COMPANION) << "Syntax error. Choose either payload_size or multitest. See available options with yarp latency-test";  return -1;
        }

    }
    else
    {
        yCError(COMPANION) << "Missing option. Use --help";
        return -1;
    }
}

//------------------------------------------------------------------------------------------------------------------------------

server_return_code_t server(double server_wait, bool verbose)
{
    //Open the port for connection with the client
    Port port;
    port.open("/latencyTest/server");
    yCInfo(COMPANION,"I am the server, now listening to the client for a `start` command\n");

    //Wait a start command from the client
    Bottle startbot;
    port.read(startbot);
    if (startbot.get(0).asString() != "start")
    {
        yCError(COMPANION) << "Invalid command received from the client";
        return SERVER_ERROR;
    }

    //Creates a payload bottle, consisting of a string with the size requested by the client
    int payload_reqsize = startbot.get(1).asInt();
    char* buf = new char[payload_reqsize];
    for (size_t elem = 0; elem < payload_reqsize; elem++)
    {
        buf[elem] = 112;
    }
    Bottle payloadbottle;
    payloadbottle.addString(buf);
    yCInfo(COMPANION,"Generated a string of %d bytes, as requested by the client (%d)", payloadbottle.get(0).asString().size(), payload_reqsize);

    size_t serverframecounter=0;
    while(true)
    {
        //Reads the frame sent by the client and checks if command `stop` or `quit` is received. Otherwise:
        //Adds the payload, the time required to add the payload, the frame number. Finally, it sends it back to client.
        Bottle b;
        port.read(b);
        if      (b.get(0).asString() == "stop") break;
        else if (b.get(0).asString() == "quit") return SERVER_QUIT;
        double tt1 = yarp::os::Time::now();
        b.append(payloadbottle);
        double tt2 = yarp::os::Time::now();
        b.addDouble(tt2-tt1);
        b.addInt(serverframecounter);
        port.write(b);

        //verbose prints
        if (verbose)
        {
            yCInfo(COMPANION, "This time was required to append the payload: %f\n", tt2 - tt1);
            yCInfo(COMPANION, "Sending the frame number:%d\n", serverframecounter);
        }

        //Give the CPU some idle time
        if (server_wait > 0) Time::delay(server_wait);
        serverframecounter++;
    }

    //The test is complete
    port.close();
    return SERVER_END_TEST;
}

//------------------------------------------------------------------------------------------------------------------------------

client_return_code_t client(int nframes, int payload_size, double client_wait, string logfilename, bool verbose)
{
    //the structure where to save the data
    struct stats
    {
        double latency=0;
        double copytime=0;
    };
    std::vector<stats> test_data;
    test_data.resize(nframes);

    //opens a local port and connects bidirectionally with the server
    Port port;
    port.open("/latencyTest/client");
    while(!Network::connect("/latencyTest/server","/latencyTest/client"))
    {
        yCInfo(COMPANION, "Waiting for connection..\n");
        Time::delay(0.5);
    }
    while (!Network::connect("/latencyTest/client", "/latencyTest/server"))
    {
        yCInfo(COMPANION, "Waiting for connection..\n");
        Time::delay(0.5);
    }

    //Send to the server a command 'start', followed by the requested size of the payload (in bytes)"
    Bottle startbot;
    startbot.addString("start");
    startbot.addDouble(payload_size);
    port.write(startbot);

    //Performs the test, by sending request to the server. The duration of the test depends on the number of requested frames.
    int clientframecounter = 0;
    double latency_mean_accumulator = 0;
    while(clientframecounter <nframes)
    {
        //sends the frame to server. The frame is composed by
        //0 clientframecounter a counter id
        //1 clientframetime the current time
        Bottle datum;
        double clientframetime = Time::now();
        datum.addInt(clientframecounter);
        datum.addDouble(clientframetime);
        port.write(datum);

        //receives back from the server the frame just sent, with the additional stuff appended by the server.
        //So the final content is:
        //0 the clientframecounter
        //1 the clientframetime
        //2 a blob of data (string)
        //3 the copytime computed by the server
        //4 the serverframecounter
        port.read(datum);
        int recT =datum.get(0).asInt();
        double time = datum.get(1).asDouble();
        std::string recstringpayload = datum.get(2).asString();
        double copytime = datum.get(3).asDouble();
        double finaltime=Time::now();
        double latency_ms = (finaltime - time) * 1000;
        latency_mean_accumulator += latency_ms;
        test_data[clientframecounter].latency = latency_ms;
        test_data[clientframecounter].copytime = copytime;

        if (verbose)
        {
            //These prints are ok for debug, but they will slow down the tests.
            yCInfo(COMPANION, "Received a payload of %d bytes", recstringpayload.size());
            yCInfo(COMPANION, "latency for frame %d is: %lf ms\n", clientframecounter, (finaltime -time)*1000);
        }

        //Give the CPU some idle time
        if (client_wait >0) {yarp::os::Time::delay(client_wait);}

        clientframecounter++;
    }

    //Send to the server a command 'stop'. The test is complete.
    //The server will now restart, waiting for a new client connection.
    Bottle stopbot;
    stopbot.addString("stop");
    port.write(stopbot);

    //close the port
    port.close();

    //prints stats to screen
    double latency_mean = latency_mean_accumulator / clientframecounter;
    yCInfo(COMPANION, "Processed %d frames of %d bytes, average latency %.3lf[ms]\n", clientframecounter, payload_size, latency_mean);

    //save the stats to a logfile
    std::fstream fs;
    std::string filename = logfilename;
    filename += std::to_string(payload_size);
    filename += ".txt";
    fs.open(filename, std::fstream::out );
    for (int i = 0; i < nframes; i++)
    {
        fs << test_data[i].latency << " " << test_data[i].copytime << std::endl;
    }
    fs.close();
    yCInfo(COMPANION, "Test complete. Data saved to file: %s", filename.c_str());

    return CLIENT_END_TEST;
}
