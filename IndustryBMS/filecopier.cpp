#include "filecopier.h"

FileCopier::FileCopier(QObject *parent) : QObject(parent)
{
    // 这个类与多线程的实现没有直接的关系，它就老老实实地做好自己的事情
    m_canceled = false;
    qDebug() << Q_FUNC_INFO << "currentThreadId:" << QThread::currentThreadId();
}

void FileCopier::startCopying()
{
    qDebug() << Q_FUNC_INFO << "currentThreadId:" << QThread::currentThreadId();
    qDebug() << "START";
    m_canceled = false;
    qint64 srcSize = QFileInfo(srcFile).size();
    int bytesWritten = 0;
    if (srcSize==0 || !srcFile.open(QFile::ReadOnly))
    {
        qDebug() << __func__ << "Err: srcSize equals 0 or srcFile open failed.";
        emit errorOccurred();
        return;
    }
    if (destFile.exists())
    {
        destFile.resize(0);
    }
    if (!destFile.open(QFile::WriteOnly))
    {
        srcFile.close();
        qDebug() << __func__ << "Err: destFile open failed:" << destFile.fileName();
        emit errorOccurred();
        return;
    }
    while (!srcFile.atEnd())
    {
        if (m_canceled)
        {
            srcFile.close();
            destFile.close();
            destFile.remove();
            return;
        }
//        QTest::qSleep(1);
        qint64 wrt = destFile.write(srcFile.read(1024*10));
//        usleep(50);
//        qint64 wrt = destFile.write(srcFile.readLine());
//        qint64 wrt = srcFile.readLine().size();
//        qDebug() << __func__ << wrt;
        if (wrt < 0)
        {
            qDebug() << __func__ << "Err: destFile write failed.";
            emit errorOccurred();
            srcFile.close();
            destFile.close();
            destFile.remove();
            return;
        }
        bytesWritten += wrt;
        emit percentCopied(bytesWritten*1.0/srcSize);
    }
    srcFile.close();
    destFile.close();
    emit finished();
}

void FileCopier::cancelCopying()
{
    m_canceled = true;
}

QString FileCopier::srcFileName() const
{
    return srcFile.fileName();
}

QString FileCopier::destFileName() const
{
    return destFile.fileName();
}

void FileCopier::setSrcFileName(const QString &fileName)
{
    srcFile.setFileName(fileName);
}

void FileCopier::setDestFileName(const QString &fileName)
{
    destFile.setFileName(fileName);
}
