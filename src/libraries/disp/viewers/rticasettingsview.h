#ifndef RTICASETTINGSVIEW_H
#define RTICASETTINGSVIEW_H

#include "../disp_global.h"
#include "abstractview.h"

namespace Ui {
class RtIcaSettingsViewWidget;
}

namespace DISPLIB {

class DISPSHARED_EXPORT RtIcaSettingsView : public AbstractView
{
    Q_OBJECT

public:
    typedef QSharedPointer<RtIcaSettingsView> SPtr;
    typedef QSharedPointer<const RtIcaSettingsView> ConstSPtr;

    explicit RtIcaSettingsView(const QString& sSettingsPath = "",
                               QWidget *parent = 0,
                               Qt::WindowFlags f = 0);
    ~RtIcaSettingsView();

    void saveSettings();
    void loadSettings();

    void updateGuiMode(GuiMode mode);
    void updateProcessingMode(ProcessingMode mode);

    void clearView();

    bool getRtIcaActive();

signals:
    void rtIcaActivationChanged(bool bActive);

private slots:
    void onRtIcaActivationChanged(bool bChecked);

private:
    Ui::RtIcaSettingsViewWidget *m_pUi;
    QString m_sSettingsPath;
};

} // NAMESPACE

#endif // RTICASETTINGSVIEW_H
