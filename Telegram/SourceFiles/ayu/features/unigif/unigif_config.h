// This is the source code of AyuGram for Desktop.
//
// We do not and cannot prevent the use of our code,
// but be respectful and credit the original author.
//
// Copyright @Radolyn, 2026
#pragma once

#include <QtCore/QString>
#include <QtCore/QStringList>

namespace Ayu::UniGif {

[[nodiscard]] bool IsSelected();
[[nodiscard]] bool IsActive();
[[nodiscard]] QString BaseUrl();
[[nodiscard]] QString Key();

[[nodiscard]] QString NormalizeUrl(const QString &input);
[[nodiscard]] bool IsValidUrl(const QString &normalized);
[[nodiscard]] bool IsCleartext(const QString &normalized);

void RememberScopes(
	const QString &baseUrl,
	const QString &key,
	const QStringList &scopes);
void ForgetScopes();
[[nodiscard]] bool ScopesKnown();
[[nodiscard]] bool HasScope(const QString &scope);

} // namespace Ayu::UniGif
