#include "remindermanager.h"
#include <QDebug>
#include <QSqlQuery>
#include <QDateTime>
#include <QMessageBox>
#include <QApplication>

ReminderManager::ReminderManager(QObject *parent)
    : QObject(parent)
    , m_timer(new QTimer(this))
    , m_trayIcon(new QSystemTrayIcon(this))
{
    // 设置系统托盘图标
    // 如果没有图标资源，可以创建一个简单的图标
    if (!m_trayIcon->isSystemTrayAvailable()) {
        qDebug() << "ReminderManager: System tray not available";
    }

    connect(m_timer, &QTimer::timeout, this, &ReminderManager::checkReminders);
}

ReminderManager::~ReminderManager()
{
    stopChecking();
}

void ReminderManager::setDatabase(const QSqlDatabase &db)
{
    // 使用相同的数据库路径创建新连接
    QString connectionName = QString("reminder_connection_%1").arg(reinterpret_cast<quintptr>(this));

    if (QSqlDatabase::contains(connectionName)) {
        QSqlDatabase::removeDatabase(connectionName);
    }

    m_db = QSqlDatabase::addDatabase("QSQLITE", connectionName);
    m_db.setDatabaseName(db.databaseName());

    if (!m_db.open()) {
        qDebug() << "ReminderManager: Failed to open database:" << m_db.lastError().text();
    } else {
        qDebug() << "ReminderManager: Database opened successfully";
    }
}

void ReminderManager::startChecking()
{
    // 每分钟检查一次（60000毫秒）
    m_timer->start(60000);
    qDebug() << "ReminderManager: Started checking reminders";

    // 立即检查一次
    checkReminders();
}

void ReminderManager::stopChecking()
{
    m_timer->stop();
}

void ReminderManager::checkReminders()
{
    if (!m_db.isOpen()) {
        qDebug() << "ReminderManager: Database not open";
        return;
    }

    QDateTime now = QDateTime::currentDateTime();
    QString currentDate = now.toString("yyyy-MM-dd");

    // 检查今天到期的任务
    QSqlQuery query(m_db);
    query.prepare("SELECT name, deadline, priority FROM task "
                  "WHERE status != '已完成' "
                  "AND deadline IS NOT NULL "
                  "AND deadline = :today "
                  "ORDER BY priority DESC");
    query.bindValue(":today", currentDate);

    bool hasReminder = false;

    if (query.exec()) {
        while (query.next()) {
            hasReminder = true;
            QString taskName = query.value(0).toString();
            QString deadline = query.value(1).toString();
            QString priority = query.value(2).toString();

            QString message = QString("任务【%1】今天到期！\n优先级：%2")
                                  .arg(taskName)
                                  .arg(priority);

            showReminder("任务到期提醒", message);
        }
    } else {
        qDebug() << "ReminderManager: Query failed:" << query.lastError().text();
    }

    // 检查即将到期的任务（未来3天内）
    QDate today = QDate::currentDate();
    QDate threeDaysLater = today.addDays(3);

    QSqlQuery upcomingQuery(m_db);
    upcomingQuery.prepare("SELECT name, deadline, priority FROM task "
                          "WHERE status != '已完成' "
                          "AND deadline IS NOT NULL "
                          "AND deadline BETWEEN :tomorrow AND :threeDaysLater "
                          "ORDER BY deadline");
    upcomingQuery.bindValue(":tomorrow", today.addDays(1).toString("yyyy-MM-dd"));
    upcomingQuery.bindValue(":threeDaysLater", threeDaysLater.toString("yyyy-MM-dd"));

    if (upcomingQuery.exec()) {
        while (upcomingQuery.next()) {
            hasReminder = true;
            QString taskName = upcomingQuery.value(0).toString();
            QString deadline = upcomingQuery.value(1).toString();
            QString priority = upcomingQuery.value(2).toString();

            QDate deadlineDate = QDate::fromString(deadline, "yyyy-MM-dd");
            int daysLeft = today.daysTo(deadlineDate);

            QString message = QString("任务【%1】%2天后到期\n优先级：%3")
                                  .arg(taskName)
                                  .arg(daysLeft)
                                  .arg(priority);

            showReminder("任务即将到期提醒", message);
        }
    }

    // 如果需要发射信号，可以在这里添加
    if (hasReminder) {
        emit reminderTriggered("提醒检查", "已检查到待处理任务");
    }
}

void ReminderManager::showReminder(const QString &title, const QString &message)
{
    qDebug() << "Reminder:" << title << "-" << message;

    // 显示系统托盘提示（如果可用）
    if (m_trayIcon->isSystemTrayAvailable()) {
        m_trayIcon->showMessage(title, message, QSystemTrayIcon::Information, 5000);
    } else {
        // 系统托盘不可用时，使用控制台输出
        qDebug() << "System Tray Notification:" << title << ":" << message;
    }
}
