#include <MainWindow.h>

#include <QtWidgets>

MainWindow::MainWindow()
  : m_menu_importer( "org.gnome.Terminal.Display_0", "/com/canonical/unity/gtk/window/0" )
{
  QEventLoop menu_appeared_wait;
  menu_appeared_wait.connect( &m_menu_importer, SIGNAL( MenuAppeared() ), SLOT( quit() ) );
  menu_appeared_wait.exec();

  m_menu = m_menu_importer.GetQMenu();
  menuBar()->addMenu( m_menu.get() );
}
