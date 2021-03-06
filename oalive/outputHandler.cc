/*****************************************************************************
 *
 * outputHandler.cc -- output hander (mostly) virtual class
 *
 * Copyright 2013,2014,2017,2018 James Fidell (james@openastroproject.org)
 *
 * License:
 *
 * This file is part of the Open Astro Project.
 *
 * The Open Astro Project is free software: you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as published
 * by the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * The Open Astro Project is distributed in the hope that it will be
 * useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with the Open Astro Project.  If not, see
 * <http://www.gnu.org/licenses/>.
 *
 *****************************************************************************/

#include <oa_common.h>

#include <time.h>

#include "outputHandler.h"
#include "state.h"


#ifdef OACAPTURE
OutputHandler::OutputHandler ( int x, int y, int n, int d )
#else
OutputHandler::OutputHandler ( int x, int y, int n, int d,
    QString fileTemplate )
#endif
{
  Q_UNUSED( x );
  Q_UNUSED( y );
  Q_UNUSED( n );
  Q_UNUSED( d );

#ifdef OACAPTURE
  generateFilename();
#else
  filenameTemplate = fileTemplate;
#endif
}


void
OutputHandler::generateFilename ( void )
{
  struct tm*	tm;
  time_t	epochSecs;

  epochSecs = time(0);
  tm = localtime ( &epochSecs );

  QString seconds;
  seconds.setNum ( tm->tm_sec );
  if ( tm->tm_sec < 10 ) {
    seconds = "0" + seconds;
  }
  QString minutes;
  minutes.setNum ( tm->tm_min );
  if ( tm->tm_min < 10 ) {
    minutes = "0" + minutes;
  }
  QString hours;
  hours.setNum ( tm->tm_hour );
  if ( tm->tm_hour < 10 ) {
    hours = "0" + hours;
  }
  QString day;
  day.setNum ( tm->tm_mday );
  if ( tm->tm_mday < 10 ) {
    day = "0" + day;
  }
  QString month;
  month.setNum ( tm->tm_mon + 1 );
  if ( tm->tm_mon < 9 ) {
    month = "0" + month;
  }
  QString year;
  year.setNum ( tm->tm_year + 1900 );
  QString epoch;
  epoch.setNum ( epochSecs );
  QString date = year + month + day;
  QString time = hours + minutes + seconds;

  QString index, gain, exposureMs, exposureS;
  unsigned int exposure;
  index = QString("%1").arg ( state.captureIndex, config.indexDigits, 10,
      QChar('0'));
#ifdef OACAPTURE
  gain = QString("%1").arg ( state.controlWidget->getCurrentGain());
  exposure = state.controlWidget->getCurrentExposure();
#else
  gain = QString("%1").arg ( state.cameraControls->getCurrentGain());
  exposure = state.cameraControls->getCurrentExposure();
#endif
  exposureMs = QString("%1").arg ( exposure / 1000 );
  exposureS = QString("%1").arg (( int ) ( exposure / 1000000 ));

#ifdef OACAPTURE
  filename = config.fileNameTemplate;
#else
  filename = filenameTemplate;
#endif

  filename.replace ( "%DATE", date );
  filename.replace ( "%TIME", time );
  filename.replace ( "%YEAR", year );
  filename.replace ( "%MONTH", month );
  filename.replace ( "%DAY", day );
  filename.replace ( "%HOUR", hours );
  filename.replace ( "%MINUTE", minutes );
  filename.replace ( "%SECOND", seconds );
  filename.replace ( "%EPOCH", epoch );
#ifdef OACAPTURE
  // FIX ME -- add support for these back in oalive
  filename.replace ( "%FILTER", state.captureWidget->getCurrentFilterName());
  filename.replace ( "%PROFILE",
      state.captureWidget->getCurrentProfileName());
#endif
  filename.replace ( "%INDEX", index );
  filename.replace ( "%GAIN", gain );
  filename.replace ( "%EXPMS", exposureMs );
  filename.replace ( "%EXPS", exposureS );

  filename.replace ( "%U", date + "-" + time );
  filename.replace ( "%D", date );
  filename.replace ( "%T", time );
  filename.replace ( "%Y", year );
  filename.replace ( "%M", month );
  filename.replace ( "%d", day );
  filename.replace ( "%H", hours );
  filename.replace ( "%M", minutes );
  filename.replace ( "%S", seconds );
  filename.replace ( "%I", index );

  filename.replace ( "%E", epoch );
  filename.replace ( "%G", gain );
  filename.replace ( "%x", exposureMs );
  filename.replace ( "%X", exposureS );

  if ( filename[0] != '/' ) {
    if ( config.captureDirectory != "" ) {
      filename = config.captureDirectory + "/" + filename;
    } else {
      filename = state.currentDirectory + "/" + filename;
    }
  }

  fullSaveFilePath = "";
}


unsigned int
OutputHandler::getFrameCount ( void )
{
  return frameCount;
}


QString
OutputHandler::getFilename ( void )
{
  return filename;
}


QString
OutputHandler::getNewFilename ( void )
{
  generateFilename();
  return filename;
}


QString
OutputHandler::getRecordingFilename ( void )
{
  return fullSaveFilePath;
}


QString
OutputHandler::getRecordingBasename ( void )
{
  return filenameRoot;
}
