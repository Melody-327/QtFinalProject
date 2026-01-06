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

// 确认按钮
void TaskEditDialog::on_btnConfirm_clicked()
{
    qDebug() << "[TaskEditDialog] 点击确认按钮";

    if (saveTaskData()) {
        qDebug() << "[TaskEditDialog] 保存成功，关闭对话框";
        this->accept(); // 关闭弹窗并返回成功
    } else {
        qDebug() << "[TaskEditDialog] 保存失败";
    }
}

// 取消按钮
void TaskEditDialog::on_btnCancel_clicked()
{
    qDebug() << "[TaskEditDialog] 点击取消按钮";
    this->reject(); // 关闭弹窗并返回取消
}
