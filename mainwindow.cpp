#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    ui->tickmarkLabel->setVisible(false);
    ui->keyText->setPlaceholderText("Enter your key");
    isMousePressed = false;
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::mousePressEvent(QMouseEvent *event)
{
    QLabel *child = static_cast<QLabel*>(childAt(event->pos()));
    if(child == ui->titlebar)
    {
        isMousePressed = true;
        mStartPos = event->pos();
    }
}

void MainWindow::mouseReleaseEvent(QMouseEvent *event)
{
    QLabel *child = static_cast<QLabel*>(childAt(event->pos()));
    if(child == ui->titlebar)
    {
        isMousePressed = false;
    }
}

void MainWindow::mouseMoveEvent(QMouseEvent *event)
{
    if(isMousePressed)
    {
        QPoint deltaPos = event->pos() - mStartPos;
        this->move(this->pos()+deltaPos);
    }
}

void MainWindow::on_closeButton_clicked()
{
    this->close();
}

void MainWindow::on_minimizeButton_clicked()
{
    this->showMinimized();
}

void MainWindow::on_fileButton_clicked()
{
    QString fname = QFileDialog::getOpenFileName(NULL, tr("Open Image File"), "", tr("All Files (*)"));
    imgPixMap.load(fname, NULL, Qt::ColorOnly|Qt::ThresholdDither|Qt::AvoidDither|Qt::NoOpaqueDetection|Qt::NoFormatConversion);
    ui->pictureLabel->setPixmap(imgPixMap.scaled(200,150,Qt::KeepAspectRatioByExpanding));
}

void MainWindow::on_encryptButton_clicked()
{
    if(imgPixMap.isNull())
    {
        QMessageBox::critical(this, "Error!", "Please select an image.");
        return;
    }
    if(ui->keyText->toPlainText().size() > 16)
    {
        QMessageBox invKey;
        invKey.setIcon(QMessageBox::Icon::Warning);
        invKey.critical(this, "Error!", "The key you entered is invalid.\nThe length of key must be less than or equal to 16.");
        return;
    }
    else if(ui->keyText->toPlainText().size() == 0)
    {
        QMessageBox::critical(this, "Error!", "Please enter a key");
        return;
    }
    QImage img(imgPixMap.toImage());
    img = img.convertToFormat(QImage::Format_ARGB32);

    if(img.width() % 2 != 0)
        img = img.scaledToWidth(img.width()+1);

    QProgressDialog progress(NULL);
    progress.setWindowTitle("Please Wait");
    progress.setLabelText("Encrypting Image...");
    progress.setWindowFlags(Qt::Dialog | Qt::CustomizeWindowHint | Qt::WindowTitleHint);
    progress.setWindowModality(Qt::WindowModal);
    progress.setMinimumDuration(0);
    QProgressBar pbar(this);
    pbar.setAlignment(Qt::AlignCenter);
    pbar.setMinimum(0);
    pbar.setMaximum(img.height());
    progress.setBar(&pbar);

    unsigned int outLen;
    unsigned char key[16];
    string k = ui->keyText->toPlainText().toLocal8Bit().constData();
    while(k.length() < 16)
        k.append(".");
    std::copy(k.begin(), k.end(), key);
    QImage encrypted_img(img.width()*2, img.height(), QImage::Format_ARGB32);
    //bool p = false;
    QElapsedTimer timer;
    timer.start();
    for(int i=0; i<img.height(); i++)
    {
        progress.setValue(i);
        if(progress.wasCanceled())
            return;
        QRgb *line = reinterpret_cast<QRgb*>(img.scanLine(i));
        for(int j=0, ienc=0; j<img.width(); j+=2, ienc+=4)
        {
            unsigned char color[] = {(unsigned char)qRed(line[j]), (unsigned char)qGreen(line[j]),
                                     (unsigned char)qBlue(line[j]), (unsigned char)qAlpha(line[j]),
                                     (unsigned char)qRed(line[j+1]), (unsigned char)qGreen(line[j+1]),
                                     (unsigned char)qBlue(line[j+1]), (unsigned char)qAlpha(line[j+1])};

            unsigned char *e = aes.EncryptCBC(color, sizeof(color), key, (unsigned char*)defaultIV, outLen);
            vector<string> h = aes.getHexArray(e, outLen);
            QColor c1(stoi(h[0], NULL, 16), stoi(h[1], NULL, 16), stoi(h[2], NULL, 16), stoi(h[3], NULL, 16));
            QColor c2(stoi(h[4], NULL, 16), stoi(h[5], NULL, 16), stoi(h[6], NULL, 16), stoi(h[7], NULL, 16));
            QColor c3(stoi(h[8], NULL, 16), stoi(h[9], NULL, 16), stoi(h[10], NULL, 16), stoi(h[11], NULL, 16));
            QColor c4(stoi(h[12], NULL, 16), stoi(h[13], NULL, 16), stoi(h[14], NULL, 16), stoi(h[15], NULL, 16));
            encrypted_img.setPixelColor(ienc, i, c1);
            encrypted_img.setPixelColor(ienc+1, i, c2);
            encrypted_img.setPixelColor(ienc+2, i, c3);
            encrypted_img.setPixelColor(ienc+3, i, c4);
        }
    }
    progress.setValue(img.height());
    QString fname = QFileDialog::getSaveFileName(this, "Save Encrypted Image", QString(), tr("PNG (*.png)"));
    encrypted_img.save(fname, "png");
}

void MainWindow::on_decryptButton_clicked()
{
    if(imgPixMap.isNull())
    {
        QMessageBox::critical(this, "Error!", "Please select an image.");
        return;
    }
    if(ui->keyText->toPlainText().size() > 16)
    {
        QMessageBox::critical(this, "Error!", "The key you entered is invalid.\nThe length of key must be less than or equal to 16.");
        return;
    }
    else if(ui->keyText->toPlainText().size() == 0)
    {
        QMessageBox::critical(this, "Error!", "Please enter a key");
        return;
    }
    QImage img(imgPixMap.toImage());
    img = img.convertToFormat(QImage::Format_ARGB32);

    if(img.width() % 4 != 0)
    {
        QMessageBox::critical(this, "Error!", "Incompatible Image.\nPlease choose an encrypted image.");
        return;
    }

    QProgressDialog progress(NULL);
    progress.setWindowTitle("Please Wait");
    progress.setLabelText("Decrypting Image...");
    progress.setWindowFlags(Qt::Dialog | Qt::CustomizeWindowHint | Qt::WindowTitleHint);
    progress.setWindowModality(Qt::WindowModal);
    progress.setMinimumDuration(0);
    QProgressBar pbar(this);
    pbar.setAlignment(Qt::AlignCenter);
    pbar.setMinimum(0);
    pbar.setMaximum(img.height());
    progress.setBar(&pbar);

    unsigned char key[16];
    string k = ui->keyText->toPlainText().toLocal8Bit().constData();
    while(k.length() < 16)
        k.append(".");
    std::copy(k.begin(), k.end(), key);
    QImage decrypted_img(img.width()/2, img.height(), QImage::Format_ARGB32);
    QElapsedTimer timer;
    timer.start();
    for(int i=0; i<img.height(); i++)
    {
        progress.setValue(i);
        QRgb *line = reinterpret_cast<QRgb*>(img.scanLine(i));
        for(int j=0, idec=0; j<img.width(); j+=4, idec+=2)
        {
            unsigned char color[] = {(unsigned char)qRed(line[j]), (unsigned char)qGreen(line[j]),
                                     (unsigned char)qBlue(line[j]), (unsigned char)qAlpha(line[j]),
                                     (unsigned char)qRed(line[j+1]), (unsigned char)qGreen(line[j+1]),
                                     (unsigned char)qBlue(line[j+1]), (unsigned char)qAlpha(line[j+1]),
                                     (unsigned char)qRed(line[j+2]), (unsigned char)qGreen(line[j+2]),
                                     (unsigned char)qBlue(line[j+2]), (unsigned char)qAlpha(line[j+2]),
                                     (unsigned char)qRed(line[j+3]), (unsigned char)qGreen(line[j+3]),
                                     (unsigned char)qBlue(line[j+3]), (unsigned char)qAlpha(line[j+3])};
            unsigned char *d = aes.DecryptCBC(color, sizeof(color), key, (unsigned char*)defaultIV);
            QColor c1(d[0], d[1], d[2], d[3]);
            QColor c2(d[4], d[5], d[6], d[7]);
            decrypted_img.setPixelColor(idec, i, c1);
            decrypted_img.setPixelColor(idec+1, i, c2);
        }
    }
    QString imgFormat;
    QString fname = QFileDialog::getSaveFileName(this, "Save Decrypted Image", QString(), tr("JPEG (*.jpg);;PNG (*.png);;BMP (*.bmp)"), &imgFormat);
    decrypted_img.save(fname);
}

string MainWindow::to_hex(int i)
{
  std::stringstream stream;
  stream << setfill('0')
         << std::setw(2)
         << std::hex << i;
  return stream.str();
}

void MainWindow::on_keyText_textChanged()
{
    if(ui->keyText->toPlainText().size() > 0 && ui->keyText->toPlainText().size() <= 16)
        ui->tickmarkLabel->setVisible(true);
    else
        ui->tickmarkLabel->setVisible(false);
}
