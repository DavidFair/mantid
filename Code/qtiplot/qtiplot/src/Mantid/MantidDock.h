#ifndef MANTIDDOCK_H
#define MANTIDDOCK_H

#include <QDockWidget>
#include <QTreeWidget>
#include <QComboBox>
#include <QPoint>
#include "MantidAPI/Workspace.h"

class MantidUI;
class ApplicationWindow;
class QLabel;
class QMenu;
class QPushButton;


class MantidDockWidget: public QDockWidget
{
    Q_OBJECT
public:
    MantidDockWidget(QWidget*w):QDockWidget(w){}
    MantidDockWidget(MantidUI *mui, ApplicationWindow *w);
public slots:
    void clickedWorkspace(QTreeWidgetItem*, int);
    void deleteWorkspaces();
    void clearWorkspaceTree();			   
protected slots:
    void popupMenu(const QPoint & pos);
private slots:
  void updateWorkspaceEntry(const QString &, Mantid::API::Workspace_sptr);
  void removeWorkspaceEntry(const QString &);
  
protected:
    QTreeWidget *m_tree;
    friend class MantidUI;
private:
    QPushButton *m_loadButton;
    QPushButton *m_deleteButton;
    MantidUI *m_mantidUI;
};


class MantidTreeWidget:public QTreeWidget
{
    Q_OBJECT
public:
    MantidTreeWidget(QWidget *w, MantidUI *mui):QTreeWidget(w),m_mantidUI(mui)
    {
      setSelectionMode(QAbstractItemView::ExtendedSelection);
    }
    void mousePressEvent (QMouseEvent *e);
    void mouseMoveEvent(QMouseEvent *e);
    void mouseDoubleClickEvent(QMouseEvent *e);

private:
    QPoint m_dragStartPosition;
    MantidUI *m_mantidUI;
};

class FindAlgComboBox:public QComboBox
{
    Q_OBJECT
signals:
    void enterPressed();
protected:
    void keyPressEvent(QKeyEvent *e);
};

class AlgorithmDockWidget: public QDockWidget
{
    Q_OBJECT
public:
    AlgorithmDockWidget(MantidUI *mui, ApplicationWindow *w);
public slots:
    void update();
    void findAlgTextChanged(const QString& text);
    void treeSelectionChanged();
    void selectionChanged(const QString& algName);
    void countChanged(int n);
    void tst();
protected:
    QTreeWidget *m_tree;
    FindAlgComboBox* m_findAlg;
    QLabel *m_runningAlgsLabel;
    bool m_treeChanged;
    bool m_findAlgChanged;
    friend class MantidUI;
private:
    MantidUI *m_mantidUI;
};


class AlgorithmTreeWidget:public QTreeWidget
{
    Q_OBJECT
public:
    AlgorithmTreeWidget(QWidget *w, MantidUI *mui):QTreeWidget(w),m_mantidUI(mui){}
    void mousePressEvent (QMouseEvent *e);
    void mouseMoveEvent(QMouseEvent *e);
    void mouseDoubleClickEvent(QMouseEvent *e);
private:
    QPoint m_dragStartPosition;
    MantidUI *m_mantidUI;
};

#endif
