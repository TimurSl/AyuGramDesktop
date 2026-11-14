// This is the source code of AyuGram for Desktop.
//
// We do not and cannot prevent the use of our code,
// but be respectful and credit the original author.
//
// Copyright @Radolyn, 2026
#include "ayu/ui/settings/settings_unigif.h"

#include "ayu/features/unigif/unigif_client.h"
#include "ayu/features/unigif/unigif_config.h"
#include "ayu/features/unigif/unigif_errors.h"
#include "ayu/ui/boxes/edit_mark_box.h"
#include "ayu/ayu_settings.h"
#include "lang_auto.h"
#include "styles/style_settings.h"

namespace Settings {

using namespace Builder;
using namespace AyuBuilder;

namespace {

constexpr auto kKeyTailLength = 4;

[[nodiscard]] QString UrlLabel(const QString &url) {
	return url.trimmed().isEmpty()
		? tr::ayu_UnigifNotSet(tr::now)
		: url.trimmed();
}

[[nodiscard]] QString KeyLabel(const QString &key) {
	const auto trimmed = key.trimmed();
	if (trimmed.isEmpty()) {
		return tr::ayu_UnigifNotSet(tr::now);
	}
	const auto dots = QString(4, QChar(0x2022));
	return (trimmed.size() > 2 * kKeyTailLength)
		? (dots + trimmed.right(kKeyTailLength))
		: dots;
}

void ShowTestResult(Ayu::UniGif::Me me, Ayu::UniGif::Error error) {
	if (error) {
		Ayu::UniGif::ShowToast(Ayu::UniGif::ErrorMessage(error));
		return;
	}
	auto lines = QStringList{
		tr::ayu_UnigifTestSuccess(tr::now, lt_name, me.name),
	};
	for (const auto &scope : { u"read"_q, u"upload"_q }) {
		if (!me.scopes.contains(scope)) {
			lines.push_back(
				tr::ayu_UnigifTestMissingScope(tr::now, lt_scope, scope));
		}
	}
	Ayu::UniGif::ShowToast(lines.join(u'\n'));
}

} // namespace

void BuildUniGif(SectionBuilder &builder, AyuSectionBuilder &ayu) {
	builder.addSubsectionTitle(tr::ayu_UnigifHeader());

	ayu.addSettingToggle({
		.id = u"ayu/unigifEnabled"_q,
		.altIds = { u"ayu/unigif"_q },
		.title = tr::ayu_UnigifUse(),
		.getter = &AyuSettings::unigifEnabled,
		.setter = &AyuSettings::setUnigifEnabled,
		.keywords = { u"gif"_q, u"klipy"_q },
	});

	auto &settings = AyuSettings::getInstance();

	builder.addButton({
		.id = u"ayu/unigifUrl"_q,
		.title = tr::ayu_UnigifServerUrl(),
		.st = &st::settingsButtonNoIcon,
		.label = settings.unigifUrlValue() | rpl::map(UrlLabel),
		.onClick = [] {
			Ui::show(Box<EditMarkBox>(
				tr::ayu_UnigifServerUrl(),
				AyuSettings::getInstance().unigifUrl(),
				QString(),
				[](const QString &value) {
					AyuSettings::getInstance().setUnigifUrl(value.trimmed());
				}));
		},
		.shown = settings.unigifEnabledValue(),
	});

	builder.addButton({
		.id = u"ayu/unigifKey"_q,
		.title = tr::ayu_UnigifApiKey(),
		.st = &st::settingsButtonNoIcon,
		.label = settings.unigifKeyValue() | rpl::map(KeyLabel),
		.onClick = [] {
			Ui::show(Box<EditMarkBox>(
				tr::ayu_UnigifApiKey(),
				AyuSettings::getInstance().unigifKey(),
				QString(),
				[](const QString &value) {
					AyuSettings::getInstance().setUnigifKey(value.trimmed());
				}));
		},
		.shown = settings.unigifEnabledValue(),
	});

	builder.addButton({
		.id = u"ayu/unigifTest"_q,
		.title = tr::ayu_UnigifTestConnection(),
		.st = &st::settingsButtonNoIcon,
		.onClick = [] {
			Ayu::UniGif::RequestMe(ShowTestResult);
		},
		.shown = settings.unigifEnabledValue(),
	});

	builder.addDividerText(rpl::combine(
		settings.unigifUrlValue(),
		tr::ayu_UnigifSettingsAbout(),
		tr::ayu_UnigifCleartextWarning()
	) | rpl::map([](
			const QString &url,
			const QString &about,
			const QString &warning) {
		const auto cleartext = Ayu::UniGif::IsCleartext(
			Ayu::UniGif::NormalizeUrl(url));
		return cleartext ? (warning + u"\n\n"_q + about) : about;
	}));
}

} // namespace Settings
