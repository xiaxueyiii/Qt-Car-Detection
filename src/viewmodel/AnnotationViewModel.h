#ifndef ANNOTATIONVIEWMODEL_H
#define ANNOTATIONVIEWMODEL_H

#include "../model/DataTypes.h"

#include <QObject>
#include <QSize>

class AnnotationViewModel : public QObject
{
    Q_OBJECT

public:
    explicit AnnotationViewModel(QObject *parent = nullptr);

    bool loadImageFolder(const QString &folderPath);
    QStringList imagePaths() const;
    QString currentImagePath() const;
    int currentIndex() const;
    void setCurrentIndex(int index);
    void next();
    void previous();

    QVector<AnnotationBox> loadLabels(const QSize &imageSize) const;
    bool saveLabels(const QVector<AnnotationBox> &boxes, const QSize &imageSize);
    QString labelPathForImage(const QString &imagePath) const;

    void setClassNames(const QStringList &classNames);
    QString className(int classId) const;

signals:
    void currentImageChanged(const QString &path);
    void logMessage(const QString &message);

private:
    QStringList findImages(const QString &folderPath) const;

private:
    QStringList m_images;
    int m_currentIndex = -1;
    QStringList m_classNames;
};

#endif // ANNOTATIONVIEWMODEL_H
