// This is the source code of AyuGram for Desktop.
//
// We do not and cannot prevent the use of our code,
// but be respectful and credit the original author.
//
// Copyright @Radolyn, 2026
#pragma once

#include "base/basic_types.h"

#include <QtCore/QString>
#include <QtCore/QStringList>

#include <vector>

namespace Ayu::UniGif {

struct Media {
	QString url;
	int width = 0;
	int height = 0;
	int64 bytes = 0;
};

struct Gif {
	QString id;
	QStringList tags;
	int width = 0;
	int height = 0;
	int64 durationMs = 0;
	Media mp4;
	QString previewUrl;
	QString posterUrl;
	bool duplicate = false;
};

struct GifPage {
	std::vector<Gif> gifs;
	QString nextCursor;
};

struct Tag {
	QString name;
	int gifCount = 0;
};

struct Me {
	QString name;
	QStringList scopes;
};

struct Error {
	enum class Kind {
		None,
		NotSetUp,
		Network,
		Http,
		BadResponse,
	};

	Kind kind = Kind::None;
	int status = 0;
	QString code;
	QString detail;
	QString scope;
	int retryAfter = 0;

	[[nodiscard]] explicit operator bool() const {
		return kind != Kind::None;
	}
	[[nodiscard]] bool rateLimited() const {
		return (kind == Kind::Http)
			&& (status == 429 || code == u"rate_limited");
	}
};

} // namespace Ayu::UniGif
