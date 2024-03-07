#include "db_migration_tool.h"

#include <QFile>
#include <QSqlError>
#include <QSqlResult>
#include <QUuid>
#include <QtSql/QSqlQuery>

#include "db_migration_exception.h"

static constexpr char* DB_INFO_TABLE = "dbMetaInfo";

static constexpr char* DB_INFO_COLUMN = "info";
static constexpr char* DB_DATA_COLUMN = "data";

static constexpr char* DB_VERSION_MAJOR = "v_major";
static constexpr char* DB_VERSION_MINOR = "v_minor";
static constexpr char* DB_VERSION_UPDATE = "v_update";

DbMigrationTool::DbMigrationTool(const QString &type, const QString &address, const QString &username, const QString &password)
    : m_firstInit{false}
{
    db = QSqlDatabase::addDatabase(type, QUuid::createUuid().toString(QUuid::StringFormat::Id128));
    db.setDatabaseName(address);

    if (username.isEmpty()) {
        return;
    }

    db.setUserName(username);
    db.setPassword(password);
}

bool DbMigrationTool::open()
{
    return db.open();
}

void DbMigrationTool::close()
{
    db.close();
}

void DbMigrationTool::update()
{
    QSqlError err;
    if (auto tables = db.tables(); tables.isEmpty()) {
        addMetaInfo();

        if (!runBaselineScripts(&err))
            throw DbMigrationException(QString("Running baseline script - failed: %1").arg(err.text()));

    } else if (!tables.contains(DB_INFO_TABLE)) {
        throw DbMigrationException("Checking database metainfo table - failed: There is no table with meta information");
    }

    if (!runMigrationScripts(&err))
        throw DbMigrationException(QString("Running migration scripts - failed: %1").arg(err.text()));
}

void DbMigrationTool::addMetaInfo()
{
    m_firstInit = true;

    QSqlError err;

    db.transaction();

    if (!addInfoTable(&err))
        throw DbMigrationException(QString("Adding database metainfo table - failed: %1").arg(err.text()));

    if (!addVersionRows(&err))
        throw DbMigrationException(QString("Adding database metainfo table row - failed: %1").arg(err.text()));

    db.commit();
}

bool tryOpenFile(const QString &path) {
    QFile f(path);

    if (!f.exists())
        return false;

    bool isOpened = f.open(QIODevice::ReadOnly);
    if (isOpened) {
        f.close();
    }
    return isOpened;
}

void DbMigrationTool::setBaselineScript(const QString &path)
{
    if (tryOpenFile(path)){
        m_baselinePath = path;
    } else {
        throw DbMigrationException(QString("Couldn't open file: %1").arg(path));
    }
}

void DbMigrationTool::addMigrationScript(const QString &path, const unsigned int major, const unsigned int minor, const unsigned int update)
{
    addMigrationScript(path, DbVersion{major, minor, update});
}

void DbMigrationTool::addMigrationScript(const QString &path, const DbVersion version)
{
    if (m_migrationPath.contains(version))
        throw DbMigrationException(QString("Version: %1").arg(path));

    if (tryOpenFile(path)){
        m_migrationPath.insert(version, path);
    } else {
        throw DbMigrationException(QString("Couldn't open file: %1").arg(path));
    }
}

bool DbMigrationTool::runBaselineScripts(QSqlError *err)
{
    QFile f(m_baselinePath);

    if (!f.exists())
        return false;

    if (auto queries = readQueries(m_baselinePath); queries) {
        db.transaction();
        for (const auto &script : *queries) {
            if (runScript(script, err))
                continue;

            db.rollback();
            return false;
        }
        db.commit();
    }
    return true;
}

bool DbMigrationTool::runMigrationScripts(QSqlError *err)
{
    auto iter = m_migrationPath.constBegin();
    const auto &version = getVersion();
    if (m_firstInit || version == DbVersion()) {
        // Do nothing
    } else {
        iter = m_migrationPath.constFind(version);
        if (iter != m_migrationPath.constEnd())
            iter++;
    }

    if (iter == m_migrationPath.constEnd()) {
        return true;
    }

    while (iter != m_migrationPath.constEnd()) {
        const auto &queries = readQueries(iter.value());
        if (queries) {
            db.transaction();
            for (const auto &script : *queries) {
                if (runScript(script, err)) {
                    continue;
                }
                db.rollback();
                return false;
            }
            if (!setVersion(iter.key(), err)) {
                db.rollback();
                return false;
            }
            db.commit();
            ++iter;
        } else {
            return false;
        }
    }
    db.commit();

    return setVersion(m_migrationPath.lastKey(), err);
}

std::optional<QStringList> DbMigrationTool::readQueries(const QString &path)
{
    QFile scriptFile(path);
    if (scriptFile.open(QIODevice::ReadOnly)) {
        QStringList list;
        // The SQLite driver executes only a single (the first) query in the QSqlQuery
        //  if the script contains more queries, it needs to be splitted.
        QStringList scriptQueries = QTextStream(&scriptFile).readAll().split(';');

        foreach (QString queryString, scriptQueries) {
            if (queryString.trimmed().isEmpty()) {
                continue;
            }
            list.append(queryString);
        }
        return list;
    }
    return std::nullopt;
}

bool DbMigrationTool::runScript(const QString &script, QSqlError* err)
{
    QSqlQuery query(db);
    if (!query.exec(script)) {
        qDebug() << query.lastError().text();
        if (err)
            *err = query.lastError();
        return false;
    }
    return true;
}

DbVersion DbMigrationTool::getVersion()
{
    auto fill = [&](const QString &version, unsigned int &value){
        if (auto data = getVersion(version); data) {
            value = *data;
        } else {
            throw DbMigrationException(QString("Gerring database version - failed: Couldn't read value \"%1\"").arg(version));
        }
    };

    DbVersion v;
    fill(DB_VERSION_MAJOR, v.major);
    fill(DB_VERSION_MINOR, v.minor);
    fill(DB_VERSION_UPDATE, v.update);
    return v;
}

std::optional<int> DbMigrationTool::getVersion(const QString &version)
{
    QSqlQuery query(db);
    query.prepare("SELECT * FROM " + QString(DB_INFO_TABLE) + " WHERE " + QString(DB_INFO_COLUMN) + " = :data");
    query.bindValue(":data", QString(version));
    if (query.exec() && query.next()) {
        bool isOk;
        int value = query.value(DB_DATA_COLUMN).toInt(&isOk);
        if (isOk)
            return value;
    }
    return std::nullopt;
}

bool DbMigrationTool::addInfoTable(QSqlError *err)
{
    QSqlQuery query(db);
    query.prepare("CREATE TABLE IF NOT EXISTS " + QString(DB_INFO_TABLE) + " (" + QString(DB_INFO_COLUMN) + " varchar(255)," + QString(DB_DATA_COLUMN) + " varchar(255));");
    if (!query.exec()) {
        if (err)
            *err = query.lastError();
        return false;
    }
    return true;
}

bool DbMigrationTool::addVersionRows(QSqlError *err)
{
    try {
        db.transaction();
        if (!addVersionRow(DB_VERSION_MAJOR, err))
            throw;
        if (!addVersionRow(DB_VERSION_MINOR, err))
            throw;
        if (!addVersionRow(DB_VERSION_UPDATE, err))
            throw;

        db.commit();
        return true;
    } catch (...) {
        db.rollback();
        return false;
    }
}

bool DbMigrationTool::addVersionRow(const QString &version, QSqlError *err)
{
    QSqlQuery query(db);
    query.prepare("INSERT INTO " + QString(DB_INFO_TABLE) + " (" + QString(DB_INFO_COLUMN) + ", " + QString(DB_DATA_COLUMN) + ") VALUES (:info, :data)");
    query.bindValue(":info",   version);
    query.bindValue(":data",   QVariant(0));

    if (!query.exec()) {
        if (err)
            *err = query.lastError();
        return false;
    }
    return true;
}

bool DbMigrationTool::setVersion(const DbVersion &v, QSqlError *err)
{
    try {
        db.transaction();
        if (!setVersion(DB_VERSION_MAJOR, v.major, err))
            throw;
        if (!setVersion(DB_VERSION_MINOR, v.minor, err))
            throw;
        if (!setVersion(DB_VERSION_UPDATE, v.update, err))
            throw;

        db.commit();
        return true;
    } catch (...) {
        db.rollback();
        return false;
    }
}

bool DbMigrationTool::setVersion(const QString &version, const int value, QSqlError* err)
{
    QSqlQuery query(db);
    query.prepare("UPDATE " + QString(DB_INFO_TABLE) + " SET " + QString(DB_DATA_COLUMN) + " = :data WHERE " + QString(DB_INFO_COLUMN) + "= :info");
    query.bindValue(":info",    QString(version));
    query.bindValue(":data",    value);

    if (!query.exec()) {
        if (err)
            *err = query.lastError();
        return false;
    }
    return true;
}
