#ifndef DBMIGRATIONTOOL_H
#define DBMIGRATIONTOOL_H

#include "DbMigrationTool_global.h"
#include "db_version.h"

#include <QMap>
#include <QtSql/QSqlDatabase>

class DBMIGRATIONTOOL_EXPORT DbMigrationTool final
{
private:
    DbMigrationTool(DbMigrationTool &) = delete;
    DbMigrationTool(DbMigrationTool &&) = delete;
    DbMigrationTool operator=(DbMigrationTool &) = delete;
    DbMigrationTool operator=(DbMigrationTool &&) = delete;

public:
    DbMigrationTool(const QString &type, const QString &address, const QString& username = QString(), const QString &password = QString());

    bool open();
    void close();
    void update();
    void addMetaInfo();

    void setBaselineScript(const QString &path);
    void addMigrationScript(const QString &path, const unsigned int major, const unsigned int minor = 0, const unsigned int update = 0);
    void addMigrationScript(const QString &path, const DbVersion version);

private:
    bool runBaselineScripts(QSqlError *err = nullptr);
    bool runMigrationScripts(QSqlError *err);
    std::optional<QStringList> readQueries(const QString &path);
    bool runScript(const QString &script, QSqlError *err = nullptr);

    DbVersion getVersion();
    std::optional<int> getVersion(const QString& version);
    bool addInfoTable(QSqlError *err = nullptr);
    bool addVersionRows(QSqlError *err = nullptr);
    bool addVersionRow(const QString&, QSqlError *err = nullptr);
    bool setVersion(const DbVersion &v, QSqlError *err = nullptr);
    bool setVersion(const QString& version, const int value, QSqlError *err = nullptr);

private:
    QSqlDatabase db;

    QString m_baselinePath;
    QMap<DbVersion, QString> m_migrationPath;

    bool m_firstInit;
};

#endif // DBMIGRATIONTOOL_H
