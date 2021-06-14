#ifndef EDGEPLAYER_CONFIG_H
#define EDGEPLAYER_CONFIG_H

struct GlobalConfig
{
    GlobalConfig()
    {
        GameMode = true;
        GcInterval = 1000;
    }

    bool GameMode;
    int GcInterval;
};

extern GlobalConfig g_config;

namespace Config
{
    const unsigned kGameTerm = 1;    // 单位 s
    const unsigned kLatencyThreshold = 80; // 单位 ms

    // 任务的最短耗时
    const unsigned kMinRequestTime = 80;   // 单位 ms
    // 任务的最小存储占用
    const unsigned kMinRequestStorage = 80; // MB

    // proxy 节点获取服务器权重的周期，单位 s
    const unsigned kProxyUpdateServerWeightTime = kGameTerm;

    // 内存做一次 GC 的时间，ms
    const unsigned kGcTime = 100;
    // 一次 GC 的释放大小，MB
    const unsigned kGcSize = 30;
    // 默认的 GC 间隔，ms
    const unsigned kGcInter = 1000;
}

#endif //EDGEPLAYER_CONFIG_H
