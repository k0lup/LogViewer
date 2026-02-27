#include "highlightrulesdialog.h"
#include "highlightruleeditdialog.h"

#include <QTableWidget>
#include <QHeaderView>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QMessageBox>

enum Col {
    CEnabled = 0,
    CName,
    CLevel,
    CPid,
    CTid,
    CCat,
    CFile,
    CFunc,
    CMsg,
    CFg,
    CBg,
    CCount
};

static QTableWidgetItem* makeItem(const QString& text) {
    auto* it = new QTableWidgetItem(text);
    it->setFlags(it->flags() & ~Qt::ItemIsEditable);
    return it;
}

HighlightRulesDialog::HighlightRulesDialog(QWidget* parent)
    : QDialog(parent) {
    setWindowTitle(QStringLiteral("Правила подсветки"));
    setModal(true);
    resize(1100, 500);

    m_table = new QTableWidget(this);
    m_table->setColumnCount(CCount);
    m_table->setHorizontalHeaderLabels({
        QStringLiteral("On"),
        QStringLiteral("Название"),
        QStringLiteral("Уровень"),
        QStringLiteral("PID"),
        QStringLiteral("TID"),
        QStringLiteral("Категория (re)"),
        QStringLiteral("Файл (re)"),
        QStringLiteral("Функция (re)"),
        QStringLiteral("Сообщение (re)"),
        QStringLiteral("Текст"),
        QStringLiteral("Фон")
    });
    m_table->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setStretchLastSection(true);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setAlternatingRowColors(true);

    connect(m_table, &QTableWidget::cellDoubleClicked, this, [this](int, int){ editRule(); });
    connect(m_table->selectionModel(), &QItemSelectionModel::selectionChanged, this, &HighlightRulesDialog::onSelectionChanged);
    connect(m_table, &QTableWidget::cellChanged, this, &HighlightRulesDialog::onCellChanged);

    m_add = new QPushButton(QStringLiteral("Добавить"), this);
    m_edit = new QPushButton(QStringLiteral("Изменить"), this);
    m_remove = new QPushButton(QStringLiteral("Удалить"), this);
    m_up = new QPushButton(QStringLiteral("Вверх"), this);
    m_down = new QPushButton(QStringLiteral("Вниз"), this);
    m_defaults = new QPushButton(QStringLiteral("Сбросить по умолчанию"), this);

    connect(m_add, &QPushButton::clicked, this, &HighlightRulesDialog::addRule);
    connect(m_edit, &QPushButton::clicked, this, &HighlightRulesDialog::editRule);
    connect(m_remove, &QPushButton::clicked, this, &HighlightRulesDialog::removeRule);
    connect(m_up, &QPushButton::clicked, this, &HighlightRulesDialog::moveUp);
    connect(m_down, &QPushButton::clicked, this, &HighlightRulesDialog::moveDown);
    connect(m_defaults, &QPushButton::clicked, this, &HighlightRulesDialog::resetDefaults);

    auto* leftBtns = new QHBoxLayout;
    leftBtns->addWidget(m_add);
    leftBtns->addWidget(m_edit);
    leftBtns->addWidget(m_remove);
    leftBtns->addSpacing(10);
    leftBtns->addWidget(m_up);
    leftBtns->addWidget(m_down);
    leftBtns->addStretch();
    leftBtns->addWidget(m_defaults);

    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

    auto* v = new QVBoxLayout(this);
    v->addWidget(m_table);
    v->addLayout(leftBtns);
    v->addWidget(buttons);

    setButtonsEnabled();
}

void HighlightRulesDialog::setRules(const QVector<HighlightRule>& rules) {
    m_rules = rules;
    rebuildTable();
}

QVector<HighlightRule> HighlightRulesDialog::rules() const {
    return m_rules;
}

void HighlightRulesDialog::rebuildTable() {
    m_blockCellSignals = true;
    m_table->setRowCount(m_rules.size());

    for (int r = 0; r < m_rules.size(); ++r) {
        const HighlightRule& rule = m_rules.at(r);

        auto* en = new QTableWidgetItem;
        en->setFlags((en->flags() | Qt::ItemIsUserCheckable) & ~Qt::ItemIsEditable);
        en->setCheckState(rule.enabled ? Qt::Checked : Qt::Unchecked);
        m_table->setItem(r, CEnabled, en);

        m_table->setItem(r, CName, makeItem(rule.name));
        m_table->setItem(r, CLevel, makeItem(rule.level.isEmpty() ? QStringLiteral("*") : rule.level));
        m_table->setItem(r, CPid, makeItem(rule.pid.isEmpty() ? QStringLiteral("*") : rule.pid));
        m_table->setItem(r, CTid, makeItem(rule.tid.isEmpty() ? QStringLiteral("*") : rule.tid));
        m_table->setItem(r, CCat, makeItem(rule.categoryRegex.isEmpty() ? QStringLiteral("*") : rule.categoryRegex));
        m_table->setItem(r, CFile, makeItem(rule.fileRegex.isEmpty() ? QStringLiteral("*") : rule.fileRegex));
        m_table->setItem(r, CFunc, makeItem(rule.functionRegex.isEmpty() ? QStringLiteral("*") : rule.functionRegex));
        m_table->setItem(r, CMsg, makeItem(rule.messageRegex.isEmpty() ? QStringLiteral("*") : rule.messageRegex));

        {
            QString t = rule.useForeground ? rule.foreground.name(QColor::HexArgb) : QStringLiteral("(нет)");
            auto* it = makeItem(t);
            if (rule.useForeground) it->setForeground(QBrush(rule.foreground));
            m_table->setItem(r, CFg, it);
        }
        {
            QString t = rule.useBackground ? rule.background.name(QColor::HexArgb) : QStringLiteral("(нет)");
            auto* it = makeItem(t);
            if (rule.useBackground) it->setBackground(QBrush(rule.background));
            m_table->setItem(r, CBg, it);
        }
    }

    m_blockCellSignals = false;
    setButtonsEnabled();
}

int HighlightRulesDialog::currentRow() const {
    const QModelIndexList rows = m_table->selectionModel()->selectedRows();
    if (rows.isEmpty()) return -1;
    return rows.first().row();
}

void HighlightRulesDialog::setButtonsEnabled() {
    const int r = currentRow();
    const bool has = (r >= 0 && r < m_rules.size());

    m_edit->setEnabled(has);
    m_remove->setEnabled(has);
    m_up->setEnabled(has && r > 0);
    m_down->setEnabled(has && r < m_rules.size() - 1);
}

void HighlightRulesDialog::onSelectionChanged() {
    setButtonsEnabled();
}

void HighlightRulesDialog::onCellChanged(int row, int col) {
    if (m_blockCellSignals) return;
    if (row < 0 || row >= m_rules.size()) return;

    if (col == CEnabled) {
        const auto* it = m_table->item(row, CEnabled);
        if (!it) return;
        m_rules[row].enabled = (it->checkState() == Qt::Checked);
    }
}

void HighlightRulesDialog::addRule() {
    HighlightRuleEditDialog dlg(this);
    HighlightRule r;
    r.name = QStringLiteral("Новое правило");
    r.compile();
    dlg.setRule(r);
    if (dlg.exec() != QDialog::Accepted) return;

    m_rules.push_back(dlg.rule());
    rebuildTable();

    const int newRow = m_rules.size() - 1;
    m_table->selectRow(newRow);
}

void HighlightRulesDialog::editRule() {
    const int r = currentRow();
    if (r < 0 || r >= m_rules.size()) return;

    HighlightRuleEditDialog dlg(this);
    dlg.setRule(m_rules.at(r));
    if (dlg.exec() != QDialog::Accepted) return;

    m_rules[r] = dlg.rule();
    rebuildTable();
    m_table->selectRow(r);
}

void HighlightRulesDialog::removeRule() {
    const int r = currentRow();
    if (r < 0 || r >= m_rules.size()) return;

    if (QMessageBox::question(this,
                              QStringLiteral("Удалить правило"),
                              QStringLiteral("Удалить правило \"%1\"?").arg(m_rules.at(r).name))
        != QMessageBox::Yes) {
        return;
    }

    m_rules.removeAt(r);
    rebuildTable();

    const int sel = qMin(r, m_rules.size() - 1);
    if (sel >= 0) m_table->selectRow(sel);
}

void HighlightRulesDialog::moveUp() {
    const int r = currentRow();
    if (r <= 0 || r >= m_rules.size()) return;
    m_rules.swapItemsAt(r, r - 1);
    rebuildTable();
    m_table->selectRow(r - 1);
}

void HighlightRulesDialog::moveDown() {
    const int r = currentRow();
    if (r < 0 || r >= m_rules.size() - 1) return;
    m_rules.swapItemsAt(r, r + 1);
    rebuildTable();
    m_table->selectRow(r + 1);
}

void HighlightRulesDialog::resetDefaults() {
    if (QMessageBox::question(this,
                              QStringLiteral("Сброс"),
                              QStringLiteral("Сбросить правила на значения по умолчанию?"))
        != QMessageBox::Yes) {
        return;
    }
    m_rules = HighlightRule::defaultRules();
    rebuildTable();
}
