#ifndef REMINDERMANAGER_H
#define REMINDERMANAGER_H

#include <QObject>
#include <QTimer>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSystemTrayIcon>

class ReminderManager : public QObject
{
    Q_OBJECT

public:
    explicit ReminderManager(QObject *parent = nullptr);
    ~ReminderManager();

    void startChecking();
    void stopChecking();
    void setDatabase(const QSqlDatabase &db);

signals:  // 添加信号声明
    void reminderTriggered(const QString &title, const QString &message);

private slots:
    void checkReminders();

private:
    QTimer *m_timer;
    QSqlDatabase m_db;
    QSystemTrayIcon *m_trayIcon;

    void showReminder(const QString &title, const QString &message);
};

#endif // REMINDERMANAGER_H
