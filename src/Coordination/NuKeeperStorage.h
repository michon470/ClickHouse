#pragma once

#include <Common/ThreadPool.h>
#include <Common/ZooKeeper/IKeeper.h>
#include <Common/ConcurrentBoundedQueue.h>
#include <Common/ZooKeeper/ZooKeeperCommon.h>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace DB
{

using namespace DB;
struct NuKeeperStorageRequest;
using NuKeeperStorageRequestPtr = std::shared_ptr<NuKeeperStorageRequest>;
using ResponseCallback = std::function<void(const Coordination::ZooKeeperResponsePtr &)>;

class NuKeeperStorage
{
public:
    int64_t session_id_counter{0};

    struct Node
    {
        String data;
        Coordination::ACLs acls{};
        bool is_ephemeral = false;
        bool is_sequental = false;
        Coordination::Stat stat{};
        int32_t seq_num = 0;
    };

    struct ResponseForSession
    {
        int64_t session_id;
        Coordination::ZooKeeperResponsePtr response;
    };

    using ResponsesForSessions = std::vector<ResponseForSession>;

    struct RequestForSession
    {
        int64_t session_id;
        Coordination::ZooKeeperRequestPtr request;
    };

    using RequestsForSessions = std::vector<RequestForSession>;

    using Container = std::map<std::string, Node>;
    using Ephemerals = std::unordered_map<int64_t, std::unordered_set<String>>;
    using SessionAndWatcher = std::unordered_map<int64_t, std::unordered_set<String>>;
    using SessionIDs = std::vector<int64_t>;

    using Watches = std::map<String /* path, relative of root_path */, SessionIDs>;

    Container container;
    Ephemerals ephemerals;
    SessionAndWatcher sessions_and_watchers;

    int64_t zxid{0};
    bool finalized{false};

    Watches watches;
    Watches list_watches;   /// Watches for 'list' request (watches on children).

    void clearDeadWatches(int64_t session_id);

    int64_t getZXID()
    {
        return zxid++;
    }

public:
    NuKeeperStorage();

    int64_t getSessionID()
    {
        return session_id_counter++;
    }

    ResponsesForSessions processRequest(const Coordination::ZooKeeperRequestPtr & request, int64_t session_id);
    ResponsesForSessions finalize(const RequestsForSessions & expired_requests);
};

}