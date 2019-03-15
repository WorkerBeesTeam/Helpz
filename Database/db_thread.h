#ifndef HELPZ_DATABASE_THREAD_H
#define HELPZ_DATABASE_THREAD_H

#include <thread>
#include <condition_variable>
#include <mutex>
#include <queue>

#include <QVariantList>

#include <Helpz/db_connection_info.h>

namespace Helpz {
namespace Database {

class Base;
class Thread : public std::thread
{
public:
    Thread(Connection_Info&& info);
    Thread(std::shared_ptr<Base> db);
    ~Thread();

    void stop();

    void add_pending_query(QString&& sql, std::vector<QVariantList>&& values);
private:
    void open_and_run(Connection_Info&& info);
    void store_and_run(std::shared_ptr<Base> db);
    void run();

    struct query_pack
    {
        QString sql;
        std::vector<QVariantList> values;
    };

    bool break_flag_;
    std::queue<query_pack> data_queue_;
    std::mutex mutex_;
    std::condition_variable cond_;

    std::shared_ptr<Base> db_;
};

} // namespace Database
} // namespace Helpz

#endif // HELPZ_DATABASE_THREAD_H
