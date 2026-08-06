//=============================================================================================================
/**
 * @file     adaptivedenoising_global.h
 * @brief    Adaptive Denoising plugin export and build-information declarations.
 */

#ifndef ADAPTIVEDENOISING_GLOBAL_H
#define ADAPTIVEDENOISING_GLOBAL_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <utils/buildinfo.h>

#include <QtCore/qglobal.h>

//=============================================================================================================
// PREPROCESSOR DEFINES
//=============================================================================================================

#if defined(SCAN_ADAPTIVEDENOISING_PLUGIN)
#  define ADAPTIVEDENOISINGSHARED_EXPORT Q_DECL_EXPORT
#else
#  define ADAPTIVEDENOISINGSHARED_EXPORT Q_DECL_IMPORT
#endif

//=============================================================================================================
// DEFINE NAMESPACE ADAPTIVEDENOISINGPLUGIN
//=============================================================================================================

namespace ADAPTIVEDENOISINGPLUGIN
{

ADAPTIVEDENOISINGSHARED_EXPORT const char* buildDateTime();
ADAPTIVEDENOISINGSHARED_EXPORT const char* buildHash();
ADAPTIVEDENOISINGSHARED_EXPORT const char* buildHashLong();

} // NAMESPACE

#endif // ADAPTIVEDENOISING_GLOBAL_H
