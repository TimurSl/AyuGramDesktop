// This is the source code of AyuGram for Desktop.
//
// We do not and cannot prevent the use of our code,
// but be respectful and credit the original author.
//
// Copyright @Radolyn, 2026
#include "ayu/features/unigif/unigif_query.h"

namespace Ayu::UniGif {
namespace {

[[nodiscard]] int LastWordStart(const QString &text) {
	auto i = int(text.size());
	while (i > 0 && !text[i - 1].isSpace()) {
		--i;
	}
	return i;
}

} // namespace

QString NormalizeQuery(const QString &text) {
	auto start = 0;
	while (start < text.size() && text[start].isSpace()) {
		++start;
	}
	auto end = int(text.size());
	while (end > start && text[end - 1].isSpace()) {
		--end;
	}
	if (end == start) {
		return QString();
	}
	const auto result = text.mid(start, end - start);
	return (end < text.size()) ? (result + ' ') : result;
}

QString LastWord(const QString &text) {
	auto word = text.mid(LastWordStart(text));
	if (word.startsWith('-')) {
		word.remove(0, 1);
	}
	if (word.startsWith('#')) {
		word.remove(0, 1);
	}
	return word;
}

QString ReplaceLastWord(const QString &text, const QString &tag) {
	const auto start = LastWordStart(text);
	const auto exclude = text.mid(start).startsWith('-');
	return text.left(start)
		+ (exclude ? u"-"_q : QString())
		+ TagName(tag)
		+ ' ';
}

QString TagName(const QString &tag) {
	const auto trimmed = tag.trimmed();
	return trimmed.startsWith('#') ? trimmed.mid(1) : trimmed;
}

} // namespace Ayu::UniGif
