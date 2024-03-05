#include "CaptureVideoThread.h"
#include "BXC_VideoRecorder.h"
#include "BXC_AvEncoder.h"
#include "FpsControl.h"
#include "Recorder.h"
#include <QDebug>
#include <QsLog.h>

using namespace BXC_MediaLibrary;
CaptureVideoThread::CaptureVideoThread(QObject *parent,CaptureVideoDevice *videoDevice): QThread(parent),mVideoDevice(videoDevice){
    mRecorder = (Recorder *)parent;

}
CaptureVideoThread::~CaptureVideoThread(){
    mIsStop = true;
    while (true) {
        if(isFinished()){
            break;
        }else{
            msleep(1);
        }
    }
    //    terminate();//强制结束
    //    wait();//配合terminate()强制结束后，等待回收资源
}
void CaptureVideoThread::run(){
    mIsStop = false;

    int fps = mVideoDevice->getFps();
    int width = mVideoDevice->width;
    int height = mVideoDevice->height;


    int      frameBuffSize = height * width * 4;//这个长乘款X4框架大小
    uint8_t* frameBuff = new uint8_t[frameBuffSize];
    uint8_t* frameBuff_rgba = new uint8_t[frameBuffSize];

    BXC_AvFrame* videoFrame = new BXC_AvFrame(AVVIDEO, frameBuffSize);


    int64_t frameTimestamp = 0;
    int64_t frameCount = 0;

    int show_interval = 5;//屏幕显示时间隔数量
    if(mVideoDevice->isCamera()){
        show_interval = 2;
    }

    int ret;
    FpsControl fpsControl(fps);
    fpsControl.realTimeStart();

    while (true)
    {
        fpsControl.intervalStart();
        if(mIsStop){
            break;
        }
        //截屏流程 1.BXC获取框架BXC_VideoRecorder 存放截取设备名称 视屏采样实例
        // 宽度 高度；存放数据的地址，地址大小，采样时间错
        ret = BXC_get_frame(mRecorder->getVideoRecorder(), frameBuff, frameBuffSize,frameTimestamp);
        //截屏成功
        if (ret >= 0) {
            // 拷贝数据存放 BXC_AvFrame结构
            memcpy(videoFrame->data, frameBuff, frameBuffSize);
            videoFrame->size = frameBuffSize;
            videoFrame->timestamp = frameTimestamp;
            videoFrame->count = frameCount;

            //添加数据帧待编码帧，由编码器解码 编码器BXC_AvEncoder包含比特率 fps等设置，
            BXC_send_frame(mRecorder->getAvEncoder(), videoFrame);
            // 0 %5 取余  = 0
            if(0 == frameCount % show_interval){
                //BXC 像素格式PIXEL_BGRA 编码转换
                if(PIXEL_BGRA == mRecorder->getPixelFormat()){
                    // (bgra->rgba)大端模式，RGBA中R存储在高位，A存储在低位
                    memcpy_s(frameBuff_rgba,frameBuffSize,frameBuff,frameBuffSize);
                    for (int i = 0; i < width*height; i++) {
                        frameBuff_rgba[i] = (frameBuff_rgba[i] & 0x000000FF) |         // ______AA
                              ((frameBuff_rgba[i] & 0x0000FF00) << 16) | // RR______
                              (frameBuff_rgba[i] & 0x00FF0000) |         // __GG____
                              ((frameBuff_rgba[i] & 0xFF000000) >> 16);  // ____BB__
                    }
                    QImage image(frameBuff_rgba, width,height,QImage::Format_RGB32);
                    emit mRecorder->setImage(image.copy());

                }else if(PIXEL_RGB == mRecorder->getPixelFormat()){
                    QImage image(frameBuff, width,height,QImage::Format_RGB888);
                    emit mRecorder->setImage(image.copy());
                }
            }
            ++frameCount;//5桢取一个画面
            fpsControl.realTimeIncrease();
        }else{
            continue;
            //break;
        }

        fpsControl.adjust();
//        qDebug() <<"realTimeFps=" << fpsControl.getRealTimeFps();
    }

    if(frameBuff){
        delete[] frameBuff;
        frameBuff = nullptr;
    }
    if(frameBuff_rgba){
        delete[] frameBuff_rgba;
        frameBuff_rgba = nullptr;
    }
    this->exit();
}
