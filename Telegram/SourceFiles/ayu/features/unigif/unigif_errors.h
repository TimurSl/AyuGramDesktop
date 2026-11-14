// This is the source code of AyuGram for Desktop.
//
// We do not and cannot prevent the use of our code,
// but be respectful and credit the original author.
//
// Copyright @Radolyn, 2026
#pragma once

#include "ayu/features/unigif/unigif_models.h"

namespace Ayu::UniGif {

[[nodiscard]] QString ErrorMessage(const Error &error);
void ShowToast(const QString &text);

} // namespace Ayu::UniGif
