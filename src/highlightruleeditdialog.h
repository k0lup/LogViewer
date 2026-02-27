#pragma once

#include "highlightrule.h"

#include <QDialog>

class QCheckBox;
class QComboBox;
class QLineEdit;
class QPushButton;

class HighlightRuleEditDialog : public QDialog {
    Q_OBJECT
public:
    explicit HighlightRuleEditDialog(QWidget* parent = nullptr);

    void setRule(const HighlightRule& rule);
    HighlightRule rule() const;

private slots:
    void pickForeground();
    void pickBackground();
    void updateButtons();

private:
    static void applyColorToButton(QPushButton* btn, const QColor& c);

private:
    QCheckBox* m_enabled = nullptr;
    QLineEdit* m_name = nullptr;
    QComboBox* m_level = nullptr;

    QLineEdit* m_pid = nullptr;
    QLineEdit* m_tid = nullptr;

    QLineEdit* m_catRe = nullptr;
    QLineEdit* m_fileRe = nullptr;
    QLineEdit* m_funcRe = nullptr;
    QLineEdit* m_msgRe = nullptr;

    QCheckBox* m_useFg = nullptr;
    QPushButton* m_fgBtn = nullptr;
    QColor m_fg;

    QCheckBox* m_useBg = nullptr;
    QPushButton* m_bgBtn = nullptr;
    QColor m_bg;
};
