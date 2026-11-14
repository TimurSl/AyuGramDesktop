// This is the source code of AyuGram for Desktop.
//
// We do not and cannot prevent the use of our code,
// but be respectful and credit the original author.
//
// Copyright @Radolyn, 2026
#pragma once

#include "ayu/features/unigif/unigif_models.h"
#include "base/basic_types.h"

#include <QtCore/QPointer>

class QNetworkReply;

namespace Ayu::UniGif {

constexpr auto kPageSize = 50;

using MeDone = Fn<void(Me, Error)>;
using PageDone = Fn<void(GifPage, Error)>;
using TagsDone = Fn<void(std::vector<Tag>, Error)>;
using GifDone = Fn<void(Gif, Error)>;
using PlainDone = Fn<void(Error)>;
using FileDone = Fn<void(QString, Error)>;

QPointer<QNetworkReply> RequestMe(MeDone done);
void EnsureScopes();

QPointer<QNetworkReply> ListGifs(
	const QString &query,
	const QString &sort,
	const QString &cursor,
	PageDone done);
QPointer<QNetworkReply> ListTags(const QString &prefix, TagsDone done);
QPointer<QNetworkReply> SuggestTags(TagsDone done);
void Use(const QString &id);
QPointer<QNetworkReply> Add(
	const QString &path,
	const QString &mime,
	const QString &sourceId,
	GifDone done);
QPointer<QNetworkReply> Delete(const QString &id, PlainDone done);
QPointer<QNetworkReply> Download(
	const QString &url,
	const QString &target,
	FileDone done);

} // namespace Ayu::UniGif
