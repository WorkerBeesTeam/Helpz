#ifndef HELPZ_DATABASE_THREAD_H
#define HELPZ_DATABASE_THREAD_H

#include <thread>
#include <condition_variable>
#include <mutex>
#include <queue>
#include <functional>
#include <future>

#include <QVariantList>

#include <Helpz/db_connection_info.h>

namespace Helpz {
namespace Database {

class Base;
class Thread
{
public:
    Thread(Connection_Info info, std::size_t thread_count = 1);
    Thread(std::shared_ptr<Base> db);
    ~Thread();

    void stop();

    template<typename Func>
    std::future<void> add(Func f)
    {
        std::packaged_task<void(Base*)> task(f);
        return add_task(std::move(task));
    }

    std::future<void> add_query(std::function<void(Base*)> callback);

    std::future<void> add_pending_query(QString&& sql, std::vector<QVariantList>&& values_list,
                           std::function<void(QSqlQuery&, const QVariantList&)> callback = nullptr);
private:
    std::future<void> add_task(std::packaged_task<void(Base*)>&& task);
    void open_and_run(const Connection_Info &info);
    void run(std::shared_ptr<Base> db);

    static void process_query(Base* db, QString& sql, std::vector<QVariantList>& values_list,
                       std::function<void(QSqlQuery&, const QVariantList&)>& callback);

    bool break_flag_;
    std::queue<std::packaged_task<void(Base*)>> data_queue_;
    std::mutex mutex_;
    std::condition_variable cond_;
    std::vector<std::thread> thread_list_;
};

} // namespace Database
} // namespace Helpz

#endif // HELPZ_DATABASE_THREAD_H
