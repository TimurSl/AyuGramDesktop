// This is the source code of AyuGram for Desktop.
//
// We do not and cannot prevent the use of our code,
// but be respectful and credit the original author.
//
// Copyright @Radolyn, 2026
#include "ayu/features/unigif/unigif_errors.h"

#include "core/application.h"
#include "window/window_controller.h"
#include "lang_auto.h"

namespace Ayu::UniGif {
namespace {

[[nodiscard]] bool KeyRejected(const Error &error) {
	return (error.status == 401)
		|| (error.code == u"unauthorized"_q)
		|| (error.code == u"invalid_credentials"_q);
}

} // namespace

QString ErrorMessage(const Error &error) {
	switch (error.kind) {
	case Error::Kind::None:
	case Error::Kind::NotSetUp:
		return tr::ayu_UnigifErrorNotSetUp(tr::now);
	case Error::Kind::Network:
		return tr::ayu_UnigifErrorNetwork(tr::now);
	case Error::Kind::BadResponse:
		return tr::ayu_UnigifErrorBadResponse(tr::now);
	case Error::Kind::Http:
		break;
	}
	if (KeyRejected(error)) {
		return tr::ayu_UnigifErrorKeyRejected(tr::now);
	} else if ((error.code == u"scope_required"_q
			|| error.code == u"forbidden"_q
			|| (error.code.isEmpty() && error.status == 403))
		&& !error.scope.isEmpty()) {
		return tr::ayu_UnigifErrorScope(
			tr::now,
			lt_scope,
			error.scope);
	} else if (error.rateLimited()) {
		return tr::ayu_UnigifErrorRateLimit(
			tr::now,
			lt_seconds,
			QString::number(error.retryAfter));
	} else if (error.code == u"too_large"_q || error.status == 413) {
		return tr::ayu_UnigifErrorTooLarge(tr::now);
	} else if (error.code == u"unsupported_media"_q
		|| error.code == u"invalid_media"_q
		|| error.status == 415) {
		return tr::ayu_UnigifErrorUnsupported(tr::now);
	}
	return tr::ayu_UnigifErrorOther(
		tr::now,
		lt_code,
		QString::number(error.status));
}

void ShowToast(const QString &text) {
	if (const auto window = Core::App().activeWindow()) {
		window->showToast(text);
	}
}

} // namespace Ayu::UniGif
