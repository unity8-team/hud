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

#include <common/AppstackModel.h>
#include <common/DBusTypes.h>
#include <common/Localisation.h>
#include <common/ResultsModel.h>
#include <service/HudService.h>
#include <service/QueryImpl.h>
#include <service/QueryAdaptor.h>

#include <QStringList>

using namespace hud::common;
using namespace hud::service;

QueryImpl::QueryImpl(unsigned int id, const QString &query, HudService &service,
		const QDBusConnection &connection, QObject *parent) :
		Query(parent), m_adaptor(new QueryAdaptor(this)), m_connection(
				connection), m_path(DBusTypes::queryPath(id)), m_service(
				service), m_query(query) {
	if (!m_connection.registerObject(m_path.path(), this)) {
		throw std::logic_error(
				_("Unable to register HUD query object on DBus"));
	}

	//FIXME Dummy data
	{
		QList<QPair<int, int>> commandHighlights;
		commandHighlights << QPair<int, int>(1, 5);
		commandHighlights << QPair<int, int>(8, 9);

		QList<QPair<int, int>> descriptionHighlights;
		descriptionHighlights << QPair<int, int>(3, 4);

		m_results
				<< Result(0, "command 1", commandHighlights, "description 1",
						descriptionHighlights, "shortcut 1", 1, false);
	}

	{
		QList<QPair<int, int>> commandHighlights;
		commandHighlights << QPair<int, int>(2, 3);

		QList<QPair<int, int>> descriptionHighlights;
		descriptionHighlights << QPair<int, int>(4, 5);

		m_results
				<< Result(1, "command 2", commandHighlights, "description 2",
						descriptionHighlights, "shortcut 2", 2, false);
	}

	m_resultsModel.reset(new ResultsModel(id));
	m_appstackModel.reset(new AppstackModel(id));

	m_resultsModel->beginChangeset();
	for (const Result &result : m_results) {
		m_resultsModel->addResult(result.id(), result.commandName(),
				result.commandHighlights(), result.description(),
				result.descriptionHighlights(), result.shortcut(),
				result.distance(), result.parameterized());
	}
	m_resultsModel->endChangeset();

	//FIXME Dummy data
	m_appstackModel->beginChangeset();
	m_appstackModel->addApplication("test-app-1", "icon 1",
			AppstackModel::ITEM_TYPE_FOCUSED_APP);
	m_appstackModel->addApplication("test-app-2", "icon 2",
			AppstackModel::ITEM_TYPE_BACKGROUND_APP);
	m_appstackModel->endChangeset();

	qDebug() << "Query constructed" << query << m_path.path();
}

QueryImpl::~QueryImpl() {
	m_connection.unregisterObject(m_path.path());

	qDebug() << "Query destroyed" << m_path.path();
}

const QDBusObjectPath & QueryImpl::path() const {
	return m_path;
}

const QList<Result> & QueryImpl::results() const {
	return m_results;
}

QString QueryImpl::appstackModel() const {
	return QString::fromStdString(m_appstackModel->name());
}

QString QueryImpl::currentQuery() const {
	return m_query;
}

QString QueryImpl::resultsModel() const {
	return QString::fromStdString(m_resultsModel->name());
}

QStringList QueryImpl::toolbarItems() const {
	return QStringList();
}

void QueryImpl::CloseQuery() {
	m_service.closeQuery(m_path);
}

void QueryImpl::ExecuteCommand(const QDBusVariant &item, uint timestamp) {
	qDebug() << "ExecuteCommand" << item.variant() << timestamp;
}

QString QueryImpl::ExecuteParameterized(const QDBusVariant &item,
		uint timestamp, QDBusObjectPath &actionPath, QDBusObjectPath &modelPath,
		int &modelSection) {
	return QString();
}

void QueryImpl::ExecuteToolbar(const QString &item, uint timestamp) {
	qDebug() << "ExecuteToolbar" << item << timestamp;
}

/**
 * This means that the user has clicked on an application
 * in the HUD user interface.
 */
int QueryImpl::UpdateApp(const QString &app) {
	qDebug() << "UpdateApp" << app;
	return 0;
}

int QueryImpl::UpdateQuery(const QString &query) {
	qDebug() << "UpdateQuery" << query;
	if (m_query == query) {
		return 0;
	}
	m_query = query;
	//FIXME emit dbus property changed
	return 0;
}

int QueryImpl::VoiceQuery(QString &query) {
	qDebug() << "VoiceQuery";

	query = QString();
	UpdateQuery(query);
	return 0;
}
