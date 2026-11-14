// This is the source code of AyuGram for Desktop.
//
// We do not and cannot prevent the use of our code,
// but be respectful and credit the original author.
//
// Copyright @Radolyn, 2026
#include "ayu/features/unigif/unigif_results.h"

#include "inline_bots/inline_bot_result.h"
#include "main/main_session.h"

#include <memory>

namespace Ayu::UniGif {
namespace {

constexpr auto kDefaultSide = 100;

[[nodiscard]] QString IdPrefix() {
	return u"ugf:"_q;
}

[[nodiscard]] MTPVector<MTPDocumentAttribute> VideoAttributes(
		const Gif &gif) {
	const auto width = (gif.width > 0 && gif.height > 0)
		? gif.width
		: (gif.mp4.width > 0 && gif.mp4.height > 0)
		? gif.mp4.width
		: kDefaultSide;
	const auto height = (gif.width > 0 && gif.height > 0)
		? gif.height
		: (gif.mp4.width > 0 && gif.mp4.height > 0)
		? gif.mp4.height
		: kDefaultSide;
	return MTP_vector<MTPDocumentAttribute>(
		QVector<MTPDocumentAttribute>(1, MTP_documentAttributeVideo(
			MTP_flags(MTPDdocumentAttributeVideo::Flags(0)),
			MTP_double(gif.durationMs / 1000.),
			MTP_int(width),
			MTP_int(height),
			MTPint(), // preload_prefix_size
			MTPdouble(), // video_start_ts
			MTPstring()))); // video_codec
}

} // namespace

std::shared_ptr<InlineBots::Result> MakeResult(
		not_null<Main::Session*> session,
		const Gif &gif) {
	if (gif.id.isEmpty() || gif.mp4.url.isEmpty()) {
		return nullptr;
	}
	const auto mime = MTP_string(u"video/mp4"_q);
	const auto content = MTP_webDocumentNoProxy(
		MTP_string(gif.mp4.url),
		MTP_int(int(gif.mp4.bytes)),
		mime,
		VideoAttributes(gif));
	const auto thumb = MTP_webDocumentNoProxy(
		MTP_string(gif.previewUrl.isEmpty() ? gif.mp4.url : gif.previewUrl),
		MTP_int(0),
		mime,
		VideoAttributes(gif));
	using Flag = MTPDbotInlineResult::Flag;
	return InlineBots::Result::Create(
		session,
		uint64(0),
		MTP_botInlineResult(
			MTP_flags(Flag::f_title | Flag::f_thumb | Flag::f_content),
			MTP_string(IdPrefix() + gif.id),
			MTP_string(u"gif"_q),
			MTP_string(gif.tags.join(u' ')),
			MTPstring(), // description
			MTPstring(), // url
			thumb,
			content,
			MTP_botInlineMessageMediaAuto(
				MTP_flags(MTPDbotInlineMessageMediaAuto::Flags(0)),
				MTP_string(QString()),
				MTPVector<MTPMessageEntity>(),
				MTPReplyMarkup())));
}

bool IsResult(const InlineBots::Result *result) {
	return result && result->getId().startsWith(IdPrefix());
}

QString GifId(not_null<const InlineBots::Result*> result) {
	return result->getId().mid(IdPrefix().size());
}

} // namespace Ayu::UniGif
