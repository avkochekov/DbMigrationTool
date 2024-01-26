#ifndef DBMIGRATIONEXCEPTION_H
#define DBMIGRATIONEXCEPTION_H

#include "DbMigrationTool_global.h"
#include <memory>
#include <string>

#include <QString>

class DBMIGRATIONTOOL_EXPORT DbMigrationException
{
public:
    DbMigrationException() = default;

    explicit DbMigrationException(const QString& messagge);

public:
    const char* what() const throw();

private:
    std::shared_ptr<std::string> m_message {};
};

#endif // DBMIGRATIONEXCEPTION_H
