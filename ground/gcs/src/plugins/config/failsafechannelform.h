#ifndef FAILSAFECHANNELFORM_H
#define FAILSAFECHANNELFORM_H

#include "channelform.h"
#include "configinputwidget.h"

#include <QWidget>

namespace Ui {
class FailsafeChannelForm;
}

class FailsafeChannelForm : public ChannelForm {
    Q_OBJECT

public:
    explicit FailsafeChannelForm(const int index, QWidget *parent = NULL);
    ~FailsafeChannelForm();

    friend class ConfigInputWidget;

    virtual QString name();
    virtual void setName(const QString &name);

private:
    Ui::FailsafeChannelForm *ui;
};

#endif // FAILSAFECHANNELFORM_H
