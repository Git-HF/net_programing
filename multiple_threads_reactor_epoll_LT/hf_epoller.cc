#include "hf_epoller.h"

#include <sys/poll.h>

#include <cassert>

#include "hf_utils.h"


namespace hf
{
Epoller::Epoller()
: epoll_fd_(::epoll_create1(EPOLL_CLOEXEC)),
  events_(kInitEventListSize)
{
    if(epoll_fd_ < 0)
    {
        ErrorAndQuit("Epoller::Epoller epoll_create1 error");
    }

    // 因为linux中，poll和epoll中的宏是相同的，所以这里为了不修改Channel::HandleEvent中的事件，这里
    // 需要判断epoll的事件是否和poll中的事件相同。
    assert(EPOLLIN == POLLIN);
    assert(EPOLLPRI == POLLPRI);
    assert(EPOLLOUT == POLLOUT);
    assert(EPOLLRDHUP == POLLRDHUP);
    assert(EPOLLERR == POLLERR);
    assert(EPOLLHUP == POLLHUP);
}

Epoller::~Epoller()
{

}

// 运行多路复用的函数
void Epoller::Poll(int time_out_ms, ChannelVector* active_channels)
{
    int num_events = ::epoll_wait(epoll_fd_, 
                                  events_.data(), 
                                  static_cast<int>(events_.size()),
                                  time_out_ms);
    // 等于0时，继续监听
    if(num_events > 0)
    {
        FillActiveChannel(num_events, active_channels);
    }
    else if(num_events < 0)
    {
        ErrorAndQuit("Epoller::Epoll error");
    }
}

// 添加Epoller中监控的文件描述符或者更改已有文件描述符检测事件
void Epoller::UpdateChannels(Channel *channel)
{
    // 只能在IO线程中运行。

    //调用该函数之前，已经由上层调用AssertInCreateThread()
    //AssertInEventLoopThread();

    if((channel->GetIndex() == Channel::kNewChannel) ||
        (channel->GetIndex() == Channel::kDeletedChannel))
    {
        if(channel->IsNoneEvent())
            return;
        
        UpdateEpoll(EPOLL_CTL_ADD, channel);

        //设置Channel的状态，不能丢
        channel->SetIndex(Channel::kOldChannel);
    }
    else
    {
        assert(channel->GetIndex() == Channel::kOldChannel);

        if(channel->IsNoneEvent())
        {
            UpdateEpoll(EPOLL_CTL_DEL, channel);
            channel->SetIndex(Channel::kDeletedChannel);
        }
        else
        {
            UpdateEpoll(EPOLL_CTL_MOD, channel);
        }
    }
}

void Epoller::RemoveChannel(Channel* channel)
{
    // 只能在IO线程中执行
    // 已经由上层保证

    // 只有在epoll中注册过的channel才需要被删除，否则不需要。
    if(channel->GetIndex() == Channel::kOldChannel)
    {
        UpdateEpoll(EPOLL_CTL_DEL, channel);
        // 注意设置Channel状态。
        channel->SetIndex(Channel::kDeletedChannel);
    }
}

void Epoller::FillActiveChannel(int num_events, ChannelVector* active_channels) const
{
    // 是否在IO线程中
    // 不需要判断，因为该函数为私有成员，只有Epoller本身才可以调用，而Epoller本身一定处在IO线程中。
    //AssertInEventLoopThread();
    
    Channel* channel;
    for(int i = 0; i < num_events; ++i)
    {
        channel = static_cast<Channel*>(events_[i].data.ptr);
        // 获取就绪事件
        channel->SetRevents(events_[i].events);
        active_channels->push_back(channel);
    }
}


void Epoller::UpdateEpoll(int operation, Channel* channel)
{
    epoll_event tmp_event;
    // 使用data中的ptr数据，而不是fd数据
    tmp_event.data.ptr = channel;
    tmp_event.events = channel->GetEvents();
    int ret = ::epoll_ctl(epoll_fd_, operation, channel->GetFD(), &tmp_event);
    if(ret == -1)
    {
        switch (operation)
        {
        case EPOLL_CTL_ADD:
            OutputError("Epoll::UpdateEpoll epoll_ctl ADD error");
            ErrorAndQuit("");
            break;
        
        case EPOLL_CTL_MOD:
            OutputError("Epoll::UpdateEpoll epoll_ctl MOD error");
            ErrorAndQuit("");
            break;

        case EPOLL_CTL_DEL:
            OutputError("Epoll::UpdateEpoll epoll_ctl DEL error");
            ErrorAndQuit("");
            break;

        default:
            ErrorAndQuit("Epoll::UpdateEpoll epoll_ctl unknow error");
            break;
        }
    }
}


}