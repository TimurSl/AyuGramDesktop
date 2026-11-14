// This is the source code of AyuGram for Desktop.
//
// We do not and cannot prevent the use of our code,
// but be respectful and credit the original author.
//
// Copyright @Radolyn, 2026
#include "ayu/features/unigif/unigif_client.h"

#include "ayu/features/unigif/unigif_config.h"

#include <crl/crl_on_main.h>
#include <crl/crl_time.h>

#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QUrl>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkReply>
#include <QtNetwork/QNetworkRequest>

#include <algorithm>
#include <map>
#include <optional>

namespace Ayu::UniGif {
namespace {

constexpr auto kTagLimit = 20;
constexpr auto kTagSuggestLimit = 12;
constexpr auto kDefaultRetryAfter = 30;
constexpr auto kMaxRetryAfter = 3600;
constexpr auto kScopesRetryDelay = 60 * crl::time(1000);
constexpr auto kApiTimeout = 20 * 1000;
constexpr auto kUploadTimeout = 60 * 1000;
constexpr auto kMaxRedirects = 5;

struct Server {
	QString baseUrl;
	QString key;

	[[nodiscard]] bool usable() const {
		if (baseUrl.isEmpty() || key.isEmpty()) {
			return false;
		}
		for (const auto &ch : key) {
			if (ch.unicode() <= 0x20 || ch.unicode() >= 0x7f) {
				return false;
			}
		}
		return true;
	}
};

using Query = std::vector<std::pair<QString, QString>>;
using Finish = Fn<void(int status, QByteArray body, Error error)>;

std::map<QString, crl::time> Backoff;
QString ScopesAskedFor;
crl::time ScopesAskedAt = 0;

[[nodiscard]] Server CurrentServer() {
	return { BaseUrl(), Key() };
}

[[nodiscard]] QNetworkAccessManager &Manager() {
	static auto manager = QNetworkAccessManager();
	return manager;
}

[[nodiscard]] QUrl BuildUrl(
		const Server &server,
		const QStringList &segments,
		const Query &query = {}) {
	auto path = server.baseUrl;
	for (const auto &segment : segments) {
		path += u'/';
		path += QString::fromLatin1(QUrl::toPercentEncoding(segment));
	}
	auto url = QUrl(path);
	if (!query.empty()) {
		auto encoded = QStringList();
		encoded.reserve(int(query.size()));
		for (const auto &[key, value] : query) {
			encoded.push_back(key
				+ u'='
				+ QString::fromLatin1(QUrl::toPercentEncoding(value)));
		}
		url.setQuery(encoded.join(u'&'));
	}
	return url;
}

[[nodiscard]] QNetworkRequest ApiRequest(
		const Server &server,
		const QUrl &url,
		int timeout) {
	auto request = QNetworkRequest(url);
	request.setRawHeader(
		"Authorization",
		"Bearer " + server.key.toUtf8());
	request.setRawHeader("Accept", "application/json");
	request.setAttribute(
		QNetworkRequest::RedirectPolicyAttribute,
		QNetworkRequest::ManualRedirectPolicy);
	request.setTransferTimeout(timeout);
	return request;
}

[[nodiscard]] QString ResolveUrl(const Server &server, const QString &url) {
	if (url.isEmpty()) {
		return QString();
	}
	const auto base = QUrl(server.baseUrl + u'/');
	const auto resolved = base.resolved(QUrl(url));
	return resolved.isValid() ? resolved.toString() : QString();
}

[[nodiscard]] Error ParseError(
		int status,
		const QByteArray &body,
		const QString &scope,
		const QByteArray &retryAfter) {
	auto result = Error{
		.kind = Error::Kind::Http,
		.status = status,
		.scope = scope,
	};
	const auto document = QJsonDocument::fromJson(body);
	if (document.isObject()) {
		const auto object = document.object();
		result.code = object.value(u"code"_q).toString();
		result.detail = object.value(u"detail"_q).toString();
		const auto own = object.value(u"scope"_q).toString();
		if (!own.isEmpty()) {
			result.scope = own;
		}
	}
	if (result.rateLimited()) {
		auto ok = false;
		const auto seconds = retryAfter.trimmed().toInt(&ok);
		result.retryAfter = ok
			? std::clamp(seconds, 1, kMaxRetryAfter)
			: kDefaultRetryAfter;
	}
	return result;
}

void Deliver(Finish done, int status, QByteArray body, Error error) {
	crl::on_main([=, done = std::move(done)] {
		done(status, body, error);
	});
}

[[nodiscard]] bool Refuse(
		const Server &server,
		const QString &scope,
		bool throttled,
		const Finish &done) {
	if (!server.usable()) {
		Deliver(
			done,
			0,
			QByteArray(),
			Error{ .kind = Error::Kind::NotSetUp });
		return true;
	} else if (!throttled) {
		return false;
	}
	const auto i = Backoff.find(server.baseUrl);
	if (i == end(Backoff)) {
		return false;
	}
	const auto left = i->second - crl::now();
	if (left <= 0) {
		Backoff.erase(i);
		return false;
	}
	Deliver(done, 429, QByteArray(), Error{
		.kind = Error::Kind::Http,
		.status = 429,
		.code = u"rate_limited"_q,
		.scope = scope,
		.retryAfter = int((left + 999) / 1000),
	});
	return true;
}

QPointer<QNetworkReply> Track(
		QNetworkReply *reply,
		const QString &baseUrl,
		const QString &scope,
		Finish done) {
	QObject::connect(reply, &QNetworkReply::finished, reply, [=] {
		reply->deleteLater();
		if (reply->error() == QNetworkReply::OperationCanceledError) {
			return;
		}
		const auto status = reply->attribute(
			QNetworkRequest::HttpStatusCodeAttribute).toInt();
		if (!status) {
			done(0, QByteArray(), Error{
				.kind = Error::Kind::Network,
				.detail = reply->errorString(),
			});
			return;
		}
		const auto body = reply->readAll();
		if (status < 200 || status >= 300) {
			auto error = ParseError(
				status,
				body,
				scope,
				reply->rawHeader("Retry-After"));
			if (error.rateLimited()) {
				Backoff[baseUrl] = crl::now() + error.retryAfter * 1000;
			}
			done(status, QByteArray(), std::move(error));
			return;
		}
		done(status, body, Error());
	});
	return reply;
}

[[nodiscard]] QPointer<QNetworkReply> Get(
		const Server &server,
		const QStringList &segments,
		const Query &query,
		const QString &scope,
		bool throttled,
		Finish done) {
	if (Refuse(server, scope, throttled, done)) {
		return nullptr;
	}
	const auto url = BuildUrl(server, segments, query);
	return Track(
		Manager().get(ApiRequest(server, url, kApiTimeout)),
		server.baseUrl,
		scope,
		std::move(done));
}

[[nodiscard]] QJsonObject ParseObject(const QByteArray &body) {
	const auto document = QJsonDocument::fromJson(body);
	return document.isObject() ? document.object() : QJsonObject();
}

[[nodiscard]] Media ParseMedia(const Server &server, const QJsonValue &value) {
	if (!value.isObject()) {
		return {};
	}
	const auto object = value.toObject();
	auto result = Media();
	result.url = ResolveUrl(server, object.value(u"url"_q).toString());
	result.width = object.value(u"width"_q).toInt();
	result.height = object.value(u"height"_q).toInt();
	result.bytes = qint64(object.value(u"bytes"_q).toDouble());
	return result;
}

[[nodiscard]] Gif ParseGif(const Server &server, const QJsonObject &object) {
	auto result = Gif();
	result.id = object.value(u"id"_q).toString();
	if (result.id.isEmpty()) {
		return {};
	}
	for (const auto &tag : object.value(u"tags"_q).toArray()) {
		const auto name = tag.toString();
		if (!name.isEmpty()) {
			result.tags.push_back(name);
		}
	}
	result.width = object.value(u"width"_q).toInt();
	result.height = object.value(u"height"_q).toInt();
	result.durationMs = qint64(object.value(u"duration_ms"_q).toDouble());
	const auto media = object.value(u"media"_q);
	if (media.isObject()) {
		const auto renditions = media.toObject();
		result.mp4 = ParseMedia(server, renditions.value(u"mp4"_q));
		result.previewUrl = ParseMedia(
			server,
			renditions.value(u"preview_mp4"_q)).url;
		result.posterUrl = ParseMedia(
			server,
			renditions.value(u"poster"_q)).url;
	}
	return result;
}

[[nodiscard]] std::optional<GifPage> ParseGifPage(
		const Server &server,
		const QByteArray &body) {
	const auto object = ParseObject(body);
	const auto items = object.value(u"items"_q);
	if (!items.isArray()) {
		return std::nullopt;
	}
	auto result = GifPage();
	for (const auto &item : items.toArray()) {
		if (!item.isObject()) {
			continue;
		}
		auto gif = ParseGif(server, item.toObject());
		if (!gif.id.isEmpty() && !gif.mp4.url.isEmpty()) {
			result.gifs.push_back(std::move(gif));
		}
	}
	result.nextCursor = object.value(u"next_cursor"_q).toString();
	return result;
}

[[nodiscard]] std::optional<std::vector<Tag>> ParseTags(
		const QByteArray &body,
		const QString &field) {
	const auto items = ParseObject(body).value(field);
	if (!items.isArray()) {
		return std::nullopt;
	}
	auto result = std::vector<Tag>();
	for (const auto &item : items.toArray()) {
		if (!item.isObject()) {
			continue;
		}
		const auto object = item.toObject();
		auto tag = Tag{
			.name = object.value(u"name"_q).toString(),
			.gifCount = object.value(u"gif_count"_q).toInt(),
		};
		if (!tag.name.isEmpty()) {
			result.push_back(std::move(tag));
		}
	}
	return result;
}

} // namespace

QPointer<QNetworkReply> RequestMe(MeDone done) {
	const auto server = CurrentServer();
	const auto finish = [=](int status, QByteArray body, Error error) {
		if (error) {
			done({}, std::move(error));
			return;
		}
		const auto object = ParseObject(body);
		if (object.isEmpty()) {
			done({}, Error{ .kind = Error::Kind::BadResponse });
			return;
		}
		auto result = Me{ .name = object.value(u"name"_q).toString() };
		for (const auto &scope : object.value(u"scopes"_q).toArray()) {
			result.scopes.push_back(scope.toString());
		}
		RememberScopes(server.baseUrl, server.key, result.scopes);
		done(std::move(result), Error());
	};
	return Get(
		server,
		{ u"me"_q },
		{},
		QString(),
		false,
		finish);
}

void EnsureScopes() {
	const auto server = CurrentServer();
	if (!server.usable() || ScopesKnown()) {
		return;
	}
	const auto asked = server.baseUrl + '\n' + server.key;
	const auto now = crl::now();
	if (asked == ScopesAskedFor
		&& (now - ScopesAskedAt) < kScopesRetryDelay) {
		return;
	}
	ScopesAskedFor = asked;
	ScopesAskedAt = now;
	RequestMe([](Me, Error) {});
}

QPointer<QNetworkReply> ListGifs(
		const QString &query,
		const QString &sort,
		const QString &cursor,
		PageDone done) {
	const auto server = CurrentServer();
	auto parameters = Query();
	if (!query.isEmpty()) {
		parameters.emplace_back(u"q"_q, query);
	}
	if (!sort.isEmpty()) {
		parameters.emplace_back(u"sort"_q, sort);
	}
	parameters.emplace_back(u"status"_q, u"ready"_q);
	parameters.emplace_back(u"limit"_q, QString::number(kPageSize));
	if (!cursor.isEmpty()) {
		parameters.emplace_back(u"cursor"_q, cursor);
	}
	const auto finish = [=](int status, QByteArray body, Error error) {
		if (error) {
			done({}, std::move(error));
			return;
		}
		auto page = ParseGifPage(server, body);
		if (!page) {
			done({}, Error{ .kind = Error::Kind::BadResponse });
			return;
		}
		done(std::move(*page), Error());
	};
	return Get(
		server,
		{ u"gifs"_q },
		parameters,
		u"read"_q,
		true,
		finish);
}

QPointer<QNetworkReply> ListTags(const QString &prefix, TagsDone done) {
	return Get(
		CurrentServer(),
		{ u"tags"_q },
		{
			{ u"q"_q, prefix },
			{ u"sort"_q, u"popular"_q },
			{ u"limit"_q, QString::number(kTagLimit) },
		},
		u"read"_q,
		true,
		[=](int status, QByteArray body, Error error) {
			if (error) {
				done({}, std::move(error));
				return;
			}
			auto tags = ParseTags(body, u"items"_q);
			if (!tags) {
				done({}, Error{ .kind = Error::Kind::BadResponse });
				return;
			}
			done(std::move(*tags), Error());
		});
}

QPointer<QNetworkReply> SuggestTags(TagsDone done) {
	return Get(
		CurrentServer(),
		{ u"tags"_q, u"suggest"_q },
		{ { u"limit"_q, QString::number(kTagSuggestLimit) } },
		u"read"_q,
		true,
		[=](int status, QByteArray body, Error error) {
			if (error) {
				done({}, std::move(error));
				return;
			}
			auto tags = ParseTags(body, u"recent"_q);
			if (!tags) {
				done({}, Error{ .kind = Error::Kind::BadResponse });
				return;
			}
			done(std::move(*tags), Error());
		});
}

void Use(const QString &id) {
	const auto server = CurrentServer();
	const auto ignore = [](int, QByteArray, Error) {};
	if (Refuse(server, u"read"_q, false, ignore)) {
		return;
	}
	auto request = ApiRequest(
		server,
		BuildUrl(server, { u"gifs"_q, id, u"use"_q }),
		kApiTimeout);
	Track(
		Manager().post(request, QByteArray()),
		server.baseUrl,
		u"read"_q,
		ignore);
}

QPointer<QNetworkReply> Add(
		const QString &path,
		const QString &mime,
		const QString &sourceId,
		GifDone done) {
	const auto server = CurrentServer();
	const auto finish = [=](int status, QByteArray body, Error error) {
		if (error) {
			done({}, std::move(error));
			return;
		}
		auto object = ParseObject(body);
		const auto inner = object.value(u"gif"_q);
		auto gif = ParseGif(
			server,
			inner.isObject() ? inner.toObject() : object);
		if (gif.id.isEmpty()) {
			done({}, Error{
				.kind = Error::Kind::BadResponse,
				.status = status,
			});
			return;
		}
		gif.duplicate = (status == 200)
			&& object.value(u"duplicate"_q).toBool();
		done(std::move(gif), Error());
	};
	if (Refuse(server, u"upload"_q, false, finish)) {
		return nullptr;
	}
	auto file = std::make_unique<QFile>(path);
	if (!file->open(QIODevice::ReadOnly)) {
		Deliver(finish, 0, QByteArray(), Error{
			.kind = Error::Kind::Network,
			.detail = file->errorString(),
		});
		return nullptr;
	}
	auto query = Query{ { u"source"_q, u"telegram"_q } };
	if (!sourceId.isEmpty()) {
		query.emplace_back(u"source_id"_q, sourceId);
	}
	query.emplace_back(u"wait"_q, u"0"_q);
	auto request = ApiRequest(
		server,
		BuildUrl(server, { u"gifs"_q }, query),
		kUploadTimeout);
	request.setHeader(
		QNetworkRequest::ContentTypeHeader,
		mime.isEmpty() ? u"video/mp4"_q : mime);
	const auto reply = Manager().post(request, file.get());
	file.release()->setParent(reply);
	return Track(reply, server.baseUrl, u"upload"_q, finish);
}

QPointer<QNetworkReply> Delete(const QString &id, PlainDone done) {
	const auto server = CurrentServer();
	const auto finish = [=](int, QByteArray, Error error) {
		done(std::move(error));
	};
	if (Refuse(server, u"delete"_q, false, finish)) {
		return nullptr;
	}
	const auto request = ApiRequest(
		server,
		BuildUrl(server, { u"gifs"_q, id }),
		kApiTimeout);
	return Track(
		Manager().deleteResource(request),
		server.baseUrl,
		u"delete"_q,
		finish);
}

QPointer<QNetworkReply> Download(
		const QString &url,
		const QString &target,
		FileDone done) {
	const auto parsed = QUrl(url);
	const auto scheme = parsed.scheme().toLower();
	if (!parsed.isValid() || (scheme != u"https"_q && scheme != u"http"_q)) {
		crl::on_main([=] {
			done(QString(), Error{ .kind = Error::Kind::BadResponse });
		});
		return nullptr;
	}
	QDir().mkpath(QFileInfo(target).absolutePath());
	auto part = std::make_unique<QFile>(target + u".part"_q);
	if (!part->open(QIODevice::WriteOnly | QIODevice::Truncate)) {
		const auto detail = part->errorString();
		crl::on_main([=] {
			done(QString(), Error{
				.kind = Error::Kind::Network,
				.detail = detail,
			});
		});
		return nullptr;
	}
	auto request = QNetworkRequest(parsed);
	request.setAttribute(
		QNetworkRequest::RedirectPolicyAttribute,
		QNetworkRequest::NoLessSafeRedirectPolicy);
	request.setMaximumRedirectsAllowed(kMaxRedirects);
	request.setTransferTimeout(kUploadTimeout);
	const auto reply = Manager().get(request);
	const auto file = part.release();
	file->setParent(reply);
	QObject::connect(reply, &QNetworkReply::readyRead, file, [=] {
		file->write(reply->readAll());
	});
	QObject::connect(reply, &QNetworkReply::finished, reply, [=] {
		reply->deleteLater();
		if (reply->error() == QNetworkReply::OperationCanceledError) {
			file->close();
			QFile::remove(file->fileName());
			return;
		}
		file->write(reply->readAll());
		file->close();
		const auto status = reply->attribute(
			QNetworkRequest::HttpStatusCodeAttribute).toInt();
		const auto partPath = file->fileName();
		if (status < 200 || status >= 300) {
			QFile::remove(partPath);
			done(QString(), Error{
				.kind = status ? Error::Kind::Http : Error::Kind::Network,
				.status = status,
				.detail = reply->errorString(),
			});
			return;
		}
		QFile::remove(target);
		if (!QFile::rename(partPath, target)) {
			QFile::remove(partPath);
			done(QString(), Error{
				.kind = Error::Kind::Network,
				.detail = u"cannot write "_q + target,
			});
			return;
		}
		done(target, Error());
	});
	return reply;
}

} // namespace Ayu::UniGif
