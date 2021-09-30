#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QMouseEvent>
#include <AES.h>
#include <fstream>
#include <QPixmap>
#include <QFileDialog>
#include <QMessageBox>
#include <QProgressDialog>
#include <QProgressBar>
#include <QElapsedTimer>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void mousePressEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);
    void mouseMoveEvent(QMouseEvent *event);

    void on_closeButton_clicked();
    void on_minimizeButton_clicked();

    void on_encryptButton_clicked();

    void on_fileButton_clicked();

    void on_decryptButton_clicked();

    void on_keyText_textChanged();

private:
    Ui::MainWindow *ui;
    bool isMousePressed;
    QPoint mStartPos;
    QPixmap imgPixMap;
    AES aes;
    std::string to_hex(int i);unsigned char defaultIV[17] = {'A','B','C','B','E','F','G','G','A','B','C','B','G','H','I','J','\n'};

};
#endif // MAINWINDOW_H
