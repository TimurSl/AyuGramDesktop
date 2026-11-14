// This is the source code of AyuGram for Desktop.
//
// We do not and cannot prevent the use of our code,
// but be respectful and credit the original author.
//
// Copyright @Radolyn, 2026
#pragma once

#include <QtCore/QString>

namespace Ayu::UniGif {

[[nodiscard]] QString NormalizeQuery(const QString &text);
[[nodiscard]] QString LastWord(const QString &text);
[[nodiscard]] QString ReplaceLastWord(const QString &text, const QString &tag);
[[nodiscard]] QString TagName(const QString &tag);

} // namespace Ayu::UniGif
