// This is the source code of AyuGram for Desktop.
//
// We do not and cannot prevent the use of our code,
// but be respectful and credit the original author.
//
// Copyright @Radolyn, 2026
#include "ayu/features/unigif/unigif_sender.h"

#include "ayu/features/unigif/unigif_client.h"
#include "ayu/features/unigif/unigif_errors.h"
#include "ayu/features/unigif/unigif_results.h"
#include "base/weak_ptr.h"
#include "inline_bots/inline_bot_result.h"
#include "main/main_session.h"
#include "storage/localimageloader.h"
#include "storage/storage_media_prepare.h"
#include "ui/chat/attach/attach_prepare.h"
#include "apiwrap.h"
#include "settings.h"
#include "styles/style_boxes.h"

#include <QtCore/QFileInfo>

#include <map>
#include <vector>

namespace Ayu::UniGif {
namespace {

using Waiter = Fn<void(QString, Error)>;

std::map<QString, std::vector<Waiter>> Pending;

[[nodiscard]] bool HasFile(const QString &path) {
	return QFileInfo(path).size() > 0;
}

void FetchMedia(const QString &url, const QString &path, Waiter waiter) {
	if (HasFile(path)) {
		waiter(path, Error());
		return;
	}
	const auto i = Pending.find(path);
	if (i != end(Pending)) {
		i->second.push_back(std::move(waiter));
		return;
	}
	Pending[path].push_back(std::move(waiter));
	Download(url, path, [=](QString file, Error error) {
		auto waiters = std::move(Pending[path]);
		Pending.erase(path);
		for (const auto &waiter : waiters) {
			waiter(file, error);
		}
	});
}

void SendFile(
		not_null<Main::Session*> session,
		const QString &path,
		const Api::SendAction &action) {
	auto file = Ui::PreparedFile(path);
	Storage::PrepareDetails(file, st::sendMediaPreviewSize, PhotoSideLimit());
	auto list = Ui::PreparedList();
	list.files.push_back(std::move(file));
	session->api().sendFiles(
		std::move(list),
		SendMediaType::Photo,
		nullptr,
		action);
}

} // namespace

QString CachePath(const QString &id) {
	return cWorkingDir() + u"tdata/ayu/unigif/"_q + id + u".mp4"_q;
}

void Send(
		not_null<Main::Session*> session,
		not_null<InlineBots::Result*> result,
		Api::SendAction action,
		Fn<void(bool)> done) {
	const auto id = GifId(result);
	const auto weak = base::make_weak(session);
	FetchMedia(result->contentUrl(), CachePath(id), [=](
			QString path,
			Error error) {
		const auto finish = [&](bool ok) {
			if (done) {
				done(ok);
			}
		};
		const auto strong = weak.get();
		if (!strong) {
			finish(false);
		} else if (error) {
			ShowToast(ErrorMessage(error));
			finish(false);
		} else {
			SendFile(strong, path, action);
			Use(id);
			finish(true);
		}
	});
}

} // namespace Ayu::UniGif
