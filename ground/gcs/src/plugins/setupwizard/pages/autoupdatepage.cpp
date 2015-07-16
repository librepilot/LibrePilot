#include "autoupdatepage.h"
#include "ui_autoupdatepage.h"
#include "setupwizard.h"
#include <extensionsystem/pluginmanager.h>
#include <uavobjectutil/uavobjectutilmanager.h>
#include <extensionsystem/pluginmanager.h>
#include "uploader/uploadergadgetfactory.h"

AutoUpdatePage::AutoUpdatePage(SetupWizard *wizard, QWidget *parent) :
    AbstractWizardPage(wizard, parent),
    ui(new Ui::AutoUpdatePage), m_isUpdating(false)
{
    ui->setupUi(this);
    ExtensionSystem::PluginManager *pm = ExtensionSystem::PluginManager::instance();
    Q_ASSERT(pm);
    UploaderGadgetFactory *uploader    = pm->getObject<UploaderGadgetFactory>();
    Q_ASSERT(uploader);
    connect(ui->startUpdate, SIGNAL(clicked()), this, SLOT(disableButtons()));
    connect(ui->startUpdate, SIGNAL(clicked()), this, SLOT(autoUpdate()));
    connect(uploader, SIGNAL(progressUpdate(uploader::ProgressStep, QVariant)), this, SLOT(updateStatus(uploader::ProgressStep, QVariant)));
}

AutoUpdatePage::~AutoUpdatePage()
{
    delete ui;
}

bool AutoUpdatePage::isComplete() const
{
    return !m_isUpdating;
}

void AutoUpdatePage::enableButtons(bool enable = false)
{
    ui->startUpdate->setEnabled(enable);
    getWizard()->button(QWizard::NextButton)->setEnabled(enable);
    getWizard()->button(QWizard::CancelButton)->setEnabled(enable);
    getWizard()->button(QWizard::BackButton)->setEnabled(enable);
    getWizard()->button(QWizard::CustomButton1)->setEnabled(enable);
    QApplication::processEvents();
}

void AutoUpdatePage::autoUpdate()
{
    ExtensionSystem::PluginManager *pm = ExtensionSystem::PluginManager::instance();

    Q_ASSERT(pm);
    UploaderGadgetFactory *uploader    = pm->getObject<UploaderGadgetFactory>();
    Q_ASSERT(uploader);
    m_isUpdating = true;
    uploader->autoUpdate(ui->eraseSettings->isChecked());
    getWizard()->setRestartNeeded(true);
}

void AutoUpdatePage::updateStatus(uploader::ProgressStep status, QVariant value)
{
    switch (status) {
    case uploader::WAITING_DISCONNECT:
        disableButtons();
        ui->statusLabel->setText(tr("Waiting for all OP boards to be disconnected."));
        // TODO get rid of magic number 20s (should use UploaderGadgetWidget::BOARD_EVENT_TIMEOUT)
        ui->levellinProgressBar->setMaximum(20);
        ui->levellinProgressBar->setValue(value.toInt());
        break;
    case uploader::WAITING_CONNECT:
        disableButtons();
        ui->statusLabel->setText(tr("Please connect the board to the USB port (don't use external supply)."));
        // TODO get rid of magic number 20s (should use UploaderGadgetWidget::BOARD_EVENT_TIMEOUT)
        ui->levellinProgressBar->setMaximum(20);
        ui->levellinProgressBar->setValue(value.toInt());
        break;
    case uploader::JUMP_TO_BL:
        disableButtons();
        ui->levellinProgressBar->setValue(value.toInt());
        ui->levellinProgressBar->setMaximum(5);
        ui->statusLabel->setText(tr("Board going into bootloader mode. Please wait."));
        break;
    case uploader::LOADING_FW:
        ui->statusLabel->setText(tr("Loading firmware."));
        break;
    case uploader::UPLOADING_FW:
        ui->statusLabel->setText(tr("Uploading firmware."));
        ui->levellinProgressBar->setMaximum(100);
        ui->levellinProgressBar->setValue(value.toInt());
        break;
    case uploader::UPLOADING_DESC:
        ui->statusLabel->setText(tr("Uploading description."));
        break;
    case uploader::BOOTING:
        ui->statusLabel->setText(tr("Booting the board. Please wait"));
        break;
    case uploader::BOOTING_AND_ERASING:
        ui->statusLabel->setText(tr("Booting and erasing the board. Please wait"));
        break;
    case uploader::SUCCESS:
        m_isUpdating = false;
        enableButtons(true);
        ui->statusLabel->setText(tr("Board updated, please press 'Next' to continue."));
        break;
    case uploader::FAILURE:
        m_isUpdating = false;
        enableButtons(true);
        QString msg = value.toString();
        if (msg.isEmpty()) {
            msg = tr("Something went wrong.");
        }
        msg += tr(" You will have to manually upgrade the board using the uploader plugin.");
        ui->statusLabel->setText(QString("<font color='red'>%1</font>").arg(msg));
        break;
    }
}
