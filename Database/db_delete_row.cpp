
#include "db_base.h"
#include "db_delete_row.h"

namespace Helpz {
namespace Database {

Delete_Row_Info::Delete_Row_Info(const QString& table_name, const QString& field_name, const std::vector<Delete_Row_Info>& childs, bool set_null, const QString& pk_name) :
    table_name_(table_name), field_name_(field_name), pk_name_(pk_name), childs_(childs), set_null_(set_null) {}

// ------------------------------------------------------------------------------------------------------------------

Delete_Row_Helper::Delete_Row_Helper(Base *obj, const QString& id) : obj_(obj), id_(id) {}

bool Delete_Row_Helper::del(const QString &table_name, const std::vector<Delete_Row_Info>& delete_rows_info, const QString &pk_name)
{
    try
    {
        QString where;
        for (const Delete_Row_Info& del: delete_rows_info)
        {
            where = del.field_name_ + '=' + id_;

            if (del.set_null_)
                exec_ok_ = obj_->update({del.table_name_, {}, { del.field_name_ }}, { QVariant() }, where);
            else
            {
                del_impl(del);
                exec_ok_ = obj_->del(del.table_name_, where).isActive();
            }

            if (!exec_ok_)
                throw del;
        }

        if (!obj_->del(table_name, pk_name + '=' + id_).isActive())
            throw Delete_Row_Info(table_name);
    }
    catch(const Delete_Row_Info& info)
    {
        qCCritical(Helpz::DBLog()) << "DeleteHelper error" << table_name << info.table_name_ << info.field_name_;
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

        if (del.set_null_ && del.table_name_ == del_parent.table_name_)
            where = del_parent.pk_name_ + '=' + id_;
        else
            where = del.field_name_ + QString(" IN(SELECT %1 FROM %2 WHERE %3 = %4)")
                    .arg(del_parent.pk_name_).arg(del_parent.table_name_).arg(del_parent.field_name_).arg(id_);

        if (del.set_null_)
            exec_ok_ = obj_->update({del.table_name_, {}, { del.field_name_ }}, { QVariant() }, where);
        else
            exec_ok_ = obj_->del(del.table_name_, where).isActive();

        if (!exec_ok_)
            throw del;
    }
}

} // namespace Database
} // namespace Helpz
