// This is the source code of AyuGram for Desktop.
//
// We do not and cannot prevent the use of our code,
// but be respectful and credit the original author.
//
// Copyright @Radolyn, 2026
#include "ayu/features/unigif/unigif_uploader.h"

#include "ayu/features/unigif/unigif_client.h"
#include "ayu/features/unigif/unigif_errors.h"
#include "data/data_document.h"
#include "data/data_document_media.h"
#include "main/main_session.h"
#include "lang_auto.h"
#include "settings.h"

#include <crl/crl_on_main.h>

#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>

#include <map>
#include <memory>
#include <set>

namespace Ayu::UniGif {
namespace {

std::set<DocumentId> InFlight;

struct Pending {
	std::shared_ptr<Data::DocumentMedia> media;
	rpl::lifetime lifetime;
};

std::map<DocumentId, std::shared_ptr<Pending>> Active;

[[nodiscard]] QString TempPath(not_null<DocumentData*> document) {
	return cWorkingDir()
		+ u"tdata/ayu/unigif/upload_"_q
		+ QString::number(document->id)
		+ u".mp4"_q;
}

[[nodiscard]] QString MimeFor(not_null<DocumentData*> document) {
	return document->mimeString().isEmpty()
		? u"video/mp4"_q
		: document->mimeString();
}

void Upload(
		not_null<DocumentData*> document,
		const QString &path,
		bool temporary) {
	const auto id = document->id;
	Add(path, MimeFor(document), QString::number(id), [=](
			Gif gif,
			Error error) {
		InFlight.erase(id);
		if (temporary) {
			QFile::remove(path);
		}
		if (error) {
			ShowToast(ErrorMessage(error));
		} else if (gif.duplicate) {
			ShowToast(tr::ayu_UnigifAlreadyAdded(tr::now));
		} else {
			ShowToast(tr::ayu_UnigifAdded(tr::now));
		}
	});
}

[[nodiscard]] QString WriteBytes(
		not_null<DocumentData*> document,
		const QByteArray &bytes) {
	const auto path = TempPath(document);
	QDir().mkpath(QFileInfo(path).absolutePath());
	auto file = QFile(path);
	if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)
		|| file.write(bytes) != bytes.size()) {
		return QString();
	}
	return path;
}

void Fail(DocumentId id) {
	InFlight.erase(id);
	ShowToast(tr::ayu_UnigifDownloadFailed(tr::now));
}

void FinishDownload(not_null<DocumentData*> document, bool ok) {
	const auto id = document->id;
	const auto i = Active.find(id);
	if (i == end(Active)) {
		return;
	}
	const auto state = i->second;
	Active.erase(i);
	const auto bytes = ok ? state->media->bytes() : QByteArray();
	crl::on_main([state = state] {
		state->lifetime.destroy();
	});
	const auto path = bytes.isEmpty()
		? QString()
		: WriteBytes(document, bytes);
	if (path.isEmpty()) {
		Fail(id);
	} else {
		Upload(document, path, true);
	}
}

} // namespace

void AddDocument(
		not_null<DocumentData*> document,
		Data::FileOrigin origin) {
	const auto id = document->id;
	if (!InFlight.emplace(id).second) {
		return;
	}
	const auto existing = document->filepath(true);
	if (!existing.isEmpty()) {
		Upload(document, existing, false);
		return;
	}
	const auto media = document->createMediaView();
	const auto cached = media->bytes();
	if (!cached.isEmpty()) {
		const auto path = WriteBytes(document, cached);
		if (path.isEmpty()) {
			Fail(id);
		} else {
			Upload(document, path, true);
		}
		return;
	}
	ShowToast(tr::ayu_UnigifAdding(tr::now));
	const auto state = std::make_shared<Pending>();
	state->media = media;
	Active.emplace(id, state);
	document->session().downloaderTaskFinished(
	) | rpl::on_next([=] {
		if (state->media->loaded()) {
			FinishDownload(document, true);
		} else if (!document->loading()) {
			FinishDownload(document, false);
		}
	}, state->lifetime);
	document->save(origin, QString());
}

} // namespace Ayu::UniGif
