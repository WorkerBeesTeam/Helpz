#include <sstream>

#include <QLoggingCategory>

#include "db_base.h"
#include "db_thread.h"

Q_DECLARE_LOGGING_CATEGORY(DBLog)

namespace Helpz {
namespace Database {

Thread::Thread(Connection_Info&& info) :
    std::thread(&Thread::open_and_run, this, std::move(info))
{}

Thread::Thread(std::shared_ptr<Base> db) :
    std::thread(&Thread::store_and_run, this, std::move(db))
{}

Thread::~Thread()
{
    stop();
    if (joinable())
    {
        join();
    }
}

void Thread::stop()
{
    break_flag_ = true;
    cond_.notify_one();
}

void Thread::add_pending_query(QString &&sql, std::vector<QVariantList> &&values,
                               std::function<void(QSqlQuery&, const QVariantList&)> call_back)
{
    if (sql.isEmpty() || values.empty())
    {
        qCritical(DBLog) << "Attempt to add bad pending query";
        return;
    }

    std::lock_guard lock(mutex_);
    data_queue_.push(Query_Item{std::move(sql), std::move(values), std::move(call_back)});
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
    QSqlQuery query;
    std::unique_lock lock(mutex_, std::defer_lock);
    while (true)
    {
        lock.lock();
        cond_.wait(lock, [this](){ return data_queue_.size() || break_flag_; });
        if (break_flag_)
        {
            break;
        }
        Query_Item item{std::move(data_queue_.front())};
        data_queue_.pop();
        lock.unlock();

        for (const QVariantList& values: item.values_)
        {
            query = db_->exec(item.sql_, values);
            if (item.call_back_)
            {
                item.call_back_(query, values);
            }
            query.clear();
        }
    }
    db_.reset();
}

} // namespace Database
} // namespace Helpz
