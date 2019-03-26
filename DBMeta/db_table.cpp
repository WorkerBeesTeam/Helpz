#include "db_table.h"

namespace Helpz {
namespace Database {

bool Table::operator !() const {
    return name_.isEmpty() || field_names_.empty();
}

} // namespace Database
} // namespace Helpz
