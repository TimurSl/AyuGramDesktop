// This is the source code of AyuGram for Desktop.
//
// We do not and cannot prevent the use of our code,
// but be respectful and credit the original author.
//
// Copyright @Radolyn, 2026
#pragma once

#include "ayu/features/unigif/unigif_models.h"
#include "base/basic_types.h"

namespace InlineBots {
class Result;
} // namespace InlineBots

namespace Main {
class Session;
} // namespace Main

namespace Ayu::UniGif {

[[nodiscard]] std::shared_ptr<InlineBots::Result> MakeResult(
	not_null<Main::Session*> session,
	const Gif &gif);
[[nodiscard]] bool IsResult(const InlineBots::Result *result);
[[nodiscard]] QString GifId(not_null<const InlineBots::Result*> result);

} // namespace Ayu::UniGif
