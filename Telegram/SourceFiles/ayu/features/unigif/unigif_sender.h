// This is the source code of AyuGram for Desktop.
//
// We do not and cannot prevent the use of our code,
// but be respectful and credit the original author.
//
// Copyright @Radolyn, 2026
#pragma once

#include "api/api_common.h"
#include "base/basic_types.h"

namespace InlineBots {
class Result;
} // namespace InlineBots

namespace Main {
class Session;
} // namespace Main

namespace Ayu::UniGif {

[[nodiscard]] QString CachePath(const QString &id);

void Send(
	not_null<Main::Session*> session,
	not_null<InlineBots::Result*> result,
	Api::SendAction action,
	Fn<void(bool)> done);

} // namespace Ayu::UniGif
