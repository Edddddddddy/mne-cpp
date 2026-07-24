//=============================================================================================================
/**
 * @file     aoemeg_global.h
 * @author   OpenAI Codex
 * @date     March, 2026
 *
 * @brief    Contains the AOE-MEG plugin export/import macros.
 *
 */

#ifndef AOEMEG_GLOBAL_H
#define AOEMEG_GLOBAL_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <utils/buildinfo.h>

#include <QtCore/qglobal.h>

//=============================================================================================================
// PREPROCESSOR DEFINES
//=============================================================================================================

#if defined(SCAN_AOEMEG_PLUGIN)
#  define AOEMEGSHARED_EXPORT Q_DECL_EXPORT
#else
#  define AOEMEGSHARED_EXPORT Q_DECL_IMPORT
#endif

namespace AOEMEGPLUGIN
{

AOEMEGSHARED_EXPORT const char* buildDateTime();
AOEMEGSHARED_EXPORT const char* buildHash();
AOEMEGSHARED_EXPORT const char* buildHashLong();

} // namespace AOEMEGPLUGIN

#endif // AOEMEG_GLOBAL_H
