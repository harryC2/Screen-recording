#include "FpsControl.h"
#include <chrono>
#include <thread>
#include <iostream>

FpsControl::FpsControl(int fps):target_fps(fps){
    //fpx 25  20桢，初始化设备时填入的信息  1000/25 = 40ms
    interval_duration = float(float(1000) / float(fps));
}
FpsControl::~FpsControl() {

}

void FpsControl::realTimeStart() {
    realtime_start = gettime();
}
void FpsControl::realTimeIncrease() {
	++realtime_totalCount;
}

void FpsControl::intervalStart() {
	interval_start = gettime();
}

void FpsControl::adjust() {

    //单帧间隔计算及其休眠计算
    interval_end = gettime(); // 获得毫秒级时间戳
    //40ms   - end - start;
    // 超出的额外时间
    interval_extra = interval_duration - (interval_end - interval_start);
    // 大于零超时，线程暂停，小于零需要补充时间下次超时就可以不补

    //例子： 50ms时开始截图，这个就 >0  =10; 这个时候开始睡眠10/2 5毫秒；
    if (interval_extra > 0) {
        if (total_interval_extra > 0) {
            if (total_interval_extra >= interval_extra) {
                total_interval_extra -= interval_extra;
            }
            else {
                interval_extra -= total_interval_extra;
                total_interval_extra = 0;
                std::this_thread::sleep_for(std::chrono::milliseconds(interval_extra / 2));
            }
        }
        else {
            std::this_thread::sleep_for(std::chrono::milliseconds(interval_extra / 2));
        }
    }
    else if (interval_extra < 0) {
        total_interval_extra += (-interval_extra);
    }

    //实时计算帧率
    realtime_duration = gettime() - realtime_start; // 实时帧率 = 当前时间-开始时间；
    // 开睡眠？ 那肯定是>0呀
    if (realtime_duration > 0) {

        //fps = float（realtime_totalCount * 1000）/40ms = 100；
        //realtime_totalCount 这个是解码一帧，就++; target_fps = 25 或者20
        realtime_fps = float(realtime_totalCount * 1000) / realtime_duration;

        // 这里如果帧率过大了，再睡完剩余的一般秒；没过就慢慢补
        if (realtime_fps > target_fps) {
            std::this_thread::sleep_for(std::chrono::milliseconds(interval_extra / 2));
        }
    }

}
float FpsControl::getRealTimeFps(){
    return realtime_fps;
}
int64_t FpsControl::gettime()// 获取毫秒级时间戳（13位）
{
	return std::chrono::duration_cast<std::chrono::milliseconds>(
		std::chrono::system_clock::now().time_since_epoch()).
		count();

}

