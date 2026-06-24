#include "AnnotationViewModel.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QImageReader>
#include <QRegularExpression>
#include <QTextStream>

AnnotationViewModel::AnnotationViewModel(QObject *parent)
    : QObject(parent)
{
}

bool AnnotationViewModel::loadImageFolder(const QString &folderPath)
{
    m_images = findImages(folderPath);
    m_currentIndex = m_images.isEmpty() ? -1 : 0;
    emit logMessage(QString("加载图片文件夹：%1，图片数量：%2").arg(folderPath).arg(m_images.size()));
    if (m_currentIndex >= 0) {
        emit currentImageChanged(currentImagePath());
    }
    return !m_images.isEmpty();
}

QStringList AnnotationViewModel::imagePaths() const
{
    return m_images;
}

QString AnnotationViewModel::currentImagePath() const
{
    if (m_currentIndex < 0 || m_currentIndex >= m_images.size()) {
        return QString();
    }
    return m_images.at(m_currentIndex);
}

int AnnotationViewModel::currentIndex() const
{
    return m_currentIndex;
}

void AnnotationViewModel::setCurrentIndex(int index)
{
    if (index < 0 || index >= m_images.size() || index == m_currentIndex) {
        return;
    }
    m_currentIndex = index;
    emit currentImageChanged(currentImagePath());
}

void AnnotationViewModel::next()
{
    if (m_images.isEmpty()) {
        return;
    }
    setCurrentIndex(qMin(m_currentIndex + 1, m_images.size() - 1));
}

void AnnotationViewModel::previous()
{
    if (m_images.isEmpty()) {
        return;
    }
    setCurrentIndex(qMax(m_currentIndex - 1, 0));
}

QVector<AnnotationBox> AnnotationViewModel::loadLabels(const QSize &) const
{
    QVector<AnnotationBox> boxes;
    const QString labelPath = labelPathForImage(currentImagePath());
    QFile file(labelPath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return boxes;
    }

    QTextStream in(&file);
    while (!in.atEnd()) {
        const QString line = in.readLine().trimmed();
        if (line.isEmpty()) {
            continue;
        }
        const QStringList parts = line.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
        if (parts.size() < 5) {
            continue;
        }
        AnnotationBox box;
        box.classId = parts.at(0).toInt();
        box.className = className(box.classId);
        const double cx = parts.at(1).toDouble();
        const double cy = parts.at(2).toDouble();
        const double w = parts.at(3).toDouble();
        const double h = parts.at(4).toDouble();
        box.normalizedRect = QRectF(cx - w / 2.0, cy - h / 2.0, w, h).intersected(QRectF(0, 0, 1, 1));
        boxes << box;
    }
    return boxes;
}

bool AnnotationViewModel::saveLabels(const QVector<AnnotationBox> &boxes, const QSize &)
{
    const QString labelPath = labelPathForImage(currentImagePath());
    if (labelPath.isEmpty()) {
        emit logMessage("没有当前图片，无法保存标签。");
        return false;
    }

    QDir().mkpath(QFileInfo(labelPath).absolutePath());
    QFile file(labelPath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
        emit logMessage("保存标签失败：" + file.errorString());
        return false;
    }

    QTextStream out(&file);
    out.setRealNumberNotation(QTextStream::FixedNotation);
    out.setRealNumberPrecision(6);
    for (const AnnotationBox &box : boxes) {
        const QRectF rect = box.normalizedRect.normalized().intersected(QRectF(0, 0, 1, 1));
        const double cx = rect.x() + rect.width() / 2.0;
        const double cy = rect.y() + rect.height() / 2.0;
        out << box.classId << ' ' << cx << ' ' << cy << ' ' << rect.width() << ' ' << rect.height() << '\n';
    }
    emit logMessage("YOLO 标签已保存：" + labelPath);
    return true;
}

QString AnnotationViewModel::labelPathForImage(const QString &imagePath) const
{
    if (imagePath.isEmpty()) {
        return QString();
    }

    QFileInfo imageInfo(imagePath);
    QDir imageDir = imageInfo.absoluteDir();
    if (imageDir.dirName().compare("images", Qt::CaseInsensitive) == 0) {
        imageDir.cdUp();
        return imageDir.filePath("labels/" + imageInfo.completeBaseName() + ".txt");
    }
    return imageInfo.absolutePath() + "/" + imageInfo.completeBaseName() + ".txt";
}

void AnnotationViewModel::setClassNames(const QStringList &classNames)
{
    m_classNames = classNames;
}

QString AnnotationViewModel::className(int classId) const
{
    if (classId >= 0 && classId < m_classNames.size()) {
        return m_classNames.at(classId);
    }
    return QString("class_%1").arg(classId);
}

QStringList AnnotationViewModel::findImages(const QString &folderPath) const
{
    QStringList images;
    QDir dir(folderPath);
    if (!dir.exists()) {
        return images;
    }

    const QStringList filters = {"*.jpg", "*.jpeg", "*.png", "*.bmp", "*.webp"};
    const QFileInfoList files = dir.entryInfoList(filters, QDir::Files, QDir::Name);
    for (const QFileInfo &file : files) {
        images << QDir::fromNativeSeparators(file.absoluteFilePath());
    }
    return images;
}
