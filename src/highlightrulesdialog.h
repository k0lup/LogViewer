#pragma once

#include "highlightrule.h"

#include <QDialog>
#include <QVector>

class QTableWidget;
class QPushButton;

class HighlightRulesDialog : public QDialog {
    Q_OBJECT
public:
    explicit HighlightRulesDialog(QWidget* parent = nullptr);

    void setRules(const QVector<HighlightRule>& rules);
    QVector<HighlightRule> rules() const;

private slots:
    void addRule();
    void editRule();
    void removeRule();
    void moveUp();
    void moveDown();
    void resetDefaults();
    void onSelectionChanged();
    void onCellChanged(int row, int col);

private:
    void rebuildTable();
    int currentRow() const;
    void setButtonsEnabled();

private:
    QTableWidget* m_table = nullptr;
    QPushButton* m_add = nullptr;
    QPushButton* m_edit = nullptr;
    QPushButton* m_remove = nullptr;
    QPushButton* m_up = nullptr;
    QPushButton* m_down = nullptr;
    QPushButton* m_defaults = nullptr;

    QVector<HighlightRule> m_rules;
    bool m_blockCellSignals = false;
};
