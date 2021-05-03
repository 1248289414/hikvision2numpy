#include "library.h"

#include <iostream>
#include "HCNetSDK.h"
#include "PlayM4.h"

#include <opencv2/opencv.hpp>
#include <mutex>
#include <condition_variable>

LONG lUserID;
LONG lRealPlayHandle;
LONG lPort; //全局的播放库 port 号
NET_DVR_DEVICEINFO_V30 struDeviceInfo;

std::queue<cv::Mat> g_frameQueue;
std::mutex g_frameQueue_mutex;
std::condition_variable g_frameQueue_conditionVariable;
int nWidth = 0;
int nHeight = 0;

void CALLBACK DecCBFun(int nPort, char *pBuf, int nSize, FRAME_INFO *pFrameInfo, void *nReserved1, int nReserved2) {
    long lFrameType = pFrameInfo->nType;
    if (lFrameType == T_YV12) {
        nHeight = pFrameInfo->nHeight;
        nWidth = pFrameInfo->nWidth;
        cv::Mat YV12(pFrameInfo->nHeight + pFrameInfo->nHeight / 2, pFrameInfo->nWidth, CV_8UC1, (uchar *) pBuf);
        cv::Mat BGR(pFrameInfo->nHeight, pFrameInfo->nWidth, CV_8UC3);
        cvtColor(YV12, BGR, CV_YUV2BGR_YV12);

        g_frameQueue_mutex.lock();

        g_frameQueue.push(BGR);
        if (g_frameQueue.size() > 10)
            g_frameQueue.pop();

        g_frameQueue_mutex.unlock();
        g_frameQueue_conditionVariable.notify_all();
    }
}

void CALLBACK g_RealDataCallBack_V30(LONG lRealHandle, DWORD dwDataType, BYTE *pBuffer, DWORD dwBufSize, void *dwUser) {
    switch (dwDataType) {
        case NET_DVR_SYSHEAD: //系统头
            if (!PlayM4_GetPort(&lPort)) //获取播放库未使用的通道号
            {
                std::cout << NET_DVR_GetLastError() << std::endl;
            }
            //m_iPort = lPort; //第一次回调的是系统头,将获取的播放库 port 号赋值给全局 port,下次回调数据时即使用此 port 号播放
            if (dwBufSize > 0) {
                if (!PlayM4_SetStreamOpenMode(lPort, STREAME_REALTIME)) //设置实时流播放模式
                {
                    std::cout << NET_DVR_GetLastError() << std::endl;
                }
                if (!PlayM4_SetDecCBStream(lPort, 1)) //设置为视频流
                {
                    std::cout << NET_DVR_GetLastError() << std::endl;
                }
                if (!PlayM4_OpenStream(lPort, pBuffer, dwBufSize, SOURCE_BUF_MAX)) //打开流接口
                {
                    std::cout << NET_DVR_GetLastError() << std::endl;
                }
                //设置解码回调函数 只解码不显示
                if (!PlayM4_SetDecCallBack(lPort, DecCBFun)) {
                    std::cout << NET_DVR_GetLastError() << std::endl;
                }
                if (!PlayM4_Play(lPort, 0)) //播放开始
                {
                    std::cout << NET_DVR_GetLastError() << std::endl;
                }
            }
            break;
        case NET_DVR_STREAMDATA: //码流数据
        default: //其他数据
            if (dwBufSize > 0 && lPort != -1) {
                if (!PlayM4_InputData(lPort, pBuffer, dwBufSize)) {
                    std::cout << NET_DVR_GetLastError() << std::endl;
                }
            }
            break;
    }
}

void CALLBACK g_ExceptionCallBack(DWORD dwType, LONG UserID, LONG lHandle, void *pUser) {
    switch (dwType) {
        case EXCEPTION_RECONNECT: //预览时重连
            std::cout << "reconnect, " << NET_DVR_GetLastError() << std::endl;
            break;
        default:
            std::cout << NET_DVR_GetLastError() << std::endl;
            break;
    }
}

int getHeight() {
    return nHeight;
}

int getWidth() {
    return nWidth;
}

void init(char *sDVRIP, char *sUserName, char *sPassword) {
//---------------------------------------
// 初始化
    NET_DVR_Init();
//设置连接时间与重连时间
    NET_DVR_SetConnectTime(3000, 3);
    NET_DVR_SetReconnect(10000, true);
// 注册设备
    lUserID = NET_DVR_Login_V30(sDVRIP, 8000, sUserName, sPassword, &struDeviceInfo);
    if (lUserID < 0) {
        std::cout << "Login error, " << NET_DVR_GetLastError() << std::endl;
        NET_DVR_Cleanup();
        return;
    }
//---------------------------------------
//设置异常消息回调函数
    NET_DVR_SetExceptionCallBack_V30(0, nullptr, g_ExceptionCallBack, nullptr);
//---------------------------------------
//启动预览并设置回调数据流
    NET_DVR_PREVIEWINFO struPlayInfo = {0};
//    struPlayInfo.hPlayWnd = hWnd; //需要 SDK 解码时句柄设为有效值,仅取流不解码时可设为空
    struPlayInfo.lChannel = 1;//预览通道号
    struPlayInfo.dwStreamType = 0; //0-主码流,1-子码流,2-码流 3,3-码流 4,以此类推
    struPlayInfo.dwLinkMode = 0; //0- TCP 方式,1- UDP 方式,2- 多播方式,3- RTP 方式,4-RTP/RTSP,5-RSTP/HTTP
    struPlayInfo.bBlocked = 1; //0- 非阻塞取流,1- 阻塞取流
    lRealPlayHandle = NET_DVR_RealPlay_V40(lUserID, &struPlayInfo, g_RealDataCallBack_V30, nullptr);
    if (lRealPlayHandle < 0) {
        std::cout << "NET_DVR_RealPlay_V40 error, " << NET_DVR_GetLastError() << std::endl;
        NET_DVR_Logout(lUserID);
        NET_DVR_Cleanup();
        return;
    }
}

void getSize(int *size, int n) {
    if (n != 2) {
        std::cout << "getSize() failed, n != 2." << std::endl;
        return;
    }
    size[0] = nHeight;
    size[1] = nWidth;
}

void getFrame(unsigned char *frame, int DIM1, int DIM2, int DIM3) {
    std::unique_lock<std::mutex> uniqueLock(g_frameQueue_mutex);
    g_frameQueue_conditionVariable.wait(uniqueLock, []() {   //调用wait函数，先解锁lck，然后判断lambda的返回值
        return !g_frameQueue.empty();
    });

    cv::Mat src = g_frameQueue.front();

    if (DIM1 != src.rows || DIM2 != src.cols || DIM3 != src.channels()) {
        std::cout << "getFrame() failed, frame size much be ("
                  << src.rows << "," << src.cols << "," << src.channels()
                  << ")." << std::endl;
        return;
    }
    cv::Mat dest = cv::Mat(DIM1, DIM2, CV_8UC3, frame);
    src.copyTo(dest); //copyTo函数在不匹配的时候有可能从分配空间

    g_frameQueue.pop();
}

void release() {
    //---------------------------------------
    //关闭预览
    NET_DVR_StopRealPlay(lRealPlayHandle);
    //注销用户
    NET_DVR_Logout(lUserID);
    NET_DVR_Cleanup();

    cv::destroyAllWindows();
}