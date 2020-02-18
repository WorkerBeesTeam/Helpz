#ifndef HELPZ_DATABASE_DELETE_ROW_H
#define HELPZ_DATABASE_DELETE_ROW_H

#include <vector>
#include <functional>

#include <QString>

namespace Helpz {
namespace DB {

class Delete_Row_Info
{
public:
    Delete_Row_Info(const QString& table_name, const QString& field_name = QString(),
                 const std::vector<Delete_Row_Info>& childs = {}, bool set_null = false, const QString& pk_name = "id");

    QString table_name_;
    QString field_name_;
    QString pk_name_;
    std::vector<Delete_Row_Info> childs_;
    bool set_null_;
};

typedef std::vector<Delete_Row_Info> Delete_Row_Info_List;

class Base;
class Delete_Row_Helper
{
public:
    using FILL_WHERE_FUNC_T = std::function<void(const Delete_Row_Info&, QString&)>;

    Delete_Row_Helper(Base* obj, const QString& id, FILL_WHERE_FUNC_T fill_where_func = nullptr);

    bool del(const QString& table_name, const std::vector<Delete_Row_Info>& delete_rows_info, const QString& pk_name = "id");

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
