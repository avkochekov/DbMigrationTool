#include "db_migration_exception.h"

static constexpr char const* empty_message = "";

DbMigrationException::DbMigrationException(const QString& message)
    : m_message(std::make_shared<std::string>(message.toStdString()))
{}

const char *DbMigrationException::what() const throw()
{
    return m_message ? m_message->c_str() : empty_message;
}
