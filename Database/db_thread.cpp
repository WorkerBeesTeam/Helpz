#include <sstream>

#include <QLoggingCategory>

#include "db_base.h"
#include "db_thread.h"

Q_DECLARE_LOGGING_CATEGORY(DBLog)

namespace Helpz {
namespace Database {

Thread::Thread(Connection_Info&& info)
{
    thread_ = new std::thread(&Thread::open_and_run, this, std::move(info));
}

Thread::Thread(std::shared_ptr<Base> db)
{
    thread_ = new std::thread(&Thread::store_and_run, this, std::move(db));
}

Thread::~Thread()
{
    stop();
    if (thread_->joinable())
    {
        thread_->join();
    }
    delete thread_;
}

void Thread::stop()
{
    break_flag_ = true;
    cond_.notify_one();
}

const Base* Thread::db() const
{
    return db_.get();
}

void Thread::add_query(std::function<void (Base *)> callback)
{
    std::lock_guard lock(mutex_);
    data_queue_.push(std::move(callback));
    cond_.notify_one();
}

void Thread::add_pending_query(QString &&sql, std::vector<QVariantList> &&values_list,
                               std::function<void(QSqlQuery&, const QVariantList&)> callback)
{
    if (sql.isEmpty() || values_list.empty())
    {
        qCritical(DBLog) << "Attempt to add bad pending query";
        return;
    }

    std::function<void (Base *)> item =
            std::bind(process_query, std::placeholders::_1, std::move(sql), std::move(values_list), std::move(callback));

    std::lock_guard lock(mutex_);
    data_queue_.push(std::move(item));
    cond_.notify_one();
}

void Thread::open_and_run(Connection_Info&& info)
{
    std::stringstream s;
    s << "pending_queries" << std::this_thread::get_id();
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
    break_flag_ = false;
    std::unique_lock lock(mutex_, std::defer_lock);
    while (!break_flag_)
    {
        lock.lock();
        cond_.wait(lock, [this](){ return data_queue_.size() || break_flag_; });
        if (break_flag_)
        {
            break;
        }
        std::function<void (Base *)> item{std::move(data_queue_.front())};
        data_queue_.pop();
        lock.unlock();

        if (item)
        {
            try
            {
                item(db_.get());
            }
            catch (...) {}
        }
    }
    db_.reset();
}

/*static*/ void Thread::process_query(Base* db, QString &sql, std::vector<QVariantList> &values_list,
                                      std::function<void (QSqlQuery &, const QVariantList &)>& callback)
{
    QSqlQuery query;
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

} // namespace Database
} // namespace Helpz
