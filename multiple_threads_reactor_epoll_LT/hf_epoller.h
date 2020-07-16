#ifndef HF_EPOLLER_H_
#define HF_EPOLLER_H_

//#include <poll.h>
#include <sys/epoll.h>

#include <vector>
#include <map>

#include "hf_non_copy.h"
#include "hf_channel.h"

namespace hf
{

// Epoller主要是对多路复用函数的封装，
// 只有EventLoop拥有该对象
class Epoller : public NonCopy
{
    public:
        // EventLoop中也需要使用该类型，所以定义为public
        typedef std::vector<Channel*> ChannelVector;
        typedef std::vector<epoll_event> EventVector;

        Epoller();
        ~Epoller();

        // 运行多路复用的函数
        // 只能在IO线程中运行，不用加锁
        void Poll(int time_out_ms, ChannelVector* active_channels);

        // 添加Epoller中监控的文件描述符或者更改已有文件描述符检测事件
        // 这里不是常量引用是因为第一次添加channel时，需要设置其中的index_成员
        // 只能在IO线程中运行，不用加锁
        void UpdateChannels(Channel* channel);

        // 由于该函数需要更新监听事件的数据结构，所以必须是线程安全的
        // 上层保证其线程安全性（上层调用AssertInIOThread）
        void RemoveChannel(Channel* channel);
    private:

        static const int kInitEventListSize = 50;
        // 这两种类型只有自己才需要使用，所以定义为private
        // typedef std::vector<pollfd> EpollfdVector;
        // typedef std::map<int, Channel*> FdToChannelMap;

        //当多路复用返回时，检测其中是否有就绪的文件描述符，如果有，则将加入active_channels
        void FillActiveChannel(int num_events, ChannelVector* active_channels) const;

        int epoll_fd_;
        EventVector events_;

        // Channel好像可以定义为const指针
        void UpdateEpoll(int operation, Channel* channel);

        //当前是否运行在创建EventLoop对象的线程上
        //void AssertInEventLoopThread(){
        //    event_loop_->AssertInCreaeteThread();
        //}

        //用于缓存监控的fd
        //EpollfdVector pollfds_;
        // channels使用map的原因是为了加快FillActiveChannels函数的查找速度
        // Epoller并不拥有Channel，所以当Channel对象析勾之前，必须把自己注销掉，否则会产生空悬指针
        // FdToChannelMap channels_;

    

};
}
#endif 