module;
// Hacks to avoid pulling in the entire source code of sqlite_modern_cpp to remove `static` from one function
#define throw_sqlite_error throw_sqlite_error_
#include <sqlite_modern_cpp/type_wrapper.h>
#undef throw_sqlite_error
namespace sqlite::errors
{
void throw_sqlite_error(const int& error_code, str_ref sql, const char* errmsg)
{
    throw_sqlite_error_(error_code, sql, errmsg);
}
}
#include <sqlite_modern_cpp.h>

export module sqlite_modern_cpp;

export using ::sqlite3_interrupt;
export using ::sqlite3_progress_handler;

export namespace sqlite
{
using sqlite::operator++;
using sqlite::operator<<;
using sqlite::operator>>;
using sqlite::database;

namespace errors
{
using errors::busy;
using errors::interrupt;
}
}

export
{
using ::sqlite_uint64;
using ::sqlite_int64;
}
