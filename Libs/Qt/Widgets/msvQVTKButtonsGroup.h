/*==============================================================================

  Library: MSVTK

  Copyright (c) SCS s.r.l. (B3C)

  Licensed under the Apache License, Version 2.0 (the "License");
  you may not use this file except in compliance with the License.
  You may obtain a copy of the License at

      http://www.apache.org/licenses/LICENSE-2.0.txt

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

==============================================================================*/

#ifndef msvQVTKButtonsGroup_H
#define msvQVTKButtonsGroup_H

// Includes list

#include "msvQtWidgetsExport.h"
#include "msvQVTKButtonsInterface.h"
#include <QVariant>

// forward declarations
class vtkRenderer;
class msvQVTKButtons;
class vtkSliderWidget;
class vtkSliderCallback;
class vtkCommand;

// Pimpl
class msvQVTKButtonsGroupPrivate;

/**
Class name: msvQVTKButtonsGroup
Manager class to manage groups of msvQVTKButtons
*/

class MSV_QT_WIDGETS_EXPORT msvQVTKButtonsGroup : public msvQVTKButtonsInterface
{
  Q_OBJECT

public:
  /// Object constructor.
  msvQVTKButtonsGroup(QObject *parent = 0);

  /// Object destructor.
  virtual ~msvQVTKButtonsGroup();

  /// Add a buttons to the buttons' vector
  void addElement(msvQVTKButtonsInterface* buttons);

  /// Remove a buttons to the buttons' vector
  void removeElement(msvQVTKButtonsInterface* buttons);

  /// Get the specified element
  msvQVTKButtonsInterface* getElement(int index);

  /// create a new element
  msvQVTKButtonsGroup* createGroup();

  /// create a new element
  msvQVTKButtons* createButtons();

  /// Allow to show/hide button
  void setShowButton(bool show);

  /// Allow to show/hide label
  void setShowLabel(bool show);

  /// set the icon path
  void setIconFileName(QString iconFileName);

  /// add vtk button to Renderer
  void setCurrentRenderer(vtkRenderer *renderer);

  /// Update graphic objects
  void update();

  /// Get the slider widget
  vtkSliderWidget* slider();

  /// Show/hide the slider widget
  void showSlider(bool show);

  /// Set the position on the path at the specified ratio
  void setCameraPoistionOnPath(double ratio);

  vtkCommand *getSliderCallback();

public slots:

  /// hide/show group
  void show(bool val);

protected:
  QScopedPointer<msvQVTKButtonsGroupPrivate> d_ptr;

  /// Set the specified property
  void setElementProperty(QString name, QVariant value);

  /// Calculate element position
  void calculatePosition();

//   QVector<msvQVTKButtonsInterface*> m_Elements; //< Vector of buttons
//   vtkSliderWidget* m_Slider; //< Slider widget
  vtkSliderCallback* m_SliderCallback; //< Slider callback function

private:
  Q_DECLARE_PRIVATE(msvQVTKButtonsGroup);
  Q_DISABLE_COPY(msvQVTKButtonsGroup);
};

/////////////////////////////////////////////////////////////
// Inline methods
/////////////////////////////////////////////////////////////

inline void msvQVTKButtonsGroup::setShowButton(bool show)
{
  msvQVTKButtonsInterface::setShowButton(show);
  setElementProperty("showButton",show);
}

inline void msvQVTKButtonsGroup::setShowLabel(bool show)
{
  msvQVTKButtonsInterface::setShowLabel(show);
  setElementProperty("showLabel",show);
}

inline void msvQVTKButtonsGroup::setIconFileName(QString iconFileName)
{
  msvQVTKButtonsInterface::setIconFileName(iconFileName);
  setElementProperty("iconFileName",iconFileName);
}

#endif // msvQVTKButtonsGroup_H
