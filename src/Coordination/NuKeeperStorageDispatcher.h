#pragma once

#if !defined(ARCADIA_BUILD)
#    include <Common/config.h>
#    include "config_core.h"
#endif

#if USE_NURAFT

#include <Common/ThreadPool.h>
#include <Common/ConcurrentBoundedQueue.h>
#include <Poco/Util/AbstractConfiguration.h>
#include <Common/Exception.h>
#include <common/logger_useful.h>
#include <functional>
#include <Coordination/NuKeeperServer.h>


namespace DB
{

using ZooKeeperResponseCallback = std::function<void(const Coordination::ZooKeeperResponsePtr & response)>;

class NuKeeperStorageDispatcher
{

private:
    Poco::Timespan operation_timeout{0, Coordination::DEFAULT_OPERATION_TIMEOUT_MS * 1000};


    std::mutex push_request_mutex;

    using RequestsQueue = ConcurrentBoundedQueue<NuKeeperStorage::RequestForSession>;
    RequestsQueue requests_queue{1};
    std::atomic<bool> shutdown_called{false};
    using SessionToResponseCallback = std::unordered_map<int64_t, ZooKeeperResponseCallback>;

    std::mutex session_to_response_callback_mutex;
    SessionToResponseCallback session_to_response_callback;

    ThreadFromGlobalPool processing_thread;

    std::unique_ptr<NuKeeperServer> server;

    Poco::Logger * log;

private:
    void processingThread();
    void setResponse(int64_t session_id, const Coordination::ZooKeeperResponsePtr & response);

public:
    NuKeeperStorageDispatcher();

    void initialize(const Poco::Util::AbstractConfiguration & config);

    void shutdown();

    ~NuKeeperStorageDispatcher();

    bool putRequest(const Coordination::ZooKeeperRequestPtr & request, int64_t session_id);

    bool isLeader() const
    {
        return server->isLeader();
    }

    bool hasLeader() const
    {
        return server->isLeaderAlive();
    }

    int64_t getSessionID()
    {
        return server->getSessionID();
    }

    void registerSession(int64_t session_id, ZooKeeperResponseCallback callback);
    /// Call if we don't need any responses for this session no more (session was expired)
    void finishSession(int64_t session_id);

};

}

#endif