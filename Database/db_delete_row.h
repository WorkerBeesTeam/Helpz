#ifndef HELPZ_DATABASE_DELETE_ROW_H
#define HELPZ_DATABASE_DELETE_ROW_H

#include <vector>
#include <functional>

#include <QString>

#include <Helpz/db_table.h>

namespace Helpz {
namespace DB {

class Delete_Row_Info
{
public:
    Delete_Row_Info(const Table& table, int field_index,
                 const std::vector<Delete_Row_Info>& childs = {}, bool set_null = false, int pk_index = 0);

    QString table_name() const;
    QString field_name() const;
    QString pk_name() const;

    bool set_null_;
    int field_i_, pk_i_;

    Table table_;
    std::vector<Delete_Row_Info> childs_;
};

typedef std::vector<Delete_Row_Info> Delete_Row_Info_List;

class Base;
class Delete_Row_Helper
{
public:
    using FILL_WHERE_FUNC_T = std::function<void(const Delete_Row_Info&, QString&)>;

    Delete_Row_Helper(Base* obj, const QString& id, FILL_WHERE_FUNC_T fill_where_func = nullptr);

    bool del(const Delete_Row_Info& row_info);

private:
    void del_impl(const Delete_Row_Info& del_parent);

    bool exec_ok_;

    Base* obj_;
    QString id_;

    FILL_WHERE_FUNC_T fill_where_func_;
};

} // namespace DB
} // namespace Helpz

#endif // HELPZ_DATABASE_DELETE_ROW_H
