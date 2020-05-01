
#include "db_base.h"
#include "db_delete_row.h"

namespace Helpz {
namespace DB {

Delete_Row_Info::Delete_Row_Info(const Table& table, int field_index, const std::vector<Delete_Row_Info>& childs, bool set_null, int pk_index) :
    set_null_(set_null), field_i_(field_index), pk_i_(pk_index), table_(table), childs_(childs) {}

QString Delete_Row_Info::table_name() const
{
    return table_.name();
}

QString Delete_Row_Info::field_name() const
{
    if (field_i_ < 0 || field_i_ >= table_.field_names().size())
        return QString();
    return table_.field_names().at(field_i_);
}

QString Delete_Row_Info::pk_name() const
{
    if (pk_i_ < 0 || pk_i_ >= table_.field_names().size())
        return QString();
    return table_.field_names().at(pk_i_);
}

// ------------------------------------------------------------------------------------------------------------------

Delete_Row_Helper::Delete_Row_Helper(Base *obj, const QString& id, FILL_WHERE_FUNC_T fill_where_func) :
    obj_(obj), id_(id), fill_where_func_(std::move(fill_where_func)) {}

bool Delete_Row_Helper::del(const Delete_Row_Info& row_info)
{
    try
    {
        QString where;
        for (const Delete_Row_Info& del: row_info.childs_)
        {
            where = del.field_name() + '=' + id_;

            if (fill_where_func_)
                fill_where_func_(del, where);

            if (del.set_null_)
                exec_ok_ = obj_->update({del.table_name(), {}, { del.field_name() }}, { QVariant() }, where).isActive();
            else
            {
                del_impl(del);
                exec_ok_ = obj_->del(del.table_name(), where).isActive();
            }

            if (!exec_ok_)
                throw del;
        }

        where = row_info.pk_name() + '=' + id_;
        if (fill_where_func_)
            fill_where_func_(row_info, where);

        if (!obj_->del(row_info.table_name(), where).isActive())
            throw row_info;
    }
    catch(const Delete_Row_Info& info)
    {
        qCCritical(Helpz::DBLog()) << "DeleteHelper error" << row_info.table_name() << info.table_name() << info.field_name();
        return false;
    }
    return true;
}

void Delete_Row_Helper::del_impl(const Delete_Row_Info& del_parent)
{
    QString where;
    for (const Delete_Row_Info& del: del_parent.childs_)
    {
        del_impl(del);

        if (del.set_null_ && del.table_name() == del_parent.table_name())
            where = del_parent.pk_name() + '=' + id_;
        else
            where = del.field_name() + QString(" IN(SELECT %1 FROM %2 WHERE %3 = %4)")
                    .arg(del_parent.pk_name()).arg(del_parent.table_name()).arg(del_parent.field_name()).arg(id_);

        if (fill_where_func_)
            fill_where_func_(del, where);

        if (del.set_null_)
            exec_ok_ = obj_->update({del.table_name(), {}, { del.field_name() }}, { QVariant() }, where).isActive();
        else
            exec_ok_ = obj_->del(del.table_name(), where).isActive();

        if (!exec_ok_)
            throw del;
    }
}

} // namespace DB
} // namespace Helpz
