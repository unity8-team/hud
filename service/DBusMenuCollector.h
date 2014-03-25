/*
 * Copyright (C) 2013 Canonical, Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3, as published
 * by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranties of
 * MERCHANTABILITY, SATISFACTORY QUALITY, or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Author: Pete Woods <pete.woods@canonical.com>
 */

#ifndef HUD_SERVICE_DBUSMENUCOLLECTOR_H_
#define HUD_SERVICE_DBUSMENUCOLLECTOR_H_

#include <service/Collector.h>

#include <QDBusConnection>
#include <memory>

class ComCanonicalAppMenuRegistrarInterface;
class DBusMenuImporter;

QT_BEGIN_NAMESPACE
class QMenu;
QT_END_NAMESPACE

namespace hud {
namespace service {

class DBusMenuCollector: public Collector, public std::enable_shared_from_this<
		DBusMenuCollector> {
Q_OBJECT
public:
	typedef std::shared_ptr<DBusMenuCollector> Ptr;

	DBusMenuCollector(unsigned int windowId,
			QSharedPointer<ComCanonicalAppMenuRegistrarInterface> appmenu);

	virtual ~DBusMenuCollector();

	virtual bool isValid() const override;

	virtual QList<CollectorToken::Ptr> activate() override;

protected Q_SLOTS:
	void WindowRegistered(uint windowId, const QString &service,
			const QDBusObjectPath &menuObjectPath);

protected:
	virtual void deactivate();

	void windowRegistered(const QString &service,
			const QDBusObjectPath &menuObjectPath);

protected:
	void openMenu(QMenu *menu, unsigned int &limit);

	void hideMenu(QMenu *menu);

	unsigned int m_windowId;

	QSharedPointer<ComCanonicalAppMenuRegistrarInterface> m_registrar;

	QWeakPointer<CollectorToken> m_collectorToken;

	QSharedPointer<DBusMenuImporter> m_menuImporter;

	QString m_service;

	QDBusObjectPath m_path;
};

}
}

#endif /* HUD_SERVICE_DBUSMENUCOLLECTOR_H_ */
