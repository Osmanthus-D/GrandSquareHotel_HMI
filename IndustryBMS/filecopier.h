#ifndef FILECOPIER_H
#define FILECOPIER_H

#include <QFile>
#include <QFileInfo>
#include <QThread>
#include <QDebug>
#include <QTest>
#include <unistd.h>

class FileCopier : public QObject
{
    Q_OBJECT
public:
    explicit FileCopier(QObject *parent = 0);
    QString srcFileName() const;
    QString destFileName() const;
    void setSrcFileName(const QString &fileName);
    void setDestFileName(const QString &fileName);

signals:
    void percentCopied(double percent);
    void finished();
    void errorOccurred();

public slots:
    void startCopying();
    void cancelCopying();

private:
    bool m_canceled;
    QFile srcFile;
    QFile destFile;
};

#endif // FILECOPIER_H
