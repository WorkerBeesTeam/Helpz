#include "db_table.h"

namespace Helpz {
namespace Database {

Table::Table(const QString& name, const QString& short_name, const QStringList& field_names) :
    name_(name), short_name_(short_name), field_names_(field_names)
{
}

bool Table::operator !() const {
    return name_.isEmpty() || field_names_.empty();
}

const QString& Table::name() const
{
    return name_;
}

void Table::set_name(const QString& name)
{
    name_ = name;
}

const QString& Table::short_name() const
{
    return short_name_;
}

void Table::set_short_name(const QString& short_name)
{
    short_name_ = short_name;
}

QStringList& Table::field_names()
{
    return field_names_;
}

const QStringList& Table::field_names() const
{
    return field_names_;
}

void Table::set_field_names(const QStringList& field_names)
{
    field_names_ = field_names;
}

} // namespace Database
} // namespace Helpz
