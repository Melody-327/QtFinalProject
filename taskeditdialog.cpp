#include "taskeditdialog.h"
#include "ui_taskeditdialog.h"
#include <QMessageBox>    // 消息框
#include <QSqlDatabase>   // 数据库
#include <QSqlQuery>      // 数据库查询
#include <QSqlError>      // 数据库错误
#include <QDebug>         // 调试输出

// 任务编辑对话框构造函数
TaskEditDialog::TaskEditDialog(QWidget *parent, int taskId, const QSqlDatabase &parentDb)
    : QDialog(parent)
    , ui(new Ui::TaskEditDialog)
    , m_taskId(taskId)  // 任务ID（-1表示添加模式，>0表示编辑模式）
{
    ui->setupUi(this);      // 设置UI
    this->setModal(true);   // 设置为模态对话框

    qDebug() << "\n[TaskEditDialog] 构造函数开始，任务ID:" << taskId;

    // 1. 从父窗口获取数据库路径
    QString dbPath = parentDb.databaseName();
    qDebug() << "[TaskEditDialog] 父窗口数据库路径:" << dbPath;

    // 2. 创建自己的数据库连接（避免连接冲突）
    QString connectionName = QString("task_dialog_connection_%1").arg(quintptr(this));

    // 移除可能存在的旧连接
    if (QSqlDatabase::contains(connectionName)) {
        QSqlDatabase::removeDatabase(connectionName);
    }

    // 创建新的数据库连接
    m_db = QSqlDatabase::addDatabase("QSQLITE", connectionName);
    m_db.setDatabaseName(dbPath);  // 使用相同的数据库文件

    qDebug() << "[TaskEditDialog] 创建连接:" << connectionName << ", 路径:" << dbPath;

    // 尝试打开数据库
    if (!m_db.open()) {
        qDebug() << "[TaskEditDialog] 打开数据库失败:" << m_db.lastError().text();
        QMessageBox::critical(this, "数据库错误",
                              "无法连接到数据库。\n"
                              "路径: " + dbPath + "\n"
                                             "错误: " + m_db.lastError().text());
    } else {
        qDebug() << "[TaskEditDialog] 数据库连接成功";
    }

    // 清除UI设计器中的静态项目
    ui->comboCategory->clear();

    // 初始化下拉框
    initComboBox();

    // 如果数据库连接成功，加载数据
    if (m_db.isOpen()) {
        // 加载分类数据
        loadCategoryData();

        // 编辑模式：加载任务数据
        if (m_taskId != -1) {
            loadTaskData(m_taskId);
        } else {
            // 添加模式：设置默认值
            ui->dateEditDeadline->setDate(QDate::currentDate());  // 默认截止日期为今天

            // 默认状态为"未开始"
            int statusIndex = ui->comboStatus->findText("未开始");
            if (statusIndex >= 0) {
                ui->comboStatus->setCurrentIndex(statusIndex);
            }
        }
    } else {
        qDebug() << "[TaskEditDialog] 数据库连接失败，使用默认数据";

        // 数据库连接失败时，使用默认分类
        QStringList defaultCategories = {"工作", "生活", "学习", "健康", "社交"};
        for (int i = 0; i < defaultCategories.size(); i++) {
            ui->comboCategory->addItem(defaultCategories[i], i+1);  // 添加分类，分配临时ID
        }

        ui->comboCategory->setCurrentIndex(0);  // 默认选中第一个分类

        // 默认截止日期为今天
        ui->dateEditDeadline->setDate(QDate::currentDate());

        // 默认状态为"未开始"
        int statusIndex = ui->comboStatus->findText("未开始");
        if (statusIndex >= 0) {
            ui->comboStatus->setCurrentIndex(statusIndex);
        }
    }

    qDebug() << "[TaskEditDialog] 构造函数结束\n";
}

// 任务编辑对话框析构函数
TaskEditDialog::~TaskEditDialog()
{
    qDebug() << "[TaskEditDialog] 析构函数开始";

    // 关闭并清理数据库连接
    if (m_db.isOpen()) {
        QString connectionName = m_db.connectionName();
        m_db.close();
        QSqlDatabase::removeDatabase(connectionName);  // 移除数据库连接
        qDebug() << "[TaskEditDialog] 关闭并移除数据库连接:" << connectionName;
    }

    delete ui;
    qDebug() << "[TaskEditDialog] 析构函数结束";
}

// 初始化下拉框选项
void TaskEditDialog::initComboBox()
{
    qDebug() << "[TaskEditDialog] 初始化下拉框";

    // 初始化优先级下拉框
    ui->comboPriority->clear();
    ui->comboPriority->addItems({"高", "中", "低"});
    qDebug() << "[TaskEditDialog] 优先级下拉框初始化完成";

    // 初始化状态下拉框
    ui->comboStatus->clear();
    ui->comboStatus->addItems({"未开始", "进行中", "已完成"});
    qDebug() << "[TaskEditDialog] 状态下拉框初始化完成";
}

// 加载分类数据到下拉框
void TaskEditDialog::loadCategoryData()
{
    ui->comboCategory->clear();  // 清空现有分类

    qDebug() << "[TaskEditDialog] 开始从数据库加载分类...";

    if (!m_db.isOpen()) {
        qDebug() << "[TaskEditDialog] 数据库未打开，无法加载分类";
        return;
    }

    // 从数据库查询所有分类
    QSqlQuery query(m_db);
    if (!query.exec("SELECT id, name FROM category ORDER BY name")) {
        qDebug() << "[TaskEditDialog] 查询分类失败:" << query.lastError().text();
        qDebug() << "[TaskEditDialog] 执行的SQL:" << query.lastQuery();

        // 查询失败时添加默认分类
        QStringList defaultCategories = {"工作", "生活", "学习", "健康", "社交"};
        for (int i = 0; i < defaultCategories.size(); i++) {
            ui->comboCategory->addItem(defaultCategories[i], i+1);
        }
        return;
    }

    // 遍历查询结果，添加到下拉框
    int count = 0;
    while (query.next()) {
        count++;
        int id = query.value(0).toInt();        // 分类ID
        QString name = query.value(1).toString();  // 分类名称

        qDebug() << "[TaskEditDialog] 找到分类 #" << count << ": ID=" << id << ", name=" << name;

        ui->comboCategory->addItem(name, id);  // 添加分类，保存ID作为关联数据
    }

    qDebug() << "[TaskEditDialog] 共从数据库加载了" << count << "个分类";
    qDebug() << "[TaskEditDialog] 下拉框当前有" << ui->comboCategory->count() << "个项目";

    // 如果数据库分类数量不足，补充默认分类
    if (count < 5) {
        qDebug() << "[TaskEditDialog] 数据库分类数量不足，添加缺失的默认分类";
        QStringList allCategories = {"工作", "生活", "学习", "健康", "社交"};

        // 检查哪些分类已经存在
        for (const QString& category : allCategories) {
            bool exists = false;
            for (int i = 0; i < ui->comboCategory->count(); i++) {
                if (ui->comboCategory->itemText(i) == category) {
                    exists = true;
                    break;
                }
            }

            // 添加不存在的分类
            if (!exists) {
                qDebug() << "[TaskEditDialog] 添加缺失的分类:" << category;
                ui->comboCategory->addItem(category, ui->comboCategory->count() + 1);
            }
        }
    }

    // 默认选中第一个分类
    if (ui->comboCategory->count() > 0) {
        ui->comboCategory->setCurrentIndex(0);
        qDebug() << "[TaskEditDialog] 默认选中:" << ui->comboCategory->currentText();
    }
}

// 加载任务数据（编辑模式）
void TaskEditDialog::loadTaskData(int taskId)
{
    qDebug() << "[TaskEditDialog] 加载任务数据，任务ID:" << taskId;

    if (!m_db.isOpen()) {
        qDebug() << "[TaskEditDialog] 数据库未打开，无法加载任务数据";
        QMessageBox::critical(this, "错误", "数据库连接未打开，无法加载任务数据！");
        return;
    }

    // 查询指定ID的任务信息
    QSqlQuery query(m_db);
    query.prepare("SELECT t.name, t.desc, t.priority, t.deadline, t.status, t.category_id "
                  "FROM task t WHERE t.id = :id");
    query.bindValue(":id", taskId);

    if (!query.exec() || !query.next()) {
        qDebug() << "[TaskEditDialog] 加载任务数据失败:" << query.lastError().text();
        QMessageBox::critical(this, "错误", "加载任务数据失败！\n" + query.lastError().text());
        return;
    }

    // 填充数据到控件
    ui->lineEditTaskName->setText(query.value(0).toString());  // 任务名称
    ui->textEditDesc->setText(query.value(1).toString());       // 任务描述

    // 设置优先级
    int priorityIndex = ui->comboPriority->findText(query.value(2).toString());
    if (priorityIndex >= 0) {
        ui->comboPriority->setCurrentIndex(priorityIndex);
    } else {
        ui->comboPriority->setCurrentIndex(1); // 默认选中"中"
    }

    // 设置截止日期
    QString deadlineStr = query.value(3).toString();
    if (!deadlineStr.isEmpty()) {
        ui->dateEditDeadline->setDate(QDate::fromString(deadlineStr, "yyyy-MM-dd"));
    } else {
        ui->dateEditDeadline->setDate(QDate::currentDate());
    }

    // 设置状态
    int statusIndex = ui->comboStatus->findText(query.value(4).toString());
    if (statusIndex >= 0) {
        ui->comboStatus->setCurrentIndex(statusIndex);
    } else {
        ui->comboStatus->setCurrentIndex(0); // 默认选中"未开始"
    }

    // 设置分类
    int categoryId = query.value(5).toInt();
    bool foundCategory = false;
    for (int i = 0; i < ui->comboCategory->count(); i++) {
        if (ui->comboCategory->itemData(i).toInt() == categoryId) {
            ui->comboCategory->setCurrentIndex(i);
            foundCategory = true;
            break;
        }
    }

    // 如果没找到对应的分类，选中第一个
    if (!foundCategory && ui->comboCategory->count() > 0) {
        ui->comboCategory->setCurrentIndex(0);
    }

    qDebug() << "[TaskEditDialog] 任务数据加载完成";
}

// 保存任务数据
bool TaskEditDialog::saveTaskData()
{
    qDebug() << "[TaskEditDialog] 保存任务数据...";

    // 数据验证：任务名称不能为空
    QString taskName = ui->lineEditTaskName->text().trimmed();
    if (taskName.isEmpty()) {
        QMessageBox::warning(this, "提示", "任务名称不能为空！");
        return false;
    }

    // 检查数据库连接状态
    if (!m_db.isOpen()) {
        qDebug() << "[TaskEditDialog] 数据库未打开，尝试重新连接";

        // 尝试重新连接数据库
        m_db = QSqlDatabase::addDatabase("QSQLITE",
                                         QString("task_save_reconnect_%1").arg(quintptr(this)));
        m_db.setDatabaseName("FinalLab.db");

        if (!m_db.open()) {
            qDebug() << "[TaskEditDialog] 重新连接数据库失败:" << m_db.lastError().text();
            QMessageBox::critical(this, "数据库错误",
                                  "无法连接到数据库。\n"
                                  "错误信息: " + m_db.lastError().text());
            return false;
        }
    }

    // 获取分类信息
    QString categoryName = ui->comboCategory->currentText();
    int categoryId = ui->comboCategory->currentData().toInt();

    qDebug() << "[TaskEditDialog] 任务信息:";
    qDebug() << "  名称:" << taskName;
    qDebug() << "  分类:" << categoryName << "(ID:" << categoryId << ")";

    // 如果分类ID无效，尝试从数据库获取或创建
    if (categoryId <= 0) {
        qDebug() << "[TaskEditDialog] 分类ID无效，从数据库查询...";

        QSqlQuery findQuery(m_db);
        findQuery.prepare("SELECT id FROM category WHERE name = :name");
        findQuery.bindValue(":name", categoryName);

        // 查询分类ID
        if (findQuery.exec() && findQuery.next()) {
            categoryId = findQuery.value(0).toInt();
            qDebug() << "[TaskEditDialog] 找到分类ID:" << categoryId;
        } else {
            qDebug() << "[TaskEditDialog] 分类不存在，创建新分类...";

            // 分类不存在，插入新分类
            findQuery.prepare("INSERT INTO category (name) VALUES (:name)");
            findQuery.bindValue(":name", categoryName);

            if (findQuery.exec()) {
                categoryId = findQuery.lastInsertId().toInt();  // 获取新插入的分类ID
                qDebug() << "[TaskEditDialog] 创建新分类成功，ID:" << categoryId;
            } else {
                qDebug() << "[TaskEditDialog] 创建分类失败:" << findQuery.lastError().text();
                QMessageBox::warning(this, "错误", "创建分类失败！\n" + findQuery.lastError().text());
                return false;
            }
        }
    }

    // 获取其他控件数据
    QString desc = ui->textEditDesc->toPlainText().trimmed();
    QString priority = ui->comboPriority->currentText();
    QString deadline = ui->dateEditDeadline->date().toString("yyyy-MM-dd");
    QString status = ui->comboStatus->currentText();

    qDebug() << "[TaskEditDialog] 其他信息:";
    qDebug() << "  描述:" << desc;
    qDebug() << "  优先级:" << priority;
    qDebug() << "  截止日期:" << deadline;
    qDebug() << "  状态:" << status;

    // 创建SQL查询
    QSqlQuery query(m_db);
    if (m_taskId == -1) {
        // 添加模式：插入新任务
        qDebug() << "[TaskEditDialog] 添加新任务...";
        query.prepare("INSERT INTO task (name, desc, priority, deadline, status, category_id) "
                      "VALUES (:name, :desc, :priority, :deadline, :status, :category_id)");
    } else {
        // 编辑模式：更新任务
        qDebug() << "[TaskEditDialog] 更新任务，ID:" << m_taskId;
        query.prepare("UPDATE task SET name=:name, desc=:desc, priority=:priority, "
                      "deadline=:deadline, status=:status, category_id=:category_id WHERE id=:id");
        query.bindValue(":id", m_taskId);
    }

    // 绑定参数（防止SQL注入）
    query.bindValue(":name", taskName);
    query.bindValue(":desc", desc);
    query.bindValue(":priority", priority);
    query.bindValue(":deadline", deadline);
    query.bindValue(":status", status);
    query.bindValue(":category_id", categoryId);

    // 执行SQL
    if (!query.exec()) {
        qDebug() << "[TaskEditDialog] 保存任务失败:" << query.lastError().text();
        QMessageBox::critical(this, "错误", "保存任务失败！\n" + query.lastError().text());
        return false;
    }

    qDebug() << "[TaskEditDialog] 保存任务成功!";
    return true;
}

// 确认按钮点击事件
void TaskEditDialog::on_btnConfirm_clicked()
{
    qDebug() << "[TaskEditDialog] 点击确认按钮";

    if (saveTaskData()) {
        qDebug() << "[TaskEditDialog] 保存成功，关闭对话框";
        this->accept();  // 关闭对话框并返回Accepted
    } else {
        qDebug() << "[TaskEditDialog] 保存失败";
    }
}

// 取消按钮点击事件
void TaskEditDialog::on_btnCancel_clicked()
{
    qDebug() << "[TaskEditDialog] 点击取消按钮";
    this->reject();  // 关闭对话框并返回Rejected
}
