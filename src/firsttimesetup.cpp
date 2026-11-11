#include "firsttimesetup.h"
#include "ui_firsttimesetup.h"
#include <QGuiApplication>
#include <QLayout>
#include <QLabel>
#include <QMetaObject>
#include <QScrollArea>
#include <QScroller>
#include <QScreen>
#include <QShowEvent>
#include <QSizePolicy>
#include <QVBoxLayout>

FirstTimeSetup::FirstTimeSetup(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::FirstTimeSetup)
{
#ifdef Q_OS_IOS
    QWidget *content = new QWidget;
    ui->setupUi(content);
#else
    ui->setupUi(this);
#endif
#ifdef Q_OS_IOS
    ui->serialPortThisPC->hide();
    ui->USBPortThisPC->hide();
    ui->WiFiNetwork->setChecked(true);
    isNetwork = true;

    // The desktop welcome form is 518 px wide, which is wider than an iPhone
    // in portrait. Let it use the current screen bounds and re-fit after a
    // device rotation instead of clipping its controls.
    setMinimumSize(0, 0);
    if (content->layout()) {
        content->layout()->setContentsMargins(12, 12, 12, 12);
        content->layout()->setSpacing(8);
    }
    content->setMinimumSize(0, 0);
    content->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
    for (QLabel* label : content->findChildren<QLabel*>())
        label->setWordWrap(true);

    auto *scrollArea = new QScrollArea(this);
    scrollArea->setFrameShape(QFrame::NoFrame);
    scrollArea->setWidgetResizable(true);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    scrollArea->setWidget(content);
    auto *dialogLayout = new QVBoxLayout(this);
    dialogLayout->setContentsMargins(0, 0, 0, 0);
    dialogLayout->addWidget(scrollArea);
    QScroller::grabGesture(scrollArea->viewport(), QScroller::TouchGesture);
    QMetaObject::connectSlotsByName(this);

    connect(QGuiApplication::primaryScreen(), &QScreen::geometryChanged,
            this, &FirstTimeSetup::fitToScreen);
#endif

    setWindowFlags(Qt::Dialog
        | Qt::FramelessWindowHint);

#ifdef Q_OS_IOS
    if (QScreen* screen = QGuiApplication::primaryScreen())
        fitToScreen(screen->geometry());
#endif

    this->setupState = setupInitial;
    ui->step2GroupBox->setHidden(true);
    ui->backBtn->setHidden(true);

    serialText1 = QString(tr("Serial Port Name"));
    serialText2 = QString(tr("Baud Rate"));
    serialText3 = QString("");

    networkText1 = QString(tr("Radio IP address, UDP Port Numbers"));
    networkText2 = QString(tr("Radio Username, Radio Password"));
    networkText3 = QString(tr("Mic and Speaker on THIS PC"));

}

#ifdef Q_OS_IOS
void FirstTimeSetup::fitToScreen(const QRect& geometry)
{
    if (!geometry.isValid())
        return;

    setMinimumSize(0, 0);
    setGeometry(geometry);
    setWindowState(windowState() | Qt::WindowFullScreen);
    if (layout())
        layout()->activate();
}

void FirstTimeSetup::showEvent(QShowEvent* event)
{
    QDialog::showEvent(event);
    if (QScreen* screen = QGuiApplication::primaryScreen())
        fitToScreen(screen->geometry());
}
#endif

FirstTimeSetup::~FirstTimeSetup()
{
    delete ui;
}

void FirstTimeSetup::on_exitProgramBtn_clicked()
{
    emit exitProgram();
    this->close();
}


void FirstTimeSetup::on_nextBtn_clicked()
{
    switch (setupState) {
    case setupInitial:
        // go to step 2:
        ui->nextBtn->setText(tr("Finish"));
        ui->nextBtn->setAccessibleName(tr("Finish Welcome Screen and proceed to Settings"));
        ui->step1GroupBox->setHidden(true);
        ui->step2GroupBox->setHidden(false);
        ui->neededDetailsLabel2->setHidden(false);
        if(isNetwork) {
            ui->neededDetailsLabel1->setText(networkText1);
            ui->neededDetailsLabel2->setText(networkText2);
            ui->neededDetailsLabel3->setText(networkText3);
        } else {
            ui->neededDetailsLabel1->setText(serialText1);
            ui->neededDetailsLabel2->setText(serialText2);
            ui->neededDetailsLabel3->setText(serialText3);
        }
        ui->backBtn->setHidden(false);
        setupState = setupStep2;
        break;
    case setupStep2:
        // Done
        emit showSettings(isNetwork);
        this->close();
        break;
    default:
        break;
    }
}

void FirstTimeSetup::on_onMyOwnBtn_clicked()
{
    emit skipSetup();
    this->close();
}


void FirstTimeSetup::on_ethernetNetwork_clicked(bool checked)
{
    this->isNetwork = checked;
}


void FirstTimeSetup::on_WiFiNetwork_clicked(bool checked)
{
    this->isNetwork = checked;
}


void FirstTimeSetup::on_USBPortThisPC_clicked(bool checked)
{
    this->isNetwork = !checked;
}


void FirstTimeSetup::on_serialPortThisPC_clicked(bool checked)
{
    this->isNetwork = !checked;
}


void FirstTimeSetup::on_backBtn_clicked()
{
    setupState = setupInitial;
    ui->nextBtn->setText(tr("Next"));
    ui->nextBtn->setAccessibleName(tr("Next Step"));
    ui->step1GroupBox->setHidden(false);
    ui->step2GroupBox->setHidden(true);
    ui->backBtn->setHidden(true);
}

