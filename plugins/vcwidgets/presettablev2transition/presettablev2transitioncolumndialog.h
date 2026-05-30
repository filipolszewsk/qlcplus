#pragma once

#include <QDialog>
#include <QSharedPointer>

class Doc;
class QLCInputSource;
class InputSelectionWidget;

class PresetTableV2TransitionColumnDialog : public QDialog
{
    Q_OBJECT
public:
    PresetTableV2TransitionColumnDialog(Doc* doc,
                                          const QString& columnTitle,
                                          QSharedPointer<QLCInputSource> src,
                                          int widgetPage,
                                          QWidget* parent = nullptr);

    QSharedPointer<QLCInputSource> inputSource() const;

private:
    InputSelectionWidget* m_inputSel = nullptr;
};
