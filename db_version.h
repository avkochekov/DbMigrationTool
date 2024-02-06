#ifndef DB_VERSION_H
#define DB_VERSION_H

#include "DbMigrationTool_global.h"

struct DBMIGRATIONTOOL_EXPORT DbVersion {
    unsigned major = 0;
    unsigned minor = 0;
    unsigned update = 0;

    bool operator<(const DbVersion& other) const {
        if (this->major != other.major) {
            return this->major < other.major;
        }

        if (this->minor != other.minor) {
            return this->minor < other.minor;
        }

        return this->update < other.update;
    }

    bool operator==(const DbVersion& other) const {
        return this->major == other.major &&
               this->minor == other.minor &&
               this->update == other.update;
    }

    bool operator>(const DbVersion& other) const {
        return !(this->operator<(other)) && !(this->operator==(other));
    }
};

#endif // DB_VERSION_H
