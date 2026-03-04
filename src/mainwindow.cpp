#include "mainwindow.h"

#include "logparser.h"
#include "logmodel.h"
#include "logfilterproxymodel.h"
#include "highlightrulesdialog.h"

#include <QAction>
#include <QApplication>
#include <QCheckBox>
#include <QClipboard>
#include <QCloseEvent>
#include <QDockWidget>
#include <QFileDialog>
#include <QFileInfo>
#include <QFont>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QJsonArray>
#include <QJsonDocument>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSettings>
#include <QSplitter>
#include <QStatusBar>
#include <QTableView>
#include <QToolBar>
#include <QVBoxLayout>

#include <functional>

static QString displayEmpty(const QString& s) {
    return s.isEmpty() ? QStringLiteral("(пусто)") : s;
}

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent) {
    setupUi();
    createMenus();
    createFilterDock();

    loadSettings();
}

void MainWindow::setupUi() {
    setWindowTitle(QStringLiteral("LogViewer"));

    m_model = new LogModel(this);
    m_proxy = new LogFilterProxyModel(this);
    m_proxy->setSourceModel(m_model);

    m_view = new QTableView(this);
    m_view->setModel(m_proxy);
    m_view->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_view->setSelectionMode(QAbstractItemView::SingleSelection);
    m_view->setAlternatingRowColors(true);
    m_view->setSortingEnabled(true);
    m_view->setWordWrap(false);
    m_view->horizontalHeader()->setStretchLastSection(true);
    m_view->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);
    m_view->verticalHeader()->setVisible(false);
    m_view->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_view->setContextMenuPolicy(Qt::ActionsContextMenu);

    // Useful default widths
    m_view->setColumnWidth(LogModel::ColMid, 90);
    m_view->setColumnWidth(LogModel::ColTime, 180);
    m_view->setColumnWidth(LogModel::ColLevel, 70);
    m_view->setColumnWidth(LogModel::ColPid, 60);
    m_view->setColumnWidth(LogModel::ColTid, 130);
    m_view->setColumnWidth(LogModel::ColCategory, 140);
    m_view->setColumnWidth(LogModel::ColFile, 220);
    m_view->setColumnWidth(LogModel::ColLine, 60);
    m_view->setColumnWidth(LogModel::ColFunction, 200);

    m_details = new QPlainTextEdit(this);
    m_details->setReadOnly(true);
    QFont f = m_details->font();
    f.setFamily(QStringLiteral("Monospace"));
    f.setStyleHint(QFont::TypeWriter);
    m_details->setFont(f);

    auto* split = new QSplitter(Qt::Vertical, this);
    split->addWidget(m_view);
    split->addWidget(m_details);
    split->setStretchFactor(0, 3);
    split->setStretchFactor(1, 1);

    setCentralWidget(split);

    // actions in view context menu
    auto* copyAct = new QAction(QStringLiteral("Копировать строку"), this);
    connect(copyAct, &QAction::triggered, this, &MainWindow::copySelectedLine);
    m_view->addAction(copyAct);

    // selection -> details
    connect(m_view->selectionModel(), &QItemSelectionModel::currentChanged, this, [this]{ onCurrentChanged(); });

    // counts refresh
    connect(m_proxy, &QAbstractItemModel::rowsInserted, this, &MainWindow::updateCounts);
    connect(m_proxy, &QAbstractItemModel::rowsRemoved,  this, &MainWindow::updateCounts);
    connect(m_proxy, &QAbstractItemModel::modelReset,   this, &MainWindow::updateCounts);
    connect(m_proxy, &QAbstractItemModel::layoutChanged,this, &MainWindow::updateCounts);

    m_countLabel = new QLabel(this);
    statusBar()->addPermanentWidget(m_countLabel);
    updateCounts();
}

void MainWindow::createMenus() {
    auto* fileMenu = menuBar()->addMenu(QStringLiteral("Файл"));

    auto* openAct = fileMenu->addAction(QStringLiteral("Открыть..."));
    openAct->setShortcut(QKeySequence::Open);
    connect(openAct, &QAction::triggered, this, &MainWindow::openFile);

    auto* reloadAct = fileMenu->addAction(QStringLiteral("Перезагрузить"));
    reloadAct->setShortcut(QKeySequence::Refresh);
    connect(reloadAct, &QAction::triggered, this, &MainWindow::reloadFile);

    fileMenu->addSeparator();

    auto* exitAct = fileMenu->addAction(QStringLiteral("Выход"));
    exitAct->setShortcut(QKeySequence::Quit);
    connect(exitAct, &QAction::triggered, this, &QWidget::close);

    auto* viewMenu = menuBar()->addMenu(QStringLiteral("Вид"));

    auto* newestAct = viewMenu->addAction(QStringLiteral("Сортировать: новые сверху"));
    connect(newestAct, &QAction::triggered, this, &MainWindow::sortNewest);

    auto* oldestAct = viewMenu->addAction(QStringLiteral("Сортировать: старые сверху"));
    connect(oldestAct, &QAction::triggered, this, &MainWindow::sortOldest);

    viewMenu->addSeparator();

    auto* clearFiltersAct = viewMenu->addAction(QStringLiteral("Сбросить фильтры"));
    clearFiltersAct->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_L));
    connect(clearFiltersAct, &QAction::triggered, this, &MainWindow::clearAllFilters);

    auto* settingsMenu = menuBar()->addMenu(QStringLiteral("Настройки"));
    auto* rulesAct = settingsMenu->addAction(QStringLiteral("Правила подсветки..."));
    connect(rulesAct, &QAction::triggered, this, &MainWindow::showHighlightRules);

    // toolbar
    auto* tb = addToolBar(QStringLiteral("Основное"));
    tb->setMovable(false);
    tb->addAction(openAct);
    tb->addAction(reloadAct);
    tb->addSeparator();
    tb->addAction(newestAct);
    tb->addAction(oldestAct);
    tb->addSeparator();
    tb->addAction(clearFiltersAct);
    tb->addSeparator();
    tb->addAction(rulesAct);
}

static QListWidget* makeCheckList(QWidget* parent) {
    auto* w = new QListWidget(parent);
    w->setSelectionMode(QAbstractItemView::NoSelection);
    w->setUniformItemSizes(true);
    w->setAlternatingRowColors(true);
    w->setMaximumHeight(180);
    return w;
}

static void addCheckItem(QListWidget* w, const QString& value, const QString& display) {
    auto* it = new QListWidgetItem(display, w);
    it->setData(Qt::UserRole, value);
    it->setFlags(it->flags() | Qt::ItemIsUserCheckable);
    it->setCheckState(Qt::Checked);
}

static QWidget* makeAllNoneRow(const QString& allText, const QString& noneText,
                              std::function<void()> onAll,
                              std::function<void()> onNone,
                              QWidget* parent) {
    auto* w = new QWidget(parent);
    auto* h = new QHBoxLayout(w);
    h->setContentsMargins(0, 0, 0, 0);

    auto* allBtn = new QPushButton(allText, w);
    auto* noneBtn = new QPushButton(noneText, w);
    allBtn->setMaximumWidth(110);
    noneBtn->setMaximumWidth(110);

    QObject::connect(allBtn, &QPushButton::clicked, w, [onAll]{ onAll(); });
    QObject::connect(noneBtn, &QPushButton::clicked, w, [onNone]{ onNone(); });

    h->addWidget(allBtn);
    h->addWidget(noneBtn);
    h->addStretch();
    return w;
}

void MainWindow::createFilterDock() {
    auto* dock = new QDockWidget(QStringLiteral("Фильтры"), this);
    dock->setObjectName(QStringLiteral("filtersDock"));
    dock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);

    auto* root = new QWidget(dock);
    auto* v = new QVBoxLayout(root);
    v->setContentsMargins(8, 8, 8, 8);

    // Levels
    {
        auto* box = new QGroupBox(QStringLiteral("Уровни"), root);
        auto* bv = new QVBoxLayout(box);
        m_levels = makeCheckList(box);
        bv->addWidget(m_levels);
        bv->addWidget(makeAllNoneRow(QStringLiteral("Все"), QStringLiteral("Ничего"),
                                     [this]{ setAllChecked(m_levels, true); updateFiltersFromUI(); },
                                     [this]{ setAllChecked(m_levels, false); updateFiltersFromUI(); },
                                     box));
        v->addWidget(box);

        connect(m_levels, &QListWidget::itemChanged, this, &MainWindow::updateFiltersFromUI);
    }

    // Threads
    {
        auto* box = new QGroupBox(QStringLiteral("Потоки (TID)"), root);
        auto* bv = new QVBoxLayout(box);
        m_tids = makeCheckList(box);
        bv->addWidget(m_tids);
        bv->addWidget(makeAllNoneRow(QStringLiteral("Все"), QStringLiteral("Ничего"),
                                     [this]{ setAllChecked(m_tids, true); updateFiltersFromUI(); },
                                     [this]{ setAllChecked(m_tids, false); updateFiltersFromUI(); },
                                     box));
        v->addWidget(box);

        connect(m_tids, &QListWidget::itemChanged, this, &MainWindow::updateFiltersFromUI);
    }

    // Categories
    {
        auto* box = new QGroupBox(QStringLiteral("Категории"), root);
        auto* bv = new QVBoxLayout(box);
        m_categories = makeCheckList(box);
        bv->addWidget(m_categories);
        bv->addWidget(makeAllNoneRow(QStringLiteral("Все"), QStringLiteral("Ничего"),
                                     [this]{ setAllChecked(m_categories, true); updateFiltersFromUI(); },
                                     [this]{ setAllChecked(m_categories, false); updateFiltersFromUI(); },
                                     box));
        v->addWidget(box);

        connect(m_categories, &QListWidget::itemChanged, this, &MainWindow::updateFiltersFromUI);
    }

    // Other filters
    {
        auto* box = new QGroupBox(QStringLiteral("Дополнительно"), root);
        auto* bv = new QVBoxLayout(box);

        m_pidEdit = new QLineEdit(box);
        m_pidEdit->setPlaceholderText(QStringLiteral("PID (список через запятую), например: 40358"));

        m_catContainsEdit = new QLineEdit(box);
        m_catContainsEdit->setPlaceholderText(QStringLiteral("Категория содержит..."));

        m_fileEdit = new QLineEdit(box);
        m_fileEdit->setPlaceholderText(QStringLiteral("Файл содержит..."));

        m_funcEdit = new QLineEdit(box);
        m_funcEdit->setPlaceholderText(QStringLiteral("Функция содержит..."));

        m_msgEdit = new QLineEdit(box);
        m_msgEdit->setPlaceholderText(QStringLiteral("Сообщение содержит..."));

        m_showMeta = new QCheckBox(QStringLiteral("Показывать META строки"), box);
        m_showMeta->setChecked(true);

        auto* clearBtn = new QPushButton(QStringLiteral("Сбросить все фильтры"), box);
        connect(clearBtn, &QPushButton::clicked, this, &MainWindow::clearAllFilters);

        bv->addWidget(m_pidEdit);
        bv->addWidget(m_catContainsEdit);
        bv->addWidget(m_fileEdit);
        bv->addWidget(m_funcEdit);
        bv->addWidget(m_msgEdit);
        bv->addWidget(m_showMeta);
        bv->addWidget(clearBtn);

        v->addWidget(box);

        connect(m_pidEdit, &QLineEdit::textChanged, this, &MainWindow::updateFiltersFromUI);
        connect(m_catContainsEdit, &QLineEdit::textChanged, this, &MainWindow::updateFiltersFromUI);
        connect(m_fileEdit, &QLineEdit::textChanged, this, &MainWindow::updateFiltersFromUI);
        connect(m_funcEdit, &QLineEdit::textChanged, this, &MainWindow::updateFiltersFromUI);
        connect(m_msgEdit, &QLineEdit::textChanged, this, &MainWindow::updateFiltersFromUI);
        connect(m_showMeta, &QCheckBox::toggled, this, &MainWindow::updateFiltersFromUI);
    }

    v->addStretch();

    dock->setWidget(root);
    addDockWidget(Qt::LeftDockWidgetArea, dock);
}

void MainWindow::openFile() {
    const QString path = QFileDialog::getOpenFileName(
        this,
        QStringLiteral("Открыть лог"),
        m_currentFile.isEmpty() ? QString() : QFileInfo(m_currentFile).absolutePath(),
        QStringLiteral("Log files (*.log *.txt);;All files (*.*)")
    );
    if (path.isEmpty()) return;
    loadLogFile(path);
}

void MainWindow::reloadFile() {
    if (m_currentFile.isEmpty()) return;
    loadLogFile(m_currentFile);
}

void MainWindow::loadLogFile(const QString& path) {
    const auto res = LogParser::parseFile(path);
    if (!res.ok) {
        QMessageBox::critical(this, QStringLiteral("Ошибка"), res.errorString);
        return;
    }

    m_currentFile = path;
    m_model->setEntries(res.entries);

    // Apply highlight rules (already loaded from settings)
    m_model->setHighlightRules(m_rules);

    rebuildFilterLists();

    // default sorting: newest first
    sortNewest();

    updateCounts();
    statusBar()->showMessage(QStringLiteral("Загружено: %1").arg(QFileInfo(path).fileName()), 2500);
}

void MainWindow::rebuildFilterLists() {
    if (!m_levels || !m_tids || !m_categories) return;

    QSet<QString> levels;
    QSet<QString> tids;
    QSet<QString> cats;

    for (int i = 0; i < m_model->rowCount(); ++i) {
        const LogEntry& e = m_model->entryAt(i);
        levels.insert(e.level);
        tids.insert(e.tid);
        cats.insert(e.category);
    }

    auto fill = [](QListWidget* w, const QSet<QString>& values) {
        w->blockSignals(true);
        w->clear();
        QStringList list = values.values();
        list.sort();
        for (const QString& v : list) {
            addCheckItem(w, v, displayEmpty(v));
        }
        w->blockSignals(false);
    };

    fill(m_levels, levels);
    fill(m_tids, tids);
    fill(m_categories, cats);

    // Apply to proxy
    updateFiltersFromUI();
}

QSet<QString> MainWindow::checkedItemsToSet(QListWidget* w) const {
    QSet<QString> out;
    for (int i = 0; i < w->count(); ++i) {
        const auto* it = w->item(i);
        if (it->checkState() == Qt::Checked) {
            out.insert(it->data(Qt::UserRole).toString());
        }
    }
    return out;
}

void MainWindow::setAllChecked(QListWidget* w, bool checked) {
    if (!w) return;
    w->blockSignals(true);
    for (int i = 0; i < w->count(); ++i) {
        auto* it = w->item(i);
        it->setCheckState(checked ? Qt::Checked : Qt::Unchecked);
    }
    w->blockSignals(false);
}

void MainWindow::updateFiltersFromUI() {
    if (!m_proxy || !m_levels || !m_tids || !m_categories) return;

    m_proxy->setAllowedLevels(checkedItemsToSet(m_levels));
    m_proxy->setAllowedTids(checkedItemsToSet(m_tids));
    m_proxy->setAllowedCategories(checkedItemsToSet(m_categories));

    m_proxy->setPidFilterText(m_pidEdit ? m_pidEdit->text() : QString());
    m_proxy->setCategoryContains(m_catContainsEdit ? m_catContainsEdit->text() : QString());
    m_proxy->setFileContains(m_fileEdit ? m_fileEdit->text() : QString());
    m_proxy->setFunctionContains(m_funcEdit ? m_funcEdit->text() : QString());
    m_proxy->setMessageContains(m_msgEdit ? m_msgEdit->text() : QString());

    m_proxy->setShowMeta(m_showMeta ? m_showMeta->isChecked() : true);

    updateCounts();
}

void MainWindow::updateCounts() {
    const int total = m_model ? m_model->rowCount() : 0;
    const int shown = m_proxy ? m_proxy->rowCount() : 0;

    const QString file = m_currentFile.isEmpty() ? QStringLiteral("(файл не выбран)") : QFileInfo(m_currentFile).fileName();
    m_countLabel->setText(QStringLiteral("%1  |  Показано: %2 / %3")
                          .arg(file)
                          .arg(shown)
                          .arg(total));
}

void MainWindow::onCurrentChanged() {
    if (!m_view || !m_details) return;

    const QModelIndex proxyIdx = m_view->currentIndex();
    if (!proxyIdx.isValid()) {
        m_details->clear();
        return;
    }

    const QModelIndex srcIdx = m_proxy->mapToSource(proxyIdx);
    const int row = srcIdx.row();
    if (row < 0 || row >= m_model->rowCount()) {
        m_details->clear();
        return;
    }

    const LogEntry& e = m_model->entryAt(row);

    QString text;
    if (e.messageId >= 0) {
        text += QStringLiteral("MID=%1\n").arg(e.messageId);
    }
    if (e.timestamp.isValid()) {
        text += e.timestamp.toString("yyyy-MM-dd HH:mm:ss.zzz") + " " + e.level + "\n";
        text += QStringLiteral("pid=%1 tid=%2 [%3]\n").arg(e.pid).arg(e.tid).arg(e.category);
        text += QStringLiteral("%1:%2\n").arg(e.file, e.lineText);
        text += e.function + "\n";
        text += "\n";
    } else {
        text += QStringLiteral("META\n\n");
    }

    text += e.message;

    m_details->setPlainText(text);
}

void MainWindow::sortNewest() {
    if (!m_view) return;
    m_view->sortByColumn(LogModel::ColMid, Qt::DescendingOrder);
}

void MainWindow::sortOldest() {
    if (!m_view) return;
    m_view->sortByColumn(LogModel::ColMid, Qt::AscendingOrder);
}

void MainWindow::clearAllFilters() {
    setAllChecked(m_levels, true);
    setAllChecked(m_tids, true);
    setAllChecked(m_categories, true);

    if (m_pidEdit) m_pidEdit->clear();
    if (m_catContainsEdit) m_catContainsEdit->clear();
    if (m_fileEdit) m_fileEdit->clear();
    if (m_funcEdit) m_funcEdit->clear();
    if (m_msgEdit) m_msgEdit->clear();

    if (m_showMeta) m_showMeta->setChecked(true);

    updateFiltersFromUI();
}

void MainWindow::showHighlightRules() {
    HighlightRulesDialog dlg(this);
    dlg.setRules(m_rules);

    if (dlg.exec() != QDialog::Accepted) return;

    m_rules = dlg.rules();
    for (HighlightRule& r : m_rules) r.compile();

    if (m_model) {
        m_model->setHighlightRules(m_rules);
    }

    // persist immediately
    QSettings s;
    QJsonArray arr;
    for (const auto& r : m_rules) arr.append(r.toJson());
    s.setValue("highlightRules", QString::fromUtf8(QJsonDocument(arr).toJson(QJsonDocument::Compact)));
}

void MainWindow::copySelectedLine() {
    const QModelIndex proxyIdx = m_view->currentIndex();
    if (!proxyIdx.isValid()) return;

    const QModelIndex srcIdx = m_proxy->mapToSource(proxyIdx);
    const int row = srcIdx.row();
    if (row < 0 || row >= m_model->rowCount()) return;

    const LogEntry& e = m_model->entryAt(row);
    QApplication::clipboard()->setText(e.raw);
    statusBar()->showMessage(QStringLiteral("Скопировано в буфер"), 1500);
}

void MainWindow::loadSettings() {
    QSettings s;

    restoreGeometry(s.value("window/geometry").toByteArray());
    restoreState(s.value("window/state").toByteArray());

    // highlight rules
    const QString json = s.value("highlightRules").toString();
    if (!json.trimmed().isEmpty()) {
        const QJsonDocument doc = QJsonDocument::fromJson(json.toUtf8());
        if (doc.isArray()) {
            QVector<HighlightRule> rules;
            const QJsonArray arr = doc.array();
            rules.reserve(arr.size());
            for (const QJsonValue& v : arr) {
                if (v.isObject()) rules.push_back(HighlightRule::fromJson(v.toObject()));
            }
            m_rules = rules;
        }
    }
    if (m_rules.isEmpty()) {
        m_rules = HighlightRule::defaultRules();
    }

    // last file
    const QString last = s.value("lastFile").toString();
    if (!last.isEmpty() && QFileInfo::exists(last)) {
        loadLogFile(last);
    }
}

void MainWindow::saveSettings() {
    QSettings s;
    s.setValue("window/geometry", saveGeometry());
    s.setValue("window/state", saveState());
    s.setValue("lastFile", m_currentFile);

    // rules are saved on change (but still save on exit)
    QJsonArray arr;
    for (const auto& r : m_rules) arr.append(r.toJson());
    s.setValue("highlightRules", QString::fromUtf8(QJsonDocument(arr).toJson(QJsonDocument::Compact)));
}

void MainWindow::closeEvent(QCloseEvent* e) {
    saveSettings();
    QMainWindow::closeEvent(e);
}
