set(ROI_SRC_FILES RoiPresenter.cpp QtRoiView.cpp)

# Include files aren't required, but this makes them appear in Visual Studio
# IMPORTANT: Include files are required in the MOC_FILES set. Scroll down to
# find it.
set(ROI_INC_FILES
    RoiPresenter.h
    IRoiPresenter.h
    IRoiView.h
    QtRoiView.h
    RoiPresenterFactory.h)

set(ROI_MOC_FILES QtRoiView.h)

set(ROI_UI_FILES RoiWidget.ui)

prepend(ROI_SRC_FILES GUI/RoiSelector ${ROI_SRC_FILES})
prepend(ROI_INC_FILES GUI/RoiSelector ${ROI_INC_FILES})
prepend(ROI_MOC_FILES GUI/RoiSelector ${ROI_MOC_FILES})
prepend(ROI_UI_FILES GUI/RoiSelector ${ROI_UI_FILES})

set(ROI_SRC_FILES ${ROI_SRC_FILES} PARENT_SCOPE)
set(ROI_INC_FILES ${ROI_INC_FILES} PARENT_SCOPE)
set(ROI_MOC_FILES ${ROI_MOC_FILES} PARENT_SCOPE)
set(ROI_UI_FILES ${ROI_UI_FILES} PARENT_SCOPE)
