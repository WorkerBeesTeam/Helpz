#include <sstream>
#include <iostream>

#include <QLoggingCategory>

#include "db_base.h"
#include "db_thread.h"

Q_DECLARE_LOGGING_CATEGORY(DBLog)

namespace Helpz {
namespace Database {

Thread::Thread(Connection_Info&& info) :
    break_flag_(false),
    thread_(&Thread::open_and_run, this, std::move(info))
{
}

Thread::Thread(std::shared_ptr<Base> db) :
    break_flag_(false),
    thread_(&Thread::store_and_run, this, std::move(db))
{
}

Thread::~Thread()
{
    stop();
    if (thread_.joinable())
    {
        thread_.join();
    }
}

void Thread::stop()
{
    std::lock_guard lock(mutex_);
    break_flag_ = true;
    cond_.notify_one();
}

void Thread::set_priority(int priority)
{
#ifdef Q_OS_UNIX
    sched_param sch_param;
    sch_param.sched_priority = priority;
    pthread_setschedparam(thread_.native_handle(), SCHED_FIFO, &sch_param);
#endif
}

const Base* Thread::db() const
{
    return db_.get();
}

std::future<void> Thread::add_query(std::function<void (Base *)> callback)
{
    std::packaged_task<void(Base*)> task(callback);
    return add_task(std::move(task));
}

std::future<void> Thread::add_pending_query(QString &&sql, std::vector<QVariantList> &&values_list,
                               std::function<void(QSqlQuery&, const QVariantList&)> callback)
{
    if (sql.isEmpty())
    {
        qCritical(DBLog) << "Attempt to add empty pending query";
        return {};
    }

    std::function<void (Base *)> item =
            std::bind(process_query, std::placeholders::_1, std::move(sql), std::move(values_list), std::move(callback));

    std::packaged_task<void(Base*)> task(item);
    return add_task(std::move(task));
}

std::future<void> Thread::add_task(std::packaged_task<void (Base*)>&& task)
{
    std::future<void> res = task.get_future();
    std::lock_guard lock(mutex_);
    data_queue_.push(std::move(task));
    cond_.notify_one();
    return res;
}

void Thread::open_and_run(Connection_Info&& info)
{
    std::stringstream s;
    s << "pending_queries_" << std::this_thread::get_id();
    db_.reset(new Base{info, QString::fromStdString(s.str())});
    run();
}

void Thread::store_and_run(std::shared_ptr<Base> db)
{
    db_ = std::move(db);
    run();
}

void Thread::run()
{
    std::unique_lock lock(mutex_, std::defer_lock);
    while (!break_flag_)
    {
        lock.lock();
        cond_.wait(lock, [this](){ return data_queue_.size() || break_flag_; });
        if (break_flag_)
        {
            break;
        }
        std::packaged_task<void(Base*)> task{std::move(data_queue_.front())};
        data_queue_.pop();
        lock.unlock();

        try
        {
            task(db_.get());
        }
        catch (...) {}
    }
    db_.reset();
}

/*static*/ void Thread::process_query(Base* db, QString &sql, std::vector<QVariantList> &values_list,
                                      std::function<void (QSqlQuery &, const QVariantList &)>& callback)
{
    auto start_point = std::chrono::system_clock::now();

    QSqlQuery query;
    if (values_list.empty())
    {
        query = db->exec(sql);
        if (callback)
        {
            callback(query, QVariantList());
        }
    }
    else
    {
        for (const QVariantList& values: values_list)
        {
            query = db->exec(sql, values);
            if (callback)
            {
                callback(query, values);
            }
            query.clear();
        }
    }

    std::chrono::milliseconds delta = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - start_point);
    if (delta > std::chrono::milliseconds(100))
    {
        std::cout << "-----------> db freeze " << delta.count() << "ms vs " << values_list.size() << " sql " << sql.toStdString() << std::endl;
    }
}

} // namespace Database
} // namespace Helpz
