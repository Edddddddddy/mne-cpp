#include "rticasettingsview.h"
#include "ui_rticasettingsview.h"

#include <QSettings>

using namespace DISPLIB;

RtIcaSettingsView::RtIcaSettingsView(const QString& sSettingsPath,
                                     QWidget *parent,
                                     Qt::WindowFlags f)
: AbstractView(parent, f)
, m_pUi(new Ui::RtIcaSettingsViewWidget)
, m_sSettingsPath(sSettingsPath)
{
    m_pUi->setupUi(this);
    this->setWindowTitle("RT-ICA Settings");

    loadSettings();

    connect(m_pUi->m_pCheckBoxActivateRtIca, &QCheckBox::toggled,
            this, &RtIcaSettingsView::onRtIcaActivationChanged);
}

RtIcaSettingsView::~RtIcaSettingsView()
{
    saveSettings();
    delete m_pUi;
}

void RtIcaSettingsView::saveSettings()
{
    if(m_sSettingsPath.isEmpty()) {
        return;
    }

    QSettings settings("MNECPP");
    settings.setValue(m_sSettingsPath + QString("/RtIcaSettingsView/rtIcaActivated"), m_pUi->m_pCheckBoxActivateRtIca->isChecked());
}

void RtIcaSettingsView::loadSettings()
{
    if(m_sSettingsPath.isEmpty()) {
        return;
    }

    QSettings settings("MNECPP");
    m_pUi->m_pCheckBoxActivateRtIca->setChecked(settings.value(m_sSettingsPath + QString("/RtIcaSettingsView/rtIcaActivated"), false).toBool());
}

void RtIcaSettingsView::updateGuiMode(GuiMode mode)
{
    Q_UNUSED(mode)
}

void RtIcaSettingsView::updateProcessingMode(ProcessingMode mode)
{
    Q_UNUSED(mode)
}

void RtIcaSettingsView::clearView()
{
}

bool RtIcaSettingsView::getRtIcaActive()
{
    return m_pUi->m_pCheckBoxActivateRtIca->isChecked();
}

void RtIcaSettingsView::onRtIcaActivationChanged(bool bChecked)
{
    emit rtIcaActivationChanged(bChecked);
    saveSettings();
}
