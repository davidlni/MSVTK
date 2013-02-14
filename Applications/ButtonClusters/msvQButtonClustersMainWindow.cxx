/*==============================================================================

  Library: MSVTK

  Copyright (c) Kitware Inc.

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

// Qt includes
#include <QDebug>
#include <QFileDialog>
#include <QMessageBox>
#include <QString>
#include <unistd.h>

// MSV includes
#include "msvQButtonClustersMainWindow.h"
#include "msvVTKWidgetClusters.h"
#include "ui_msvQButtonClustersMainWindow.h"
#include "msvQButtonClustersAboutDialog.h"

// VTK includes
#include "vtkActor.h"
#include "vtkAppendPolyData.h"
#include "vtkAxesActor.h"
#include "vtkAxis.h"
#include "vtkCamera.h"
#include "vtkColorTransferFunction.h"
#include "vtkGenericOpenGLRenderWindow.h"
#include "vtkNew.h"
#include "vtkOrientationMarkerWidget.h"
#include "vtkPiecewiseFunction.h"
#include "vtkPNGReader.h"
#include "vtkPolyDataReader.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkRenderer.h"
#include "vtkRendererCollection.h"
#include "vtkSmartVolumeMapper.h"
#include "vtkSmartPointer.h"
#include "vtkStructuredPoints.h"
#include "vtkStructuredPointsReader.h"
#include "vtkVolumeProperty.h"
#include "vtkVolume.h"


// ------------------------------------------------------------------------------
class msvQButtonClustersMainWindowPrivate
  : public Ui_msvQButtonClustersMainWindow
{
  Q_DECLARE_PUBLIC(msvQButtonClustersMainWindow);
protected:
  msvQButtonClustersMainWindow* const q_ptr;

  // Scene Rendering
  vtkSmartPointer<vtkOrientationMarkerWidget> OrientationMarker;
  vtkSmartPointer<vtkAxesActor>               Axes;
  vtkSmartPointer<vtkRenderer>                ThreeDRenderer;

  // Segmentation Pipeline
  vtkSmartPointer<vtkAppendPolyData> PolyDataMerger;

  // Volume Pipeline
  vtkSmartPointer<vtkPiecewiseFunction>     ScalarOpacity;
  vtkSmartPointer<vtkPiecewiseFunction>     GradientOpacity;
  vtkSmartPointer<vtkColorTransferFunction> ColorTransferFunction;
  vtkSmartPointer<vtkVolumeProperty>        VolumeProperty;

  vtkSmartPointer<vtkPolyDataMapper> SurfaceMapper;
  vtkSmartPointer<vtkActor>          SurfaceActor;

  std::vector<vtkVolume*> VolumeList;

  // buttonsManager
  vtkSmartPointer<msvVTKWidgetClusters> ButtonsManager;
  vtkSmartPointer<vtkPNGReader>         buttonsIcon;

  class EndInteractionCallbackCommand;
  EndInteractionCallbackCommand *EndInteractionCommand;

public:
  msvQButtonClustersMainWindowPrivate(msvQButtonClustersMainWindow& object);
  ~msvQButtonClustersMainWindowPrivate();

  virtual void setup(QMainWindow*);
  virtual void setupUi(QMainWindow*);
  virtual void setupView();
  virtual void update();
  virtual void updateUi();
  virtual void updateView();

  virtual void showVolume(bool value);
  virtual void showGroup(int value);
  virtual void showDiscs(bool value);
  virtual void enableClustering(bool value);
  virtual void setPixelRadius(double value);

  virtual void clear();

  virtual void readData(const QString&);
  virtual void readDataFiles(const QString&, int&, int);
  virtual void readDataFile(const QString&, int&, int);

  void readPolyData(const QString&, int idx, int group);
  void readVolumeData(const QString&, int idx, int group);

};

// ------------------------------------------------------------------------------
// msvQButtonClustersMainWindowPrivate methods

// ------------------------------------------------------------------------------
class msvQButtonClustersMainWindowPrivate::EndInteractionCallbackCommand :
  public vtkCommand
{
public:
  static EndInteractionCallbackCommand *New()
  {
    return new EndInteractionCallbackCommand;
  };
  msvQButtonClustersMainWindowPrivate *Self;

  void Execute(vtkObject *, unsigned long, void *)
  {
    if (this->Self)
      {
      this->Self->update();
      }
  }
protected:
  EndInteractionCallbackCommand() {
    this->Self = NULL;
  };
  ~EndInteractionCallbackCommand() {
  };
};

// ------------------------------------------------------------------------------
msvQButtonClustersMainWindowPrivate::msvQButtonClustersMainWindowPrivate(
  msvQButtonClustersMainWindow& object)
  : q_ptr(&object)
{
  // Renderer
  this->ThreeDRenderer = vtkSmartPointer<vtkRenderer>::New();
  this->ThreeDRenderer->SetBackground(0.1, 0.2, 0.4);
  this->ThreeDRenderer->SetBackground2(0.2, 0.4, 0.8);
  this->ThreeDRenderer->SetGradientBackground(true);

  this->Axes = vtkSmartPointer<vtkAxesActor>::New();

  this->OrientationMarker = vtkSmartPointer<vtkOrientationMarkerWidget>::New();
  this->OrientationMarker->SetOutlineColor(0.9300, 0.5700, 0.1300);
  this->OrientationMarker->SetOrientationMarker(Axes);

  // Data merger
  this->PolyDataMerger = vtkSmartPointer<vtkAppendPolyData>::New();

  // Create Pipeline for the Points
  this->SurfaceMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
  this->SurfaceMapper->ScalarVisibilityOff();
  this->SurfaceMapper->SetInputConnection(
    this->PolyDataMerger->GetOutputPort());
  this->SurfaceActor = vtkSmartPointer<vtkActor>::New();
  this->SurfaceActor->GetProperty()->SetColor(226. / 255., 93. /255., 94. /
    255.);
  this->SurfaceActor->GetProperty()->BackfaceCullingOn();
  this->SurfaceActor->SetMapper(this->SurfaceMapper);

  // Volume props
  this->ScalarOpacity = vtkSmartPointer<vtkPiecewiseFunction>::New();
  this->ScalarOpacity->AddPoint(20.0,0.0);
  this->ScalarOpacity->AddPoint(500.0,0.15);
  this->ScalarOpacity->AddPoint(1000.0,0.15);
  this->ScalarOpacity->AddPoint(1150.0,0.85);

  this->GradientOpacity = vtkSmartPointer<vtkPiecewiseFunction>::New();
  this->GradientOpacity->AddPoint(0.0,0.0);
  this->GradientOpacity->AddPoint(90.0,0.5);
  this->GradientOpacity->AddPoint(100.0,0.1);

  this->ColorTransferFunction =
    vtkSmartPointer<vtkColorTransferFunction>::New();
  this->ColorTransferFunction->AddRGBPoint(0.0,0.0,0.0,0.0);
  this->ColorTransferFunction->AddRGBPoint(500,  1.0, 0.5, 0.3);
  this->ColorTransferFunction->AddRGBPoint(1000, 1.0, 0.5, 0.3);
  this->ColorTransferFunction->AddRGBPoint(1150, 1.0, 1.0, 0.9);

  this->VolumeProperty = vtkSmartPointer<vtkVolumeProperty>::New();
  this->VolumeProperty->SetColor(this->ColorTransferFunction);
  this->VolumeProperty->SetScalarOpacity(this->ScalarOpacity);
  this->VolumeProperty->SetGradientOpacity(this->GradientOpacity);
  this->VolumeProperty->SetInterpolationTypeToLinear();
  this->VolumeProperty->ShadeOn();
  this->VolumeProperty->SetAmbient(0.4);
  this->VolumeProperty->SetDiffuse(0.6);
  this->VolumeProperty->SetSpecular(0.2);

  // Set the buttons manager
  this->buttonsIcon = vtkSmartPointer<vtkPNGReader>::New();
  this->buttonsIcon->SetFileName("Resources/Logo/icon.png");
  this->buttonsIcon->Update();
  this->ButtonsManager = vtkSmartPointer<msvVTKWidgetClusters>::New();
  this->ButtonsManager->UseImprovedClusteringOn();
  this->ButtonsManager->ClusteringWithinGroupsOn();
  this->ButtonsManager->ShiftWidgetCenterToCornerOff();
  this->ButtonsManager->SetButtonIcon(this->buttonsIcon->GetOutput());
  // Set interaction callback to track when interaction ended
  this->EndInteractionCommand       = EndInteractionCallbackCommand::New();
  this->EndInteractionCommand->Self = this;
  this->ButtonsManager->AddObserver(vtkCommand::UpdateDataEvent,
    this->EndInteractionCommand);

}

// ------------------------------------------------------------------------------
msvQButtonClustersMainWindowPrivate::~msvQButtonClustersMainWindowPrivate()
{
  this->clear();
  if(this->EndInteractionCommand)
    {
    this->EndInteractionCommand->Delete();
    }
}

// ------------------------------------------------------------------------------
void msvQButtonClustersMainWindowPrivate::showDiscs(bool value)
{
  this->SurfaceActor->SetVisibility(value);
  this->updateView();
}

// ------------------------------------------------------------------------------
void msvQButtonClustersMainWindowPrivate::showVolume(bool value)
{
  for(size_t i = 0, end = this->VolumeList.size(); i < end; ++i)
    {
    this->VolumeList[i]->SetVisibility(value);
    }
  this->updateView();
}

// ------------------------------------------------------------------------------
void msvQButtonClustersMainWindowPrivate::enableClustering(bool value)
{
  this->ButtonsManager->SetClustering(value);
  if(value)
    {
    this->ButtonsManager->HideButtons();
    }
  else
    {
    this->ButtonsManager->ShowButtons();
    }
  this->ButtonsManager->UpdateWidgets();
  this->updateView();
}

// ------------------------------------------------------------------------------
void msvQButtonClustersMainWindowPrivate::showGroup(int value)
{
  int numberOfLevels = this->ButtonsManager->GetNumberOfLevels();
  for(int i = 0; i < numberOfLevels; ++i)
    {
    if(i == value)
      {
      this->ButtonsManager->ShowClusterButtons(i);
      }
    else
      {
      this->ButtonsManager->HideClusterButtons(i);
      }
    }
  this->updateView();
}

// ------------------------------------------------------------------------------
void msvQButtonClustersMainWindowPrivate::setPixelRadius(double value)
{
  this->ButtonsManager->SetPixelRadius(value);
  this->ButtonsManager->UpdateWidgets();
  this->updateView();
}

// ------------------------------------------------------------------------------
void msvQButtonClustersMainWindowPrivate::clear()
{
  this->ThreeDRenderer->RemoveAllViewProps(); // clean up the renderer
  this->ButtonsManager->Clear(); // clean up the buttonsManager
  this->VolumeList.clear();
}

// ------------------------------------------------------------------------------
void msvQButtonClustersMainWindowPrivate::setup(QMainWindow * mainWindow)
{
  this->setupUi(mainWindow);
  this->setupView();
}

// ------------------------------------------------------------------------------
void msvQButtonClustersMainWindowPrivate::setupUi(QMainWindow * mainWindow)
{
  Q_Q(msvQButtonClustersMainWindow);

  this->Ui_msvQButtonClustersMainWindow::setupUi(mainWindow);

  this->vtkButtonsReviewPanel->toggleViewAction()->setText(
    "Clusters review panel");
  this->vtkButtonsReviewPanel->toggleViewAction()->setShortcut(QKeySequence(
      "Ctrl+1"));
  this->menuView->addAction(this->vtkButtonsReviewPanel->toggleViewAction());

  q->setStatusBar(0);

  // Connect Menu ToolBars actions
  q->connect(this->actionOpen, SIGNAL(triggered()), q, SLOT(openData()));
  q->connect(this->actionClose, SIGNAL(triggered()), q, SLOT(closeData()));
  q->connect(this->actionExit, SIGNAL(triggered()), q, SLOT(close()));
  q->connect(this->actionAboutButtonClustersApplication, SIGNAL(triggered()),
    q, SLOT(aboutApplication()));

  // Customize QAction icons with standard pixmaps
  QIcon dirIcon         = q->style()->standardIcon(QStyle::SP_DirIcon);
  QIcon informationIcon = q->style()->standardIcon(
    QStyle::SP_MessageBoxInformation);

  this->actionOpen->setIcon(dirIcon);
  this->actionAboutButtonClustersApplication->setIcon(informationIcon);

}

// ------------------------------------------------------------------------------
void msvQButtonClustersMainWindowPrivate::setupView()
{
  this->threeDView->GetRenderWindow()->AddRenderer(this->ThreeDRenderer);

  // Marker annotation
  this->OrientationMarker->SetInteractor
    (this->ThreeDRenderer->GetRenderWindow()->GetInteractor());
  this->OrientationMarker->SetEnabled(1);
  this->OrientationMarker->InteractiveOff();

  this->ButtonsManager->SetRenderer(this->ThreeDRenderer);
}

// ------------------------------------------------------------------------------
void msvQButtonClustersMainWindowPrivate::update()
{
  this->updateUi();
  this->updateView();
}

// ------------------------------------------------------------------------------
void msvQButtonClustersMainWindowPrivate::updateUi()
{
  this->enableClustering(this->EnableClustering->isChecked());
  this->setPixelRadius(this->PixelRadius->value());
}

// ------------------------------------------------------------------------------
void msvQButtonClustersMainWindowPrivate::updateView()
{
  this->threeDView->GetRenderWindow()->Render();
}

// ------------------------------------------------------------------------------
void msvQButtonClustersMainWindowPrivate::readVolumeData(const QString &file,
                                                         int idx, int group)
{
  // Set volume pipeline first.
  vtkNew<vtkStructuredPointsReader> reader;
  vtkNew<vtkSmartVolumeMapper>      volumeMapper;
  vtkNew<vtkVolume>                 volume;

  volumeMapper->SetInputConnection(reader->GetOutputPort());
  volume->SetMapper(volumeMapper.GetPointer());
  volume->SetProperty(this->VolumeProperty);
  this->ThreeDRenderer->AddViewProp(volume.GetPointer());

  reader->SetFileName(file.toLatin1().constData());
  reader->Update();

  // Create 8 buttons for the volume.
  vtkNew<vtkPoints> points;
  points->SetNumberOfPoints(8);

  double *bounds      = reader->GetOutput()->GetBounds();
  double  point[8][3] =
    {
                          {bounds[0], bounds[2], bounds[4]},
                          {bounds[0], bounds[2], bounds[5]},
                          {bounds[0], bounds[3], bounds[4]},
                          {bounds[0], bounds[3], bounds[5]},
                          {bounds[1], bounds[2], bounds[4]},
                          {bounds[1], bounds[2], bounds[5]},
                          {bounds[1], bounds[3], bounds[4]},
                          {bounds[1], bounds[3], bounds[5]}
    };

  for(vtkIdType i = 0; i < 8; ++i)
    {
    points->InsertPoint(i, point[i]);
    }
  this->ButtonsManager->SetDataSet(group,idx,points.GetPointer());
  this->VolumeList.push_back(volume.GetPointer());
}

// ------------------------------------------------------------------------------
void msvQButtonClustersMainWindowPrivate::readPolyData(const QString &file,
                                                       int idx, int group)
{
  vtkNew<vtkPolyDataReader> reader;
  vtkNew<vtkPoints>         points;

  reader->SetFileName(file.toLatin1().constData());
  reader->Update();

  // Pick ten points to place the buttons
  vtkIdType numPoints =
    reader->GetOutput()->GetPoints()->GetNumberOfPoints();
  const vtkIdType N = 10;
  numPoints = numPoints > N ? numPoints/N : numPoints/(N/2);

  for(vtkIdType i = 0; i < N; ++i)
    {
    double point[3] = {0};
    reader->GetOutput()->GetPoints()->GetPoint(i*numPoints,point);
    points->InsertNextPoint(point);
    }

  // Add points to the widget manager
  this->ButtonsManager->SetDataSet(group,idx,points.GetPointer());

  this->PolyDataMerger->AddInput(reader->GetOutput());
}

// ------------------------------------------------------------------------------
void msvQButtonClustersMainWindowPrivate::readDataFiles(const QString& dirName,
                                                        int &          idx,
                                                        int            group)
{
  QDir        dir(dirName);
  QStringList dataFiles = dir.entryList(QDir::Files, QDir::Name).filter("vtk");

  foreach(const QString &file, dataFiles)
    {
    QString fileName = QFileInfo(dir,file).absoluteFilePath();
    readDataFile(fileName,idx,group);
    }
}

// ------------------------------------------------------------------------------
void msvQButtonClustersMainWindowPrivate::readDataFile(const QString& fileName,
                                                       int &          idx,
                                                       int            group)
{
  vtkNew<vtkDataReader> reader;
  reader->SetFileName(fileName.toLatin1().constData());
  if(reader->IsFilePolyData())
    {
    readPolyData(fileName,idx,group);
    idx++;
    }
  else if(reader->IsFileStructuredPoints())
    {
//     readVolumeData(fileName,idx,group);
//     idx++;
    }
  else
    {
    return;
    }
}

// ------------------------------------------------------------------------------
void msvQButtonClustersMainWindowPrivate::readData(const QString& rootDirectory)
{
  QDir dir(rootDirectory);

  QStringList files =
    dir.entryList(QDir::Files, QDir::Name).filter("vtk");
  QStringList directories = dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot,
    QDir::Name);

  int idx   = 0;
  int group = 0;
  foreach(const QString &file, files)
    {
    QString fileName = QFileInfo(dir,file).absoluteFilePath();
    readDataFile(fileName,idx,group);
    }
  foreach(const QString &directory, directories)
    {
    readDataFiles(QFileInfo(dir,directory).absoluteFilePath(),idx,group);
    }

  // Render
  double extent[6];
  this->SurfaceMapper->GetBounds(extent);
  this->ThreeDRenderer->AddActor(this->SurfaceActor);
  this->ThreeDRenderer->ResetCamera(extent);

}

// ------------------------------------------------------------------------------
// msvQButtonClustersMainWindow methods

// ------------------------------------------------------------------------------
msvQButtonClustersMainWindow::msvQButtonClustersMainWindow(QWidget* parentWidget)
  : Superclass(parentWidget)
  , d_ptr(new msvQButtonClustersMainWindowPrivate(*this))
{
  Q_D(msvQButtonClustersMainWindow);
  d->setup(this);
}

// ------------------------------------------------------------------------------
msvQButtonClustersMainWindow::~msvQButtonClustersMainWindow()
{
}

// ------------------------------------------------------------------------------
void msvQButtonClustersMainWindow::openData()
{
  Q_D(msvQButtonClustersMainWindow);

  QString dir = QFileDialog::getExistingDirectory(
    this, tr("Select root data directory"), QDir::homePath(),
    QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
  if(dir.isEmpty())
    {
    return;
    }
//   if (!dir.contains("Spine"))
//     {
//     QMessageBox err;
//     err.setWindowTitle(tr("Error!"));
//     err.setText(tr("Wrong data path."));
//     err.setDetailedText(tr(
//         "Make sure you are pointing to the correct directory.\n"
//         "The directory should contain the following directory:\n"
//         "Segmentation"));
//     err.exec();
//     return;
//     }

  d->clear(); // Clean Up data and scene
  d->readData(dir); // Load data
  d->update(); // Update the Ui and the View
}


// ------------------------------------------------------------------------------
void msvQButtonClustersMainWindow::on_ShowVolume_stateChanged(int state)
{
  Q_D(msvQButtonClustersMainWindow);

  d->showVolume(state);
}

// ------------------------------------------------------------------------------
void msvQButtonClustersMainWindow::on_ShowDiscs_stateChanged(int state)
{
  Q_D(msvQButtonClustersMainWindow);

  d->showDiscs(state);
}

// ------------------------------------------------------------------------------
void msvQButtonClustersMainWindow::on_EnableClustering_stateChanged(int state)
{
  Q_D(msvQButtonClustersMainWindow);

  d->enableClustering(state);
}

// ------------------------------------------------------------------------------
void msvQButtonClustersMainWindow::on_PixelRadius_valueChanged(double value)
{
  Q_D(msvQButtonClustersMainWindow);

  d->setPixelRadius(value);
}

// ------------------------------------------------------------------------------
void msvQButtonClustersMainWindow::closeData()
{
  Q_D(msvQButtonClustersMainWindow);

  d->clear();
  d->update();
}

// ------------------------------------------------------------------------------
void msvQButtonClustersMainWindow::aboutApplication()
{
  msvQButtonClustersAboutDialog about(this);
  about.exec();
}

// ------------------------------------------------------------------------------
void msvQButtonClustersMainWindow::updateView()
{
  Q_D(msvQButtonClustersMainWindow);

  d->updateView();
}









