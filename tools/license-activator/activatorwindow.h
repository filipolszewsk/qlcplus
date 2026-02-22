/*
  QLC+ License Activator
  activatorwindow.h

  Copyright (c) Filip Olszewski
  PROPRIETARY - NOT OPEN SOURCE
*/

#ifndef ACTIVATORWINDOW_H
#define ACTIVATORWINDOW_H

#include <QWidget>

class QLineEdit;
class QLabel;
class QPushButton;
class QNetworkAccessManager;
class QNetworkReply;

class ActivatorWindow : public QWidget
{
    Q_OBJECT

public:
    explicit ActivatorWindow(QWidget *parent = nullptr);

private slots:
    void slotActivate();
    void slotDeactivate();
    void slotActivateReply(QNetworkReply *reply);
    void slotDeactivateReply(QNetworkReply *reply);
    void slotCopyHwId();

private:
    void generateKeyFile(const QString &customerName, const QString &customerEmail,
                         const QString &licenseKey, const QString &instanceId);
    QString readInstanceIdFromKeyFile() const;
    void updateStatus();
    QString keyFilePath() const;
    void setStatus(const QString &msg, bool success);

    QString m_fullHwId;
    QLineEdit *m_keyInput;
    QLabel *m_statusLabel;
    QLabel *m_hwShortLabel;
    QPushButton *m_activateBtn;
    QPushButton *m_deactivateBtn;
    QNetworkAccessManager *m_nam;

    static const QString MASTER_CONTENT_KEY;
};

#endif // ACTIVATORWINDOW_H
