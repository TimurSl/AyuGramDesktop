# UniGIF integration

Ported from the Retrogram (Android) feature track. When enabled, the GIF panel
shows a self-hosted [UniGIF](https://github.com/TimurSl/unigif) library instead
of Telegram's GIF bot, and "Save to GIFs" becomes "Add to UniGIF".

## Setup

Settings → Chats → UniGIF: turn it on, enter the server URL and an API key,
then use "Test connection". The key needs `read` and `upload`; `delete` is
optional and only enables "Delete from UniGIF". Without a URL and a key the
panel behaves as before.

The key is stored in `tdata/ayu_settings.json` like the other AyuGram
settings, in plain text. An `http://` URL sends it unencrypted (the settings
screen warns about this).

## Behaviour

- Panel: the home list is the library sorted by use (`sort=used`); typing
  searches the library (`q=` as typed: words are AND-ed hashtags, the last word
  is a prefix, `-word` excludes). Pages load on scroll via `next_cursor`.
  Telegram's saved GIFs are hidden while UniGIF is on, not deleted.
- Sending: the MP4 is downloaded once to `tdata/ayu/unigif/<id>.mp4`, then sent
  through the normal file path, so it goes out as a looping GIF and respects
  reply, silent and scheduled sends. `POST /gifs/{id}/use` is pinged after.
  Nothing is added to Telegram's saved GIFs.
- Add: message context menu → "Add to UniGIF" uploads the document with
  `source=telegram&source_id=<document id>&wait=0`, and reports "already in
  your library" for a duplicate.
- Delete: right-click a library tile → "Delete from UniGIF" (moves it to the
  server's trash). Shown only when `/me` reported the `delete` scope.
- Media URLs are fetched without the key. API calls never follow redirects.

## Where it hooks into upstream code

`ApiWrap::sendInlineResult` (one early branch), `GifsListWidget` (mode flag,
data source, selection, context menu), `ChatHelpers::AddGifAction` and the two
message context menus (label and action), and `RecentInlineBots::bump` (ignores
non-bots). Everything else is under `ayu/features/unigif/`.

## Status

Written without a build: the repository's libraries are not available where it
was written. `ayu/features/unigif/unigif_client.cpp`, `unigif_query.cpp` and
`unigif_config.cpp` were compiled with Qt 6.4 and exercised against a mock
server (auth header only on API calls, query encoding, paging, upload, delete,
download, 429 back-off, error mapping). Everything that touches Telegram
internals (panel, send, add, settings screen, lang strings) has only been read
against the existing code and still needs a real build and a manual pass.

Not implemented compared with Retrogram: the hashtag suggestion chips under the
search field, and the "Newest" list. Captions on UniGIF sends are not offered.
The other places that search the GIF bot (story editor, photo picker) are
unchanged.
