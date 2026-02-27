#include "highlightruleeditdialog.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QColorDialog>

HighlightRuleEditDialog::HighlightRuleEditDialog(QWidget* parent)
    : QDialog(parent) {
    setWindowTitle(QStringLiteral("Правило подсветки"));
    setModal(true);

    m_enabled = new QCheckBox(QStringLiteral("Включено"), this);
    m_name = new QLineEdit(this);

    m_level = new QComboBox(this);
    m_level->addItem(QStringLiteral("Любой"), QString());
    m_level->addItem(QStringLiteral("DEBUG"), QStringLiteral("DEBUG"));
    m_level->addItem(QStringLiteral("INFO"),  QStringLiteral("INFO"));
    m_level->addItem(QStringLiteral("WARN"),  QStringLiteral("WARN"));
    m_level->addItem(QStringLiteral("ERROR"), QStringLiteral("ERROR"));
    m_level->addItem(QStringLiteral("FATAL"), QStringLiteral("FATAL"));
    m_level->addItem(QStringLiteral("META"),  QStringLiteral("META"));

    m_pid = new QLineEdit(this);
    m_pid->setPlaceholderText(QStringLiteral("например: 40358"));

    m_tid = new QLineEdit(this);
    m_tid->setPlaceholderText(QStringLiteral("например: 0x7fc220958c00"));

    m_catRe = new QLineEdit(this);
    m_catRe->setPlaceholderText(QStringLiteral("regex (без / /), например: ^app\\."));

    m_fileRe = new QLineEdit(this);
    m_fileRe->setPlaceholderText(QStringLiteral("regex, например: main\\.cpp"));

    m_funcRe = new QLineEdit(this);
    m_funcRe->setPlaceholderText(QStringLiteral("regex, например: ^int main"));

    m_msgRe = new QLineEdit(this);
    m_msgRe->setPlaceholderText(QStringLiteral("regex, например: Ошибка|ERROR"));

    m_useFg = new QCheckBox(QStringLiteral("Использовать"), this);
    m_fgBtn = new QPushButton(this);
    m_fgBtn->setMinimumWidth(90);
    connect(m_fgBtn, &QPushButton::clicked, this, &HighlightRuleEditDialog::pickForeground);

    m_useBg = new QCheckBox(QStringLiteral("Использовать"), this);
    m_bgBtn = new QPushButton(this);
    m_bgBtn->setMinimumWidth(90);
    connect(m_bgBtn, &QPushButton::clicked, this, &HighlightRuleEditDialog::pickBackground);

    connect(m_useFg, &QCheckBox::toggled, this, &HighlightRuleEditDialog::updateButtons);
    connect(m_useBg, &QCheckBox::toggled, this, &HighlightRuleEditDialog::updateButtons);

    auto* form = new QFormLayout;
    form->addRow(m_enabled);

    form->addRow(QStringLiteral("Название"), m_name);
    form->addRow(QStringLiteral("Уровень"), m_level);
    form->addRow(QStringLiteral("PID"), m_pid);
    form->addRow(QStringLiteral("TID"), m_tid);

    form->addRow(QStringLiteral("Категория (regex)"), m_catRe);
    form->addRow(QStringLiteral("Файл (regex)"), m_fileRe);
    form->addRow(QStringLiteral("Функция (regex)"), m_funcRe);
    form->addRow(QStringLiteral("Сообщение (regex)"), m_msgRe);

    {
        auto* w = new QWidget(this);
        auto* h = new QHBoxLayout(w);
        h->setContentsMargins(0, 0, 0, 0);
        h->addWidget(m_useFg);
        h->addWidget(m_fgBtn);
        h->addStretch();
        form->addRow(QStringLiteral("Цвет текста"), w);
    }

    {
        auto* w = new QWidget(this);
        auto* h = new QHBoxLayout(w);
        h->setContentsMargins(0, 0, 0, 0);
        h->addWidget(m_useBg);
        h->addWidget(m_bgBtn);
        h->addStretch();
        form->addRow(QStringLiteral("Фон"), w);
    }

    auto* hint = new QLabel(QStringLiteral("Regex сравнивается без учёта регистра. Пустые поля означают \"любой\"."), this);
    hint->setWordWrap(true);

    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

    auto* v = new QVBoxLayout(this);
    v->addLayout(form);
    v->addWidget(hint);
    v->addWidget(buttons);

    // defaults
    m_enabled->setChecked(true);
    m_fg = Qt::black;
    m_bg = Qt::transparent;
    applyColorToButton(m_fgBtn, m_fg);
    applyColorToButton(m_bgBtn, m_bg);
    m_useFg->setChecked(true);
    m_useBg->setChecked(false);

    updateButtons();
}

void HighlightRuleEditDialog::applyColorToButton(QPushButton* btn, const QColor& c) {
    btn->setText(c.isValid() ? c.name(QColor::HexArgb) : QStringLiteral("(нет)"));
    if (c.isValid()) {
        btn->setStyleSheet(QStringLiteral("QPushButton{ background:%1; }").arg(c.name(QColor::HexArgb)));
    } else {
        btn->setStyleSheet(QString());
    }
}

void HighlightRuleEditDialog::setRule(const HighlightRule& r) {
    m_enabled->setChecked(r.enabled);
    m_name->setText(r.name);

    // level
    const QString lvl = r.level.trimmed().toUpper();
    int idx = m_level->findData(lvl);
    if (idx < 0) idx = 0;
    m_level->setCurrentIndex(idx);

    m_pid->setText(r.pid);
    m_tid->setText(r.tid);

    m_catRe->setText(r.categoryRegex);
    m_fileRe->setText(r.fileRegex);
    m_funcRe->setText(r.functionRegex);
    m_msgRe->setText(r.messageRegex);

    m_useFg->setChecked(r.useForeground);
    m_useBg->setChecked(r.useBackground);

    m_fg = r.foreground;
    m_bg = r.background;

    applyColorToButton(m_fgBtn, m_fg);
    applyColorToButton(m_bgBtn, m_bg);

    updateButtons();
}

HighlightRule HighlightRuleEditDialog::rule() const {
    HighlightRule r;
    r.enabled = m_enabled->isChecked();
    r.name = m_name->text().trimmed();
    r.level = m_level->currentData().toString();

    r.pid = m_pid->text().trimmed();
    r.tid = m_tid->text().trimmed();

    r.categoryRegex = m_catRe->text().trimmed();
    r.fileRegex = m_fileRe->text().trimmed();
    r.functionRegex = m_funcRe->text().trimmed();
    r.messageRegex = m_msgRe->text().trimmed();

    r.useForeground = m_useFg->isChecked();
    r.foreground = m_fg;
    r.useBackground = m_useBg->isChecked();
    r.background = m_bg;

    r.compile();
    return r;
}

void HighlightRuleEditDialog::pickForeground() {
    const QColor c = QColorDialog::getColor(m_fg, this, QStringLiteral("Цвет текста"));
    if (c.isValid()) {
        m_fg = c;
        applyColorToButton(m_fgBtn, m_fg);
    }
}

void HighlightRuleEditDialog::pickBackground() {
    const QColor c = QColorDialog::getColor(m_bg, this, QStringLiteral("Цвет фона"));
    if (c.isValid()) {
        m_bg = c;
        applyColorToButton(m_bgBtn, m_bg);
    }
}

void HighlightRuleEditDialog::updateButtons() {
    m_fgBtn->setEnabled(m_useFg->isChecked());
    m_bgBtn->setEnabled(m_useBg->isChecked());
}
