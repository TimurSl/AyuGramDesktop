// This is the source code of AyuGram for Desktop.
//
// We do not and cannot prevent the use of our code,
// but be respectful and credit the original author.
//
// Copyright @Radolyn, 2026
#pragma once

#include "data/data_file_origin.h"

class DocumentData;

namespace Ayu::UniGif {

void AddDocument(
	not_null<DocumentData*> document,
	Data::FileOrigin origin);

} // namespace Ayu::UniGif
