//=============================================================================================================
/**
 * @file     aoemegsetupwidget.h
 * @author   OpenAI Codex
 * @date     March, 2026
 *
 * @brief    Contains the declaration of the AOEMegSetupWidget class.
 *
 */

#ifndef AOEMEGSETUPWIDGET_H
#define AOEMEGSETUPWIDGET_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <QtWidgets>

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

namespace AOEMEGPLUGIN
{

class AOEMeg;

class AOEMegSetupWidget : public QWidget
{
    Q_OBJECT

public:
    AOEMegSetupWidget(AOEMeg* pAoeMeg, QWidget* parent = 0);
    ~AOEMegSetupWidget();

private:
    AOEMeg*         m_pAoeMeg;
    QLabel*         m_pSummaryLabel;
};

} // namespace AOEMEGPLUGIN

#endif // AOEMEGSETUPWIDGET_H
