#include "ffmpegPortmonitor.h"
#include "cl_params.h"

#include <yarp/os/LogComponent.h>
#include <yarp/sig/all.h>

#include <cstring>
#include <cmath>
#include <algorithm>
#include <iostream>

extern "C" {
    #include <libavcodec/codec.h>
    #include <libavcodec/avcodec.h>
    #include <libavutil/opt.h>
    #include <libavutil/imgutils.h>
    #include <libavformat/avformat.h>
    #include <libswscale/swscale.h>
}

using namespace yarp::os;
using namespace yarp::sig;
using namespace std;

namespace {
YARP_LOG_COMPONENT(FFMPEGMONITOR,
                   "yarp.carrier.portmonitor.ffmpeg",
                   yarp::os::Log::minimumPrintLevel(),
                   yarp::os::Log::LogTypeReserved,
                   yarp::os::Log::printCallback(),
                   nullptr)
}


void split(const std::string &s, char delim, vector<string> &elements) {
    std::istringstream iss(s);
    std::string item;
    while (std::getline(iss, item, delim)) {
        elements.push_back(item);
    }
}

bool FfmpegMonitorObject::create(const yarp::os::Property& options)
{   
    // Check if this is sender or not
    senderSide = (options.find("sender_side").asBool());

    // Set default codec
    AVCodecID codecId = AV_CODEC_ID_MPEG2VIDEO;
    codecName = "mpeg2video";

    // Parse command line parameters and set them into global variable "paramsMap"
    std::string str = options.find("carrier").asString();
    if (getParamsFromCommandLine(str, codecId) == -1)
        return false;
    
    // Find encoder/decoder
    if (senderSide) {
        codec = avcodec_find_encoder(codecId);
    } else {
        codec = avcodec_find_decoder(codecId);
    }
    if (!codec) {
        yCError(FFMPEGMONITOR, "Can't find codec %s", codecName);
        return false;
    }

    // Prepare codec context
    if (codecContext == NULL) {
        codecContext = avcodec_alloc_context3(codec);
    } else {
        yCError(FFMPEGMONITOR, "Codec context is already allocated");
        return false;
    }
    if (!codecContext) {
        yCError(FFMPEGMONITOR, "Could not allocate video codec context");
        return false;
    }

    firstTime = true;

    // Set command line params
    if (setCommandLineParams() == -1)
        return false;
    

    // Pixel formats map
    pixelMap[VOCAB_PIXEL_RGB] = AV_PIX_FMT_RGB24;
    pixelMap[VOCAB_PIXEL_RGBA] = AV_PIX_FMT_RGBA;
    pixelMap[VOCAB_PIXEL_BGR] = AV_PIX_FMT_BGR24;
    pixelMap[VOCAB_PIXEL_BGRA] = AV_PIX_FMT_BGRA;
    pixelMap[VOCAB_PIXEL_YUV_420] = AV_PIX_FMT_YUV420P;

    codecPixelMap[AV_CODEC_ID_H264] = AV_PIX_FMT_YUV420P;
    codecPixelMap[AV_CODEC_ID_H265] = AV_PIX_FMT_YUV420P;
    codecPixelMap[AV_CODEC_ID_MPEG2VIDEO] = AV_PIX_FMT_YUV420P;

    return true;
}

void FfmpegMonitorObject::destroy(void)
{
    paramsMap.clear();

    // Check if codec context is freeable, if yes free it.
    if (codecContext != NULL) {
        avcodec_close(codecContext);
        avcodec_free_context(&codecContext);
        codecContext = NULL;
    }

}

bool FfmpegMonitorObject::setparam(const yarp::os::Property& params)
{
    yCTrace(FFMPEGMONITOR, "setparam");
    return false;
}

bool FfmpegMonitorObject::getparam(yarp::os::Property& params)
{
    yCTrace(FFMPEGMONITOR, "getparam");
    return false;
}

bool FfmpegMonitorObject::accept(yarp::os::Things& thing)
{
    if (senderSide) {
        yCTrace(FFMPEGMONITOR, "accept - sender");
        Image* img = thing.cast_as< Image >();
        if(img == nullptr) {
            yCError(FFMPEGMONITOR, "Expected type Image in sender side, but got wrong data type!");
            return false;
        }
    }
    else {
        yCTrace(FFMPEGMONITOR, "accept - receiver");
        Bottle* bt = thing.cast_as<Bottle>();
        if(bt == nullptr){
            yCError(FFMPEGMONITOR, "Expected type Bottle in receiver side, but got wrong data type!");
            return false;
        }
    }
    return true;
}

yarp::os::Things& FfmpegMonitorObject::update(yarp::os::Things& thing)
{
    if (senderSide) {
        bool success = true;
        yCTrace(FFMPEGMONITOR, "update - sender");
        Image* img = thing.cast_as< Image >();
        AVPacket *packet = av_packet_alloc();
        if (packet == NULL) {
            yCError(FFMPEGMONITOR, "Error in packet allocation");
            success = false;
        }
        
        if (success && compress(img, packet) != 0) {
            yCError(FFMPEGMONITOR, "Error in compression");
            success = false;
        }

        // Insert compressed image into a Bottle to be sent
        data.clear();

        int successCode = success ? 1 : 0;
        data.addInt32(successCode);
        data.addInt32(img->width());
        data.addInt32(img->height());
        data.addInt32(img->getPixelCode());
        data.addInt32(img->getPixelSize());

        if (success) {              
            Value p(packet, sizeof(*packet));
            data.add(p);
            Value d(packet->data, packet->size);
            data.add(d);
            data.addInt32(packet->buf->size);
            Value bd(packet->buf->data, packet->buf->size);
            data.add(bd);

            for (int i = 0; i < packet->side_data_elems; i++) {
                data.addInt32(packet->side_data[i].size);
                data.addInt32(packet->side_data[i].type);
                Value sd(packet->side_data[i].data, packet->side_data[i].size);
                data.add(sd);
            }
        }
        th.setPortWriter(&data);
        av_free_packet(packet);
    }
    else {
        yCTrace(FFMPEGMONITOR, "update - receiver");
        Bottle* compressedBottle = thing.cast_as<Bottle>();
        imageOut.zero();
        int width = compressedBottle->get(1).asInt32();
        int height = compressedBottle->get(2).asInt32();
        int pixelCode = compressedBottle->get(3).asInt32();
        int pixelSize = compressedBottle->get(4).asInt32();
        imageOut.setPixelCode(pixelCode);
        imageOut.setPixelSize(pixelSize);
        imageOut.resize(width, height);

        if (compressedBottle->get(0).asInt32() == 1) {
            bool success = true;
            // Get compressed image from Bottle            
            AVPacket* tmp = (AVPacket*) compressedBottle->get(5).asBlob();
            AVPacket* packet = av_packet_alloc();
            packet->dts = tmp->dts;
            packet->duration = tmp->duration;
            packet->flags = tmp->flags;
            packet->pos = tmp->pos;
            packet->pts = tmp->pts;
            packet->side_data_elems = 0;
            packet->stream_index = tmp->stream_index;
            packet->size = tmp->size;
            packet->data = (uint8_t *) compressedBottle->get(6).asBlob();
            packet->buf = av_buffer_create((uint8_t *) compressedBottle->get(8).asBlob(),
                                            compressedBottle->get(7).asInt32(), av_buffer_default_free,
                                            nullptr, AV_BUFFER_FLAG_READONLY);
            
            // Packet side data
            for (int i = 0; i < tmp->side_data_elems; i++) {

                int ret = av_packet_add_side_data(packet,
                                                    (AVPacketSideDataType) compressedBottle->get(10).asInt32(),     // Type
                                                    (uint8_t *) compressedBottle->get(11).asBlob(),                 // Data
                                                    compressedBottle->get(9).asInt32());                            // Size
                if (ret < 0) {
                    success = false;
                    break;
                }
            }
                        
            if (success && decompress(packet, width, height, pixelCode, pixelSize) != 0) {
                yCError(FFMPEGMONITOR, "Error in decompression");
                success = false;
            }
            
            av_freep(&packet->side_data);
            av_freep(&packet);

        }
        th.setPortWriter(&imageOut);
        
    }
    return th;
}

int FfmpegMonitorObject::compress(Image* img, AVPacket *pkt) {
    
    yCTrace(FFMPEGMONITOR, "compress");
    AVFrame *startFrame;
    AVFrame *endFrame;

    int w = img->width();
    int h = img->height();

    // Allocate video frame for original frames
    startFrame = av_frame_alloc();
    if (startFrame == NULL) {
        yCError(FFMPEGMONITOR, "Cannot allocate starting frame!");
        return -1;
    }

    // Allocate an video frame for end frame
    endFrame = av_frame_alloc();
    if (endFrame == NULL) {
        yCError(FFMPEGMONITOR, "Cannot allocate end frame!");
        av_frame_free(&startFrame);
        return -1;
    }

    int success = av_image_alloc(startFrame->data, startFrame->linesize,
                    w, h,
                    (AVPixelFormat) pixelMap[img->getPixelCode()], 16);

    if (success < 0) {
        yCError(FFMPEGMONITOR, "Cannot allocate starting frame buffer!");
        av_frame_free(&startFrame);
        av_frame_free(&endFrame);
        return -1;
    }

    startFrame->linesize[0] = img->getRowSize();
    // Free old pointer
    av_freep(&startFrame->data[0]);
    startFrame->data[0] = img->getRawImage();
    startFrame->height = h;
    startFrame->width = w;
    startFrame->format = (AVPixelFormat) pixelMap[img->getPixelCode()];

    success = av_image_alloc(endFrame->data, endFrame->linesize,
                    w, h,
                    (AVPixelFormat) codecPixelMap[codecContext->codec_id], 16);

    if (success < 0) {
        yCError(FFMPEGMONITOR, "Cannot allocate end frame buffer!");
        av_frame_free(&startFrame);
        av_frame_free(&endFrame);
        return -1;
    }
    
    endFrame->height = h;
    endFrame->width = w;
    endFrame->format = (AVPixelFormat) codecPixelMap[codecContext->codec_id];

    // Convert the image into end format
    static struct SwsContext *img_convert_ctx;
    
    img_convert_ctx = sws_getContext(w, h, 
                                        (AVPixelFormat) pixelMap[img->getPixelCode()], 
                                        w, h,
                                        (AVPixelFormat) codecPixelMap[codecContext->codec_id],
                                        SWS_BICUBIC,
                                        NULL, NULL, NULL);
    if (img_convert_ctx == NULL) {
        yCError(FFMPEGMONITOR, "Cannot initialize pixel format conversion context!");
        av_freep(&endFrame->data[0]);
        av_frame_free(&startFrame);
        av_frame_free(&endFrame);
        return -1;
    }
    
    int ret = sws_scale(img_convert_ctx, startFrame->data, startFrame->linesize, 0, 
                        h, endFrame->data, endFrame->linesize);

    if (ret < 0) {
        yCError(FFMPEGMONITOR, "Could not convert pixel format!");
        sws_freeContext(img_convert_ctx);
        av_freep(&endFrame->data[0]);
        av_frame_free(&startFrame);
        av_frame_free(&endFrame);
        return -1;
    }

    if (firstTime) {
        codecContext->width = w;
        codecContext->height = h;
        codecContext->pix_fmt = (AVPixelFormat) codecPixelMap[codecContext->codec_id];

        ret = avcodec_open2(codecContext, codec, NULL);
        if (ret < 0) {
            yCError(FFMPEGMONITOR, "Could not open codec");
            sws_freeContext(img_convert_ctx);
            av_freep(&endFrame->data[0]);
            av_frame_free(&startFrame);
            av_frame_free(&endFrame);
            return -1;
        }
        firstTime = false;
    }

    startFrame->pts = codecContext->frame_number;

    ret = avcodec_send_frame(codecContext, endFrame);
    if (ret < 0) {
        yCError(FFMPEGMONITOR, "Error sending a frame for encoding");
        sws_freeContext(img_convert_ctx);
        av_freep(&endFrame->data[0]);
        av_frame_free(&startFrame);
        av_frame_free(&endFrame);
        return -1;
    }

    ret = avcodec_receive_packet(codecContext, pkt);
    sws_freeContext(img_convert_ctx);
    av_freep(&endFrame->data[0]);
    av_frame_free(&startFrame);
    av_frame_free(&endFrame);

    if (ret == AVERROR(EAGAIN)) {
        yCError(FFMPEGMONITOR, "Error EAGAIN");
        return -1;
    } else if (ret == AVERROR_EOF) {
        yCError(FFMPEGMONITOR, "Error EOF");
        return -1;
    } else if (ret < 0) {
        yCError(FFMPEGMONITOR, "Error during encoding");
        return -1;
    }

    return 0;
}

int FfmpegMonitorObject::decompress(AVPacket* pkt, int w, int h, int pixelCode, int pixelSize) {
    
    yCTrace(FFMPEGMONITOR, "decompress");
    AVFrame *startFrame;
    AVFrame *endFrame;

    if (firstTime) {
        codecContext->width = w;
        codecContext->height = h;
        codecContext->pix_fmt = (AVPixelFormat) codecPixelMap[codecContext->codec_id];

        int ret = avcodec_open2(codecContext, codec, NULL);
        if (ret < 0) {
            yCError(FFMPEGMONITOR, "Could not open codec");
            return -1;
        }
        firstTime = false;
    }

    // Allocate video frame
    startFrame = av_frame_alloc();
    if (startFrame == NULL) {
        yCError(FFMPEGMONITOR, "Could not allocate start frame!");
        return -1;
    }

    int ret = avcodec_send_packet(codecContext, pkt);
    if (ret < 0) {
        yCError(FFMPEGMONITOR, "Error sending a frame for encoding");
        av_frame_free(&startFrame);
        return -1;
    }

    ret = avcodec_receive_frame(codecContext, startFrame);
    if (ret == AVERROR(EAGAIN)) {
        yCError(FFMPEGMONITOR, "Error EAGAIN");
        av_frame_free(&startFrame);
        return -1;
    }
    else if (ret == AVERROR_EOF) {
        yCError(FFMPEGMONITOR, "Error EOF");
        av_frame_free(&startFrame);
        return -1;
    }
    else if (ret < 0) {
        yCError(FFMPEGMONITOR, "Error during encoding");
        av_frame_free(&startFrame);
        return -1;
    }
    
    // Allocate a video frame for end frame
    endFrame = av_frame_alloc();
    if (endFrame == NULL) {
        yCError(FFMPEGMONITOR, "Could not allocating start frame!");
        av_frame_free(&startFrame);
        return -1;
    }


    int success = av_image_alloc(endFrame->data, endFrame->linesize,
                    w, h,
                    (AVPixelFormat) pixelMap[pixelCode], 16);
    
    if (success < 0) {
        yCError(FFMPEGMONITOR, "Error allocating end frame buffer!");
        av_frame_free(&startFrame);
        av_frame_free(&endFrame);
        return -1;
    }

    endFrame->height = h;
    endFrame->width = w;
    endFrame->format = (AVPixelFormat) pixelMap[pixelCode];

    // Convert the image into RGB format
    static struct SwsContext *img_convert_ctx;
    
    img_convert_ctx = sws_getContext(w, h, 
                                        (AVPixelFormat) codecPixelMap[codecContext->codec_id], 
                                        w, h,
                                        (AVPixelFormat) pixelMap[pixelCode],
                                        SWS_BICUBIC,
                                        NULL, NULL, NULL);
    if (img_convert_ctx == NULL) {
        yCError(FFMPEGMONITOR, "Cannot initialize the pixel format conversion context!");
        av_freep(&endFrame->data[0]);
        av_frame_free(&endFrame);
        av_frame_free(&startFrame);
        return -1;
    }
    
    ret = sws_scale(img_convert_ctx, startFrame->data, startFrame->linesize, 0, 
                        h, endFrame->data, endFrame->linesize);

    if (ret < 0) {
        yCError(FFMPEGMONITOR, "Could not convert pixel format!");
        av_freep(&endFrame->data[0]);
        av_frame_free(&endFrame);
        av_frame_free(&startFrame);
        sws_freeContext(img_convert_ctx);
        return -1;
    }

    memcpy(imageOut.getRawImage(), endFrame->data[0], success);
    av_freep(&endFrame->data[0]);
    av_frame_free(&endFrame);
    av_frame_free(&startFrame);
    sws_freeContext(img_convert_ctx);

    return 0;

}

int FfmpegMonitorObject::getParamsFromCommandLine(string carrierString, AVCodecID &codecId) {
    
    vector<string> parameters;
    split(carrierString, '+', parameters);
    for (string param: parameters) {

        if (find(FFMPEGPORTMONITOR_IGNORE_PARAMS.begin(), FFMPEGPORTMONITOR_IGNORE_PARAMS.end(), param) != FFMPEGPORTMONITOR_IGNORE_PARAMS.end()) {
            continue;   // Skip yarp initial parameters
        }

        int pointPosition = param.find('.');
        if (pointPosition == string::npos) {
            yCError(FFMPEGMONITOR, "Error parsing parameters!");
            return -1;
        }
        string paramKey = param.substr(0, pointPosition);
        string paramValue = param.substr(pointPosition + 1, param.length());
        
        // Parsing codec
        if (paramKey == FFMPEGPORTMONITOR_CL_CODEC_KEY) {
            bool found = false;
            for (int i = 0; i < FFMPEGPORTMONITOR_CL_CODECS.size(); i++) {
                if (paramValue == FFMPEGPORTMONITOR_CL_CODECS[i]) {
                    codecId = (AVCodecID) FFMPEGPORTMONITOR_CODE_CODECS[i];
                    codecName = paramValue;
                    found = true;
                    break;
                }
            }

            if (!found) {
                yCError(FFMPEGMONITOR, "Unrecognized codec: %s", paramValue.c_str());
                return -1;
            } else {
                continue;
            }
            
        }
        
        // Parsing codec context params
        paramsMap.insert( pair<string, string>(paramKey, paramValue) );
        return 0;
    }

}

int FfmpegMonitorObject::setCommandLineParams() {
        
    for (auto const& x : paramsMap) {

        string key = x.first;
        string value = x.second;

        char *opt;

        if (find(FFMPEGPORTMONITOR_PRIV_PARAMS.begin(), FFMPEGPORTMONITOR_PRIV_PARAMS.end(), key) != FFMPEGPORTMONITOR_PRIV_PARAMS.end()) {
            av_opt_set(codecContext->priv_data, key.c_str(), value.c_str(), 0);
            av_opt_get(codecContext->priv_data, key.c_str(), 0, (uint8_t **) &opt);
            continue;
        }

        av_opt_set(codecContext, key.c_str(), value.c_str(), 0);        
        av_opt_get(codecContext, key.c_str(), 0, (uint8_t **) &opt);
    }
    
}
