#include "failsafechannelform.h"
#include "ui_failsafechannelform.h"

FailsafeChannelForm::FailsafeChannelForm(const int index, QWidget *parent) :
    ChannelForm(index, parent), ui(new Ui::FailsafeChannelForm)
{
    ui->setupUi(this);
    disableMouseWheelEvents();
}

FailsafeChannelForm::~FailsafeChannelForm()
{
    delete ui;
}

QString FailsafeChannelForm::name()
{
    return ui->channelName->text();
}

void FailsafeChannelForm::setName(const QString &name)
{
    ui->channelName->setText(name);
}
