// This is the source code of AyuGram for Desktop.
//
// We do not and cannot prevent the use of our code,
// but be respectful and credit the original author.
//
// Copyright @Radolyn, 2026
#include "ayu/features/unigif/unigif_config.h"

#include "ayu/ayu_settings.h"

#include <QtCore/QUrl>

namespace Ayu::UniGif {
namespace {

QString ScopesOwner;
QStringList Scopes;
bool ScopesAreKnown = false;

[[nodiscard]] QString OwnerOf(const QString &baseUrl, const QString &key) {
	return baseUrl + '\n' + key;
}

} // namespace

bool IsSelected() {
	return AyuSettings::getInstance().unigifEnabled();
}

bool IsActive() {
	return IsSelected() && !BaseUrl().isEmpty() && !Key().isEmpty();
}

QString BaseUrl() {
	const auto url = NormalizeUrl(AyuSettings::getInstance().unigifUrl());
	return IsValidUrl(url) ? url : QString();
}

QString Key() {
	return AyuSettings::getInstance().unigifKey().trimmed();
}

QString NormalizeUrl(const QString &input) {
	auto url = input.trimmed();
	if (url.isEmpty()) {
		return QString();
	}
	if (!url.contains(u"://"_q)) {
		url = u"https://"_q + url;
	}
	while (url.endsWith('/')) {
		url.chop(1);
	}
	return url;
}

bool IsValidUrl(const QString &normalized) {
	if (normalized.isEmpty()) {
		return false;
	}
	const auto url = QUrl(normalized, QUrl::StrictMode);
	const auto scheme = url.scheme().toLower();
	return url.isValid()
		&& (scheme == u"https"_q || scheme == u"http"_q)
		&& !url.host().isEmpty()
		&& !url.hasQuery()
		&& !url.hasFragment()
		&& url.userInfo().isEmpty();
}

bool IsCleartext(const QString &normalized) {
	return normalized.startsWith(u"http://"_q, Qt::CaseInsensitive);
}

void RememberScopes(
		const QString &baseUrl,
		const QString &key,
		const QStringList &scopes) {
	ScopesOwner = OwnerOf(baseUrl, key);
	Scopes = scopes;
	ScopesAreKnown = true;
}

void ForgetScopes() {
	ScopesOwner = QString();
	Scopes = QStringList();
	ScopesAreKnown = false;
}

bool ScopesKnown() {
	return ScopesAreKnown && (OwnerOf(BaseUrl(), Key()) == ScopesOwner);
}

bool HasScope(const QString &scope) {
	return ScopesKnown() && Scopes.contains(scope);
}

} // namespace Ayu::UniGif
