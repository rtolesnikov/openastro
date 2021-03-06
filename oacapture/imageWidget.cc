/*****************************************************************************
 *
 * imageWidget.cc -- class for the image controls in the UI
 *
 * Copyright 2013,2014,2015,2016,2018
 *     James Fidell (james@openastroproject.org)
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

#include <QtGui>

extern "C" {
#include <openastro/camera.h>
}

#include "configuration.h"
#include "imageWidget.h"
#include "controlWidget.h"
#include "state.h"


ImageWidget::ImageWidget ( QWidget* parent ) : QGroupBox ( parent )
{
  setTitle ( tr ( "Image" ));

  grid = new QGridLayout ( this );

  cameraROILabel = new QLabel ( tr ( "Camera ROI" ));
  userROI = new QCheckBox ( tr ( "User ROI" ));
  userROI->setChecked ( false );
  userROI->setToolTip ( tr ( "Select your own region of interest" ));
  cropRegion = new QCheckBox ( tr ( "Crop to size" ));
  cropRegion->setChecked ( false );
  cropRegion->setToolTip ( tr ( "Choose the output frame size" ));

  ignoreResolutionChanges = 0;
  resMenu = new QComboBox ( this );
  QStringList resolutions;
  resolutions << "640x480" << "1280x960";
  resMenu->addItems ( resolutions );
  connect ( resMenu, SIGNAL( currentIndexChanged ( int )), this,
      SLOT( cameraROIChanged ( int )));

  roiXSize = new QLineEdit ( this );
  roiYSize = new QLineEdit ( this );
  roiBy = new QLabel ( " x ", this );

  roiXSize->setMaxLength ( 4 );
  roiXSize->setFixedWidth ( 45 );
  roiYSize->setMaxLength ( 4 );
  roiYSize->setFixedWidth ( 45 );
  QString xStr, yStr;
  if ( config.imageSizeX > 0 ) {
    xStr = QString::number ( config.imageSizeX );
  } else {
    xStr = "";
    config.imageSizeX = 0;
  }
  roiXSize->setText ( xStr );
  if ( config.imageSizeY > 0 ) {
    yStr = QString::number ( config.imageSizeY );
  } else {
    yStr = "";
    config.imageSizeY = 0;
  }
  roiYSize->setText ( yStr );
  SET_PROFILE_CONFIG( imageSizeX, config.imageSizeX );
  SET_PROFILE_CONFIG( imageSizeY, config.imageSizeY );

  roiButton = new QPushButton (
      QIcon ( ":/qt-icons/roi.png" ), "", this );
  roiButton->setToolTip ( tr ( "Set new ROI" ));
  connect ( roiButton, SIGNAL( clicked()), this, SLOT( setUserROI()));

  roiInputBox = new QHBoxLayout();
  roiInputBox->addWidget ( roiXSize );
  roiInputBox->addWidget ( roiBy );
  roiInputBox->addWidget ( roiYSize );
  roiInputBox->addWidget ( roiButton );

  cropXSize = new QLineEdit ( this );
  cropYSize = new QLineEdit ( this );
  cropBy = new QLabel ( " x ", this );

  cropXSize->setMaxLength ( 4 );
  cropXSize->setFixedWidth ( 45 );
  cropYSize->setMaxLength ( 4 );
  cropYSize->setFixedWidth ( 45 );

  cropButton = new QPushButton (
      QIcon ( ":/qt-icons/roi.png" ), "", this );
  cropButton->setToolTip ( tr ( "Set new crop size" ));
  connect ( cropButton, SIGNAL( clicked()), this, SLOT( setCropSize()));

  cropInputBox = new QHBoxLayout();
  cropInputBox->addWidget ( cropXSize );
  cropInputBox->addWidget ( cropBy );
  cropInputBox->addWidget ( cropYSize );
  cropInputBox->addWidget ( cropButton );

  roiXValidator = 0;
  roiYValidator = 0;
  cropXValidator = 0;
  cropYValidator = 0;
  state.cropMode = 0;

  grid->addWidget ( cameraROILabel, 0, 0 );
  grid->addWidget ( userROI, 0, 1 );
  grid->addWidget ( cropRegion, 0, 2 );
  grid->addWidget ( resMenu, 1, 0 );
  grid->addLayout ( roiInputBox, 1, 1 );
  grid->addLayout ( cropInputBox, 1, 2 );

  setLayout ( grid );

  connect ( userROI, SIGNAL( clicked()), this, SLOT( updateUserROI()));
  connect ( cropRegion, SIGNAL( clicked()), this, SLOT( updateFrameCrop()));
}


ImageWidget::~ImageWidget()
{
  state.mainWindow->destroyLayout (( QLayout* ) grid );
}


void
ImageWidget::configure ( void )
{
  int maxX, maxY;
  maxX = maxY = 0;

  // This includes a particularly ugly way to sort the resolutions using
  // a QMap and some further QMap abuse to be able to find the X and Y
  // resolutions should we not have a setting that matches the config and
  // happen to choose the max size option
  const FRAMESIZES* sizeList = state.camera->frameSizes();
  QMap<int,QString> sortMap;
  QMap<int,int> xRes, yRes;
  QString showItemStr;
  int firstKey = -1, lastKey = -1;
  unsigned int i;

  for ( i = 0; i < sizeList->numSizes; i++ ) {
    int numPixels = sizeList->sizes[i].x * sizeList->sizes[i].y;
    QString resStr = QString::number ( sizeList->sizes[i].x ) + "x" +
        QString::number ( sizeList->sizes[i].y );
    if ( sizeList->sizes[i].x == config.imageSizeX &&
        sizeList->sizes[i].y == config.imageSizeY ) {
      showItemStr = resStr;
    }
    sortMap [ numPixels ] = resStr;
    xRes [ numPixels ] = sizeList->sizes[i].x;
    yRes [ numPixels ] = sizeList->sizes[i].y;
    if ( lastKey < numPixels ) {
      lastKey = numPixels;
    }
    if ( -1 == firstKey || firstKey > numPixels ) {
      firstKey = numPixels;
    }
  }

  int numItems = 0, showItem = -1, showXRes = -1, showYRes = -1;
  QStringList resolutions;
  QMap<int,QString>::iterator j;
  QMap<int,int>::iterator x, y;
  XResolutions.clear();
  YResolutions.clear();
  for ( j = sortMap.begin(), x = xRes.begin(), y = yRes.begin();
      j != sortMap.end(); j++, x++, y++ ) {
    resolutions << *j;
    XResolutions << *x;
    YResolutions << *y;
    if ( *j == showItemStr ) {
      showItem = numItems;
      showXRes = *x;
      showYRes = *y;
    }
    numItems++;
  }

  ignoreResolutionChanges = 1;
  resMenu->clear();
  resMenu->addItems ( resolutions );
  ignoreResolutionChanges = 0;

  if ( showItem >= 0 ) {
    config.imageSizeX = showXRes;
    config.imageSizeY = showYRes;
  } else {
    showItem = ( !userROI->isChecked() && !cropRegion->isChecked()) ?
        numItems - 1: 0;
    if ( showItem ) {
      config.imageSizeX = xRes[ lastKey ];
      config.imageSizeY = yRes[ lastKey ];
    } else {
      config.imageSizeX = xRes[ firstKey ];
      config.imageSizeY = yRes[ firstKey ];
    }
  }
  maxX = xRes[ lastKey ];
  maxY = yRes[ lastKey ];
  state.sensorSizeX = maxX;
  state.sensorSizeY = maxY;
  SET_PROFILE_CONFIG( imageSizeX, config.imageSizeX );
  SET_PROFILE_CONFIG( imageSizeY, config.imageSizeY );

  // There's a gotcha here for cameras that only support a single
  // resolution, as the index won't actually change, and the slot
  // won't get called.  So, we have to get the current index and
  // if it is 0, call resolution changed manually
  if ( resMenu->currentIndex() == showItem ) {
    cameraROIChanged ( showItem );
  } else {
    resMenu->setCurrentIndex ( showItem );
  }
  // xSize->setText ( QString::number ( config.imageSizeX ));
  // ySize->setText ( QString::number ( config.imageSizeY ));

  if ( !roiXValidator ) {
    roiXValidator = new QIntValidator ( 1, maxX, this );
    roiYValidator = new QIntValidator ( 1, maxY, this );
    roiXSize->setValidator ( roiXValidator );
    roiYSize->setValidator ( roiYValidator );
    cropXValidator = new QIntValidator ( 1, maxX, this );
    cropYValidator = new QIntValidator ( 1, maxY, this );
    cropXSize->setValidator ( roiXValidator );
    cropYSize->setValidator ( roiYValidator );
  }
  roiXValidator->setRange ( 1, maxX );
  roiYValidator->setRange ( 1, maxY );
  cropXValidator->setRange ( 1, maxX );
  cropYValidator->setRange ( 1, maxY );

  if ( state.cropMode ) {
    if ( config.imageSizeX > state.cropSizeX || config.imageSizeY > state.
        cropSizeY ) {
      state.cropMode = 0;
      cropRegion->setChecked ( false );
    }
  }

  // If the camera has fixed frame sizes the we want to display the crop
  // label for the frame size selector.  Otherwise it's the User ROI label
  if ( state.camera->hasFixedFrameSizes()) {
    userROI->setEnabled(0);
    roiXSize->setEnabled(0);
    roiYSize->setEnabled(0);
    roiBy->setEnabled(0);
    roiButton->setEnabled(0);
  } else {
    userROI->setEnabled(1);
    roiXSize->setEnabled(1);
    roiYSize->setEnabled(1);
    roiBy->setEnabled(1);
    roiButton->setEnabled(1);
  }
}


void
ImageWidget::cameraROIChanged ( int index )
{
  // changes to this function may need to be replicated in
  // updateFromConfig()
  if ( !state.camera || ignoreResolutionChanges ||
      !state.camera->isInitialised()) {
    return;
  }

  userROI->setChecked ( false );

  config.imageSizeX = XResolutions[ index ];
  config.imageSizeY = YResolutions[ index ];
  // xSize->setText ( QString::number ( config.imageSizeX ));
  // ySize->setText ( QString::number ( config.imageSizeY ));
  doResolutionChange ( 0 );
}


void
ImageWidget::doResolutionChange ( int roiChanged )
{
  // state.camera->delayFrameRateChanges();
  if ( state.controlWidget ) {
    state.controlWidget->updateFrameRates();
  }
  if ( roiChanged ) {
    state.camera->setROI ( config.imageSizeX, config.imageSizeY );
  } else {
    state.camera->setResolution ( config.imageSizeX, config.imageSizeY );
  }
  if ( state.previewWidget ) {
    state.previewWidget->updatePreviewSize();
  }
  SET_PROFILE_CONFIG( imageSizeX, config.imageSizeX );
  SET_PROFILE_CONFIG( imageSizeY, config.imageSizeY );
}


void
ImageWidget::enableAllControls ( int state )
{
  userROI->setEnabled ( state );
  cropRegion->setEnabled ( state );
  resMenu->setEnabled ( state );
  roiButton->setEnabled ( state );
  cropButton->setEnabled ( state );
}


void
ImageWidget::resetResolution ( void )
{
  cameraROIChanged ( resMenu->currentIndex());
}


void
ImageWidget::updateFromConfig ( void )
{
  if ( !state.camera || !state.camera->isInitialised()) {
    return;
  }
  int numRes = resMenu->count();
  if ( numRes ) {
    int index = 0;
    for ( int i = 0; i < numRes && 0 == index; i++ ) {
      if ( XResolutions[ i ] == config.imageSizeX && YResolutions[ i ] ==
          config.imageSizeY ) {
        index = i;
      }
    }

    // need to ignore changes here because changing the index will
    // call cameraROIChanged() and we're doing its work ourselves
    ignoreResolutionChanges = 1;
    resMenu->setCurrentIndex ( index );
    ignoreResolutionChanges = 0;
    // xSize->setText ( QString::number ( config.imageSizeX ));
    // ySize->setText ( QString::number ( config.imageSizeY ));

    // if ( state.camera->isInitialised()) {
      // state.camera->delayFrameRateChanges();
    // }
    if ( state.controlWidget ) {
      state.controlWidget->updateFrameRates();
    }
    if ( state.camera->isInitialised()) {
      state.camera->setResolution ( config.imageSizeX, config.imageSizeY );
    }
    if ( state.previewWidget ) {
      state.previewWidget->updatePreviewSize();
    }
  }
}


void
ImageWidget::updateUserROI ( void )
{
  if ( userROI->isChecked()) {
    setUserROI();
  } else {
    // if the button is unchecked then we return to whatever is set in the
    // camera ROI
    cameraROIChanged ( resMenu->currentIndex());
  }
}


void
ImageWidget::setUserROI ( void )
{
  QString xStr = roiXSize->text();
  QString yStr = roiYSize->text();
  int x, y;

  x = y = -1;
  if ( xStr != "" ) {
    x = xStr.toInt();
  }
  if ( yStr != "" ) {
    y = yStr.toInt();
  }

  if ( x > 0 && y > 0 ) {
    unsigned int altX, altY;
    // To prevent issues with mosaicked frames make the new X and Y values
    // multiples of 2
    x = x + ( x % 2 );
    y = y + ( y % 2 );
    if ( state.camera->testROISize ( x, y, &altX, &altY )) {
      x = altX;
      y = altY;
    }
    config.imageSizeX = x;
    config.imageSizeY = y;
    roiYSize->setText ( QString::number ( config.imageSizeX ));
    roiYSize->setText ( QString::number ( config.imageSizeY ));
    doResolutionChange ( 1 );
    userROI->setChecked ( true );
  } else {
    userROI->setChecked ( false );
  }

  if ( state.cropMode ) {
    if ( config.imageSizeX > state.cropSizeX || config.imageSizeY > state.
        cropSizeY ) {
      state.cropMode = 0;
      cropRegion->setChecked ( false );
    }
  }
}


void
ImageWidget::updateFrameCrop ( void )
{
  if ( cropRegion->isChecked()) {
    setCropSize();
  } else {
    state.cropMode = 0;
  }
}


void
ImageWidget::setCropSize ( void )
{
  QString xStr = cropXSize->text();
  QString yStr = cropYSize->text();
  unsigned int x, y;

  x = y = 0;
  if ( xStr != "" ) {
    x = xStr.toInt();
  }
  if ( yStr != "" ) {
    y = yStr.toInt();
  }

  if ( x > 0 && y > 0 ) {
    // To prevent issues with mosaicked frames make the new X and Y values
    // multiples of 2
    x = x + ( x % 2 );
    y = y + ( y % 2 );
    if ( x <= config.imageSizeX && y <= config.imageSizeY ) {
      cropRegion->setChecked ( true );
      state.cropSizeX = x;
      state.cropSizeY = y;
      state.cropMode = 1;
    }
  } else {
    state.cropMode = 0;
    cropRegion->setChecked ( false );
  }
}
