#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "taskeditdialog.h"
#include <QInputDialog>
#include <QFileDialog>
#include <QTextStream>
#include <QDir>
#include <QDebug>

// 主窗口构造函数
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , m_taskModel(nullptr)        // 任务数据模型
    , m_proxyModel(nullptr)       // 代理模型（用于排序和过滤）
    , m_reminderManager(nullptr)  // 提醒管理器
{
    qDebug() << "\n[MainWindow] 构造函数开始";

    try {
        // 设置UI界面
        ui->setupUi(this);
        this->setWindowTitle("任务管理系统 - 个人工作与任务管理");

        // 初始化提醒管理器
        m_reminderManager = new ReminderManager(this);

        // 初始化数据库
        initDatabase();

        // 打开数据库连接
        if (openDatabase()) {
            // 设置提醒管理器的数据库连接
            m_reminderManager->setDatabase(db);
            m_reminderManager->startChecking();  // 开始检查提醒

            // 检查数据库连接
            checkDatabaseConnection();
            // 创建数据表（如果不存在）
            createTables();
            // 初始化过滤器下拉框
            initFilterComboBox();

            // 创建任务数据模型
            m_taskModel = new QSqlTableModel(this, db);
            m_taskModel->setTable("task");  // 设置操作的表名
            m_taskModel->setEditStrategy(QSqlTableModel::OnManualSubmit);  // 手动提交编辑

            // 创建代理模型用于排序和过滤
            m_proxyModel = new QSortFilterProxyModel(this);
            m_proxyModel->setSourceModel(m_taskModel);  // 设置源模型
            m_proxyModel->setFilterCaseSensitivity(Qt::CaseInsensitive);  // 过滤不区分大小写

            // 设置表格视图的模型
            ui->tableViewTask->setModel(m_proxyModel);

            // 设置表头显示文本
            m_taskModel->setHeaderData(m_taskModel->fieldIndex("id"), Qt::Horizontal, "ID");
            m_taskModel->setHeaderData(m_taskModel->fieldIndex("name"), Qt::Horizontal, "任务名称");
            m_taskModel->setHeaderData(m_taskModel->fieldIndex("priority"), Qt::Horizontal, "优先级");
            m_taskModel->setHeaderData(m_taskModel->fieldIndex("deadline"), Qt::Horizontal, "截止日期");
            m_taskModel->setHeaderData(m_taskModel->fieldIndex("status"), Qt::Horizontal, "状态");
            m_taskModel->setHeaderData(m_taskModel->fieldIndex("desc"), Qt::Horizontal, "描述");

            // 隐藏分类ID列（用户不需要看到）
            int categoryIdCol = m_taskModel->fieldIndex("category_id");
            if (categoryIdCol >= 0) {
                ui->tableViewTask->hideColumn(categoryIdCol);
            }

            // 设置表格属性
            ui->tableViewTask->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);  // 列自适应宽度
            ui->tableViewTask->setEditTriggers(QAbstractItemView::NoEditTriggers);  // 禁止直接编辑

            // 加载任务数据
            loadTaskData();
            // 更新任务统计信息
            updateTaskStats();

        } else {
            // 数据库连接失败，显示错误信息
            QMessageBox::critical(this, "数据库错误",
                                  "连接FinalLab.db失败！\n错误信息：" + db.lastError().text());
        }

    } catch (...) {
        // 处理初始化过程中的异常
        QMessageBox::critical(this, "初始化错误", "程序初始化时发生异常");
    }
}

// 主窗口析构函数
MainWindow::~MainWindow()
{
    // 清理资源
    if (m_reminderManager) {
        m_reminderManager->stopChecking();  // 停止提醒检查
        delete m_reminderManager;
    }
    if (m_proxyModel) delete m_proxyModel;
    if (m_taskModel) delete m_taskModel;
    delete ui;
    closeDatabase();  // 关闭数据库连接
}

// 初始化数据库连接
void MainWindow::initDatabase()
{
    // 设置数据库文件路径
    QString dbPath = "D:/Qt/FinalLab.db";  // 首选路径
    QFileInfo dbFile(dbPath);

    // 如果首选路径不存在，使用当前目录
    if (!dbFile.exists()) {
        dbPath = QDir::currentPath() + "/FinalLab.db";
        dbFile.setFile(dbPath);
    }

    qDebug() << "[MainWindow] 数据库路径:" << dbFile.absoluteFilePath();

    // 创建唯一的数据库连接名称（避免冲突）
    static int connectionCount = 0;
    QString connectionName = QString("task_management_connection_%1").arg(++connectionCount);

    // 移除可能存在的同名连接
    if (QSqlDatabase::contains(connectionName)) {
        QSqlDatabase::removeDatabase(connectionName);
    }

    // 添加SQLite数据库连接
    db = QSqlDatabase::addDatabase("QSQLITE", connectionName);
    db.setDatabaseName(dbFile.absoluteFilePath());  // 设置数据库文件路径
}

// 打开数据库连接
bool MainWindow::openDatabase()
{
    if (db.open()) {
        qDebug() << "[MainWindow] 数据库打开成功";
        return true;
    } else {
        qDebug() << "[MainWindow] 数据库打开失败:" << db.lastError().text();
        return false;
    }
}

// 关闭数据库连接
void MainWindow::closeDatabase()
{
    if (db.isOpen()) {
        db.close();
    }
}

// 创建数据表（如果不存在）
void MainWindow::createTables()
{
    QSqlQuery query(db);

    // 创建分类表
    QString createCategorySql = "CREATE TABLE IF NOT EXISTS category ("
                                "id INTEGER PRIMARY KEY AUTOINCREMENT,"  // 自增主键
                                "name TEXT NOT NULL UNIQUE)";  // 分类名称，唯一约束
    query.exec(createCategorySql);

    // 创建任务表
    QString createTaskSql = "CREATE TABLE IF NOT EXISTS task ("
                            "id INTEGER PRIMARY KEY AUTOINCREMENT,"      // 自增主键
                            "name TEXT NOT NULL,"                        // 任务名称
                            "desc TEXT,"                                 // 任务描述
                            "priority TEXT NOT NULL,"                    // 优先级
                            "deadline TEXT,"                             // 截止日期
                            "status TEXT NOT NULL,"                      // 状态
                            "category_id INTEGER)";                      // 分类ID（外键）
    query.exec(createTaskSql);

    // 插入默认分类数据
    QStringList defaultCategories = {"工作", "生活", "学习", "健康", "社交"};
    for (const QString &category : defaultCategories) {
        // 使用INSERT OR IGNORE避免重复插入
        query.exec(QString("INSERT OR IGNORE INTO category (name) VALUES ('%1')").arg(category));
    }
}

// 检查数据库连接详情
void MainWindow::checkDatabaseConnection()
{
    if (!db.isOpen()) return;

    QSqlQuery checkQuery(db);
    // 使用PRAGMA命令获取数据库信息
    if (checkQuery.exec("PRAGMA database_list")) {
        qDebug() << "[MainWindow] 数据库连接详情:";
        while (checkQuery.next()) {
            QString file = checkQuery.value(2).toString();  // 获取数据库文件路径
            qDebug() << "  文件:" << file;
        }
    }
}

// 初始化过滤器下拉框
void MainWindow::initFilterComboBox()
{
    // 初始化优先级过滤器
    ui->comboPriorityFilter->clear();
    ui->comboPriorityFilter->addItems({"全部", "高", "中", "低"});

    // 初始化状态过滤器
    ui->comboStatusFilter->clear();
    ui->comboStatusFilter->addItems({"全部", "未开始", "进行中", "已完成"});
}

// 加载任务数据
void MainWindow::loadTaskData()
{
    if (!m_taskModel || !db.isOpen()) return;

    // 获取当前过滤条件
    QString priorityFilter = ui->comboPriorityFilter->currentText();
    QString statusFilter = ui->comboStatusFilter->currentText();

    QString filterStr;  // 过滤条件字符串

    // 构建优先级过滤条件
    if (priorityFilter != "全部" && !priorityFilter.isEmpty()) {
        filterStr = QString("priority = '%1'").arg(priorityFilter);
    }

    // 构建状态过滤条件
    if (statusFilter != "全部" && !statusFilter.isEmpty()) {
        if (!filterStr.isEmpty()) filterStr += " AND ";
        filterStr += QString("status = '%1'").arg(statusFilter);
    }

    // 应用过滤条件到模型
    if (!filterStr.isEmpty()) {
        m_taskModel->setFilter(filterStr);
    } else {
        m_taskModel->setFilter("");  // 清空过滤条件，显示所有数据
    }

    m_taskModel->select();  // 从数据库重新加载数据
    updateTaskStats();      // 更新统计信息
}

// 获取当前选中任务的ID
int MainWindow::getCurrentTaskId()
{
    QModelIndex currentIndex = ui->tableViewTask->currentIndex();
    if (!currentIndex.isValid()) return -1;  // 没有选中行

    // 将代理模型索引转换为源模型索引
    QModelIndex sourceIndex = m_proxyModel->mapToSource(currentIndex);
    int idCol = m_taskModel->fieldIndex("id");  // 获取ID列的索引
    return m_taskModel->data(m_taskModel->index(sourceIndex.row(), idCol)).toInt();  // 返回ID值
}

// 更新任务统计信息
void MainWindow::updateTaskStats()
{
    if (!db.isOpen()) return;

    // 获取当前过滤条件
    QString priorityFilter = ui->comboPriorityFilter->currentText();
    QString statusFilter = ui->comboStatusFilter->currentText();

    // 构建基础WHERE子句
    QString baseWhereClause = "";
    if (priorityFilter != "全部" && !priorityFilter.isEmpty()) {
        baseWhereClause = QString("priority='%1'").arg(priorityFilter);
    }
    if (statusFilter != "全部" && !statusFilter.isEmpty()) {
        if (!baseWhereClause.isEmpty()) baseWhereClause += " AND ";
        baseWhereClause += QString("status='%1'").arg(statusFilter);
    }

    // 查询总任务数
    QString totalQueryStr = "SELECT COUNT(*) FROM task";
    if (!baseWhereClause.isEmpty()) totalQueryStr += " WHERE " + baseWhereClause;

    QSqlQuery totalQuery(db);
    int totalTasks = 0;
    if (totalQuery.exec(totalQueryStr) && totalQuery.next()) {
        totalTasks = totalQuery.value(0).toInt();
    }

    // 查询已完成任务数
    QString completedQueryStr = "SELECT COUNT(*) FROM task WHERE status='已完成'";
    if (!baseWhereClause.isEmpty()) {
        completedQueryStr = "SELECT COUNT(*) FROM task WHERE " + baseWhereClause + " AND status='已完成'";
    }

    QSqlQuery completedQuery(db);
    int completedTasks = 0;
    if (completedQuery.exec(completedQueryStr) && completedQuery.next()) {
        completedTasks = completedQuery.value(0).toInt();
    }

    // 查询进行中任务数
    QString inProgressQueryStr = "SELECT COUNT(*) FROM task WHERE status='进行中'";
    if (!baseWhereClause.isEmpty()) {
        inProgressQueryStr = "SELECT COUNT(*) FROM task WHERE " + baseWhereClause + " AND status='进行中'";
    }

    QSqlQuery inProgressQuery(db);
    int inProgressTasks = 0;
    if (inProgressQuery.exec(inProgressQueryStr) && inProgressQuery.next()) {
        inProgressTasks = inProgressQuery.value(0).toInt();
    }

    // 查询未开始任务数
    QString pendingQueryStr = "SELECT COUNT(*) FROM task WHERE status='未开始'";
    if (!baseWhereClause.isEmpty()) {
        pendingQueryStr = "SELECT COUNT(*) FROM task WHERE " + baseWhereClause + " AND status='未开始'";
    }

    QSqlQuery pendingQuery(db);
    int pendingTasks = 0;
    if (pendingQuery.exec(pendingQueryStr) && pendingQuery.next()) {
        pendingTasks = pendingQuery.value(0).toInt();
    }

    // 格式化统计信息文本
    QString statsText = QString("总计: %1 | 已完成: %2 | 进行中: %3 | 未开始: %4")
                            .arg(totalTasks)
                            .arg(completedTasks)
                            .arg(inProgressTasks)
                            .arg(pendingTasks);

    // 更新状态标签显示
    if (ui->labelStatusInfo) {
        ui->labelStatusInfo->setText(statsText);
    }
}

// 获取特定状态和优先级的任务数量（辅助函数）
int MainWindow::getPriorityCount(const QString& status, const QString& priority)
{
    QString queryStr = QString("SELECT COUNT(*) FROM task WHERE status='%1' AND priority='%2'")
    .arg(status).arg(priority);
    QSqlQuery query(db);
    if (query.exec(queryStr) && query.next()) {
        return query.value(0).toInt();
    }
    return 0;
}

// 添加任务按钮点击事件
void MainWindow::on_btnAddTask_clicked()
{
    // 创建任务编辑对话框（taskId=-1表示添加模式）
    TaskEditDialog dlg(this, -1, db);
    dlg.setWindowTitle("添加任务");
    if (dlg.exec() == QDialog::Accepted) {  // 显示模态对话框
        loadTaskData();  // 重新加载数据
        QMessageBox::information(this, "成功", "任务添加成功！");
    }
}

// 编辑任务按钮点击事件
void MainWindow::on_btnEditTask_clicked()
{
    int taskId = getCurrentTaskId();
    if (taskId < 0) {
        QMessageBox::warning(this, "提示", "请先选中要编辑的任务！");
        return;
    }

    // 创建任务编辑对话框（taskId>0表示编辑模式）
    TaskEditDialog dlg(this, taskId, db);
    dlg.setWindowTitle("编辑任务");
    if (dlg.exec() == QDialog::Accepted) {
        loadTaskData();  // 重新加载数据
        QMessageBox::information(this, "成功", "任务修改成功！");
    }
}

// 删除任务按钮点击事件
void MainWindow::on_btnDeleteTask_clicked()
{
    int taskId = getCurrentTaskId();
    if (taskId < 0) {
        QMessageBox::warning(this, "提示", "请先选中要删除的任务！");
        return;
    }

    // 确认删除对话框
    if (QMessageBox::question(this, "确认", "确定删除该任务吗？",
                              QMessageBox::Yes|QMessageBox::No) != QMessageBox::Yes) {
        return;
    }

    // 执行删除操作
    QSqlQuery query(db);
    query.prepare("DELETE FROM task WHERE id = :id");  // 使用参数化查询防止SQL注入
    query.bindValue(":id", taskId);

    if (query.exec()) {
        loadTaskData();  // 重新加载数据
        QMessageBox::information(this, "成功", "任务删除成功！");
    } else {
        QMessageBox::critical(this, "失败", "删除失败：" + query.lastError().text());
    }
}

// 刷新按钮点击事件
void MainWindow::on_btnRefresh_clicked()
{
    loadTaskData();  // 重新加载数据
    QMessageBox::information(this, "提示", "数据已刷新！");
}

// 导出按钮点击事件
void MainWindow::on_btnExport_clicked()
{
    // 创建导出菜单
    QMenu exportMenu;
    exportMenu.addAction("导出为CSV", this, &MainWindow::exportToCsv);  // CSV导出选项
    exportMenu.addAction("导出为PDF", this, &MainWindow::exportToPdf);  // PDF导出选项

    // 在按钮位置显示下拉菜单
    exportMenu.exec(ui->btnExport->mapToGlobal(QPoint(0, ui->btnExport->height())));
}

// 导出为CSV文件
void MainWindow::exportToCsv()
{
    // 设置默认文件名（包含当前日期）
    QString defaultFileName = QString("任务数据_%1.csv").arg(QDate::currentDate().toString("yyyyMMdd"));
    QString fileName = QFileDialog::getSaveFileName(this, "导出CSV文件", defaultFileName, "CSV文件 (*.csv)");

    if (fileName.isEmpty()) return;  // 用户取消操作

    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::warning(this, "错误", "无法创建文件：" + file.errorString());
        return;
    }

    QTextStream out(&file);

    // 设置编码（兼容Qt5和Qt6）
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    out.setCodec("UTF-8");
#else
    out.setEncoding(QStringConverter::Utf8);
#endif

    // 写入UTF-8 BOM（使Excel能正确识别中文编码）
    out << "\xEF\xBB\xBF";

    // 写入CSV表头
    out << "ID,任务名称,分类,优先级,截止日期,状态,描述\n";

    // 查询任务数据（包含分类信息）
    QSqlQuery query(db);
    QString sql = "SELECT t.id, t.name, c.name as category, t.priority, t.deadline, t.status, t.desc "
                  "FROM task t LEFT JOIN category c ON t.category_id = c.id ORDER BY t.id";

    if (!query.exec(sql)) {
        QMessageBox::critical(this, "错误", "查询数据失败：" + query.lastError().text());
        file.close();
        return;
    }

    // 写入数据行
    int recordCount = 0;
    while (query.next()) {
        QStringList row;
        for (int i = 0; i < 7; i++) {
            QString value = query.value(i).toString();
            // CSV特殊字符处理：逗号、引号、换行符需要转义
            if (value.contains(',') || value.contains('"') || value.contains('\n')) {
                value = "\"" + value.replace("\"", "\"\"") + "\"";  // 双引号转义
            }
            row << value;
        }
        out << row.join(",") << "\n";  // 用逗号连接字段
        recordCount++;
    }

    file.close();

    // 显示导出结果信息
    QString resultMessage = QString("数据导出成功！\n\n"
                                    "文件：%1\n"
                                    "记录数：%2 条")
                                .arg(fileName)
                                .arg(recordCount);

    QMessageBox::information(this, "导出成功", resultMessage);
}

// 导出为PDF文件
void MainWindow::exportToPdf()
{
    // 设置默认文件名
    QString defaultFileName = QString("任务数据_%1.pdf").arg(QDate::currentDate().toString("yyyyMMdd"));
    QString fileName = QFileDialog::getSaveFileName(this, "导出PDF文件", defaultFileName, "PDF文件 (*.pdf)");

    if (fileName.isEmpty()) return;

    // 创建打印机对象（用于生成PDF）
    QPrinter printer;

    // 设置打印机属性（兼容Qt5和Qt6）
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    printer.setOutputFormat(QPrinter::PdfFormat);  // 输出格式为PDF
    printer.setOutputFileName(fileName);          // 输出文件名
    printer.setPageSize(QPrinter::A4);            // 页面大小A4
    printer.setPageOrientation(QPrinter::Portrait);  // 纵向页面
#else
    printer.setOutputFormat(QPrinter::PdfFormat);
    printer.setOutputFileName(fileName);
    printer.setPageSize(QPageSize(QPageSize::A4));
    printer.setPageOrientation(QPageLayout::Portrait);
#endif

    // 创建绘图对象（用于在PDF上绘制内容）
    QPainter painter;
    if (!painter.begin(&printer)) {
        QMessageBox::critical(this, "错误", "无法创建PDF文件");
        return;
    }

    // 设置不同字体样式
    QFont titleFont("Arial", 18, QFont::Bold);    // 标题字体
    QFont headerFont("Arial", 11, QFont::Bold);   // 表头字体
    QFont dataFont("Arial", 10);                   // 数据字体
    QFont footerFont("Arial", 8);                  // 页脚字体

    // 获取页面尺寸
    int pageWidth = printer.width();
    int pageHeight = printer.height();

    // 设置页面边距
    int margin = 50;
    int usableWidth = pageWidth - 2 * margin;  // 可用宽度
    int currentY = margin;                     // 当前绘制位置Y坐标

    // 查询任务数据
    QSqlQuery query(db);
    QString sql = "SELECT t.id, t.name, c.name as category, t.priority, t.deadline, t.status, t.desc "
                  "FROM task t LEFT JOIN category c ON t.category_id = c.id ORDER BY t.id";

    if (!query.exec(sql)) {
        QMessageBox::critical(this, "错误", "查询数据失败：" + query.lastError().text());
        painter.end();
        return;
    }

    // 查询总记录数
    int totalRecords = 0;
    QSqlQuery countQuery(db);
    if (countQuery.exec("SELECT COUNT(*) FROM task")) {
        if (countQuery.next()) {
            totalRecords = countQuery.value(0).toInt();
        }
    }

    // 绘制标题
    painter.setFont(titleFont);
    painter.drawText(QRect(margin, currentY, usableWidth, 60),
                     Qt::AlignCenter, "任务管理系统 - 任务列表");
    currentY += 70;

    // 绘制信息行（生成时间和记录数）
    painter.setFont(dataFont);
    QString infoText = QString("生成时间：%1 | 总记录数：%2 条")
                           .arg(QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss"))
                           .arg(totalRecords);
    painter.drawText(QRect(margin, currentY, usableWidth, 30),
                     Qt::AlignCenter, infoText);
    currentY += 40;

    // 设置表格列宽
    int colWidths[] = {60, 180, 80, 80, 100, 80, 220};  // 各列宽度
    int totalTableWidth = 0;
    for (int w : colWidths) totalTableWidth += w;

    // 如果表格总宽度超过可用宽度，等比例缩小
    if (totalTableWidth > usableWidth) {
        double scale = (double)usableWidth / totalTableWidth;
        for (int i = 0; i < 7; i++) {
            colWidths[i] = (int)(colWidths[i] * scale);
        }
        totalTableWidth = usableWidth;
    }

    // 计算表格起始X坐标（居中显示）
    int tableStartX = margin + (usableWidth - totalTableWidth) / 2;

    // 表头内容
    QStringList headers = {"ID", "任务名称", "分类", "优先级", "截止日期", "状态", "描述"};

    int rowHeight = 35;      // 行高
    int pageNum = 1;         // 当前页码
    int rowNum = 0;          // 当前行号
    bool isFirstPage = true; // 是否为第一页

    // 遍历查询结果，绘制表格
    while (query.next()) {
        // 如果是新的一页或第一页，绘制表头
        if (rowNum == 0 || rowNum % 20 == 0) {  // 每页最多20行数据
            if (!isFirstPage) {
                printer.newPage();  // 创建新页面
                pageNum++;
                currentY = margin;  // 重置Y坐标
            }

            // 绘制表头背景
            painter.setFont(headerFont);
            painter.setBrush(QColor(240, 240, 240));  // 浅灰色背景
            painter.setPen(Qt::NoPen);                // 无边框
            painter.drawRect(tableStartX, currentY, totalTableWidth, rowHeight);

            // 绘制表头文本和单元格边框
            painter.setPen(QColor(50, 50, 50));  // 深灰色边框
            int currentX = tableStartX;
            for (int i = 0; i < headers.size(); i++) {
                painter.drawRect(currentX, currentY, colWidths[i], rowHeight);  // 绘制单元格边框
                painter.drawText(QRect(currentX, currentY, colWidths[i], rowHeight),
                                 Qt::AlignCenter, headers[i]);  // 绘制表头文本
                currentX += colWidths[i];
            }

            currentY += rowHeight;
            painter.setFont(dataFont);  // 切换到数据字体

            if (isFirstPage) isFirstPage = false;
        }

        // 设置交替行背景色（斑马线效果）
        if (rowNum % 2 == 0) {
            painter.setBrush(QColor(250, 250, 250));  // 浅灰色
        } else {
            painter.setBrush(Qt::white);  // 白色
        }

        painter.setPen(Qt::NoPen);
        painter.drawRect(tableStartX, currentY, totalTableWidth, rowHeight);  // 绘制行背景

        // 绘制单元格数据
        painter.setPen(QColor(70, 70, 70));  // 深灰色文本
        int currentX = tableStartX;

        for (int col = 0; col < 7; col++) {
            QString text = query.value(col).toString();

            // 文本截断处理（防止内容过长）
            if (col == 1 || col == 6) {  // 任务名称和描述列需要截断
                int maxChars = col == 1 ? 15 : 25;  // 最大字符数
                if (text.length() > maxChars) {
                    text = text.left(maxChars) + "...";  // 截断并添加省略号
                }
            }

            // 绘制单元格边框
            painter.drawRect(currentX, currentY, colWidths[col], rowHeight);

            // 设置文本对齐方式（ID列左对齐，其他居中）
            Qt::Alignment alignment = (col == 0) ? Qt::AlignLeft | Qt::AlignVCenter : Qt::AlignCenter;
            int padding = (col == 0) ? 10 : 5;  // 内边距

            // 绘制单元格文本
            painter.drawText(QRect(currentX + padding, currentY,
                                   colWidths[col] - 2 * padding, rowHeight),
                             alignment, text);
            currentX += colWidths[col];
        }

        currentY += rowHeight;
        rowNum++;

        // 检查是否需要换页
        if (currentY + rowHeight > pageHeight - margin) {
            rowNum = 0;  // 重置行号，下一页重新开始计数
        }
    }

    // 绘制页脚（仅在最后一页底部）
    if (currentY + 50 < pageHeight) {
        painter.setFont(footerFont);
        painter.setPen(QColor(120, 120, 120));  // 灰色文本

        QString footerText = QString("第 %1 页 | 任务管理系统").arg(pageNum);
        painter.drawText(QRect(margin, pageHeight - 40, usableWidth, 30),
                         Qt::AlignCenter, footerText);
    }

    painter.end();  // 结束绘制

    // 显示导出结果信息
    QString resultMessage = QString("PDF导出成功！\n\n"
                                    "文件：%1\n"
                                    "总页数：%2 页\n"
                                    "记录数：%3 条")
                                .arg(fileName)
                                .arg(pageNum)
                                .arg(rowNum);

    QMessageBox::information(this, "导出成功", resultMessage);
}

// 优先级过滤器改变事件
void MainWindow::on_comboPriorityFilter_currentIndexChanged(int index)
{
    Q_UNUSED(index);  // 标记未使用参数（避免编译器警告）
    loadTaskData();   // 重新加载数据
}

// 状态过滤器改变事件
void MainWindow::on_comboStatusFilter_currentIndexChanged(int index)
{
    Q_UNUSED(index);  // 标记未使用参数
    loadTaskData();   // 重新加载数据
}
