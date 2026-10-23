#pragma once

// Keep menu terms from README.ja.md; Korean strings cover untranslated entries.
const UiStrings JapaneseStrings = [] {
    UiStrings text = KoreanStrings;
    text.languageCode = L"ja";
    text.nativeName = L"日本語";
    text.languageFileChoose = L"言語ファイルを選択...";
    text.languageFileTitle = L"言語ファイルを選択";
    text.languageFileFilter = L"言語ファイル\0*.h\0すべてのファイル\0*.*\0\0";
    text.languageFileError = L"言語ファイルを読み込めませんでした。ファイルが存在し、正しい言語ファイル形式であることを確認してください。";
    text.background = L"背景表示";
    text.ticks = L"目盛り表示";
    text.numbers = L"数字表示";
    text.alarmAdd = L"アラームを追加";
    text.alarmEdit = L"アラームを編集";
    text.alarmDelete = L"アラームを削除";
    text.effectVolume = L"効果音の音量調整";
    text.alarmFileChoose = L"アラームファイルの選択...";
    text.alarmDefault = L"既定のアラーム音";
    text.volume = L"音量調整";
    text.alarmSound = L"アラーム音の調整";
    text.alwaysOnTop = L"常に手前に表示";
    text.topmostExceptions = L"「常に手前に表示」の例外";
    text.noPrograms = L"(プログラムなし)";
    text.programChoose = L"プログラムを選択...";
    text.runAtStartup = L"Windows 起動時に自動実行";
    text.startupError = L"スタートアップの設定を変更できませんでした。実行ファイルのパスの長さとレジストリへのアクセス権限を確認してください。";
    text.taskbar = L"タスクバーに表示";
    text.layout = L"サイズ・位置の調整";
    text.displaySelect = L"表示先ディスプレイ";
    text.displayPrefix = L"ディスプレイ ";
    text.displayPrimary = L" · メイン";
    text.exit = L"終了";
    text.autoUpdateCheck = L"更新を自動チェック";
    text.githubVisit = L"Github を開く";
    text.updateAvailable = L"新しい更新があります。ダウンロードしますか？";
    text.updateVisitGithub = L"Github ページを開く";
    text.updateIgnore = L"この更新をスキップ";
    text.updateCancel = L"キャンセル";
    text.alarmDays = {
        L"日",
        L"月",
        L"火",
        L"水",
        L"木",
        L"金",
        L"土",
    };
    text.alarmNoDays = L"曜日なし";
    text.alarmFileFilter =
        L"対応するメディア\0*.wav;*.mp3;*.m4a;*.aac;*.flac;*.wma;*.mp4;*.m4v;*.mov;*.wmv\0"
        L"オーディオファイル\0*.wav;*.mp3;*.m4a;*.aac;*.flac;*.wma\0"
        L"動画ファイル\0*.mp4;*.m4v;*.mov;*.wmv\0\0";
    text.alarmFileTitle = L"アラーム音のファイルを選択";
    text.programFileFilter = L"プログラム\0*.exe\0すべてのファイル\0*.*\0\0";
    text.programFileTitle = L"「常に手前に表示」の例外にするプログラムを選択";
    text.mediaPackMissing = L"Windows メディアパックがありません。正常に動作しない可能性があります。";
    text.alarmFileError = L"選択したアラームファイルの再生中にエラーが発生しました";
    text.artworkLoadError = L"PNG アニメーション、時計の針の画像、または locate data の座標設定を読み込めませんでした。";
    text.artworkLoadHelp = L"build.cmd を再実行し、bin フォルダーと sources フォルダーを一緒に保管してください。";
    text.standaloneArtworkLoadHelp = L"配布用 EXE を再ダウンロードするか、ユーザーデータフォルダーの assets フォルダーを削除してから再実行してください。";
    text.clockWindowError = L"時計のウィンドウを作成できませんでした。";
    text.standaloneDataError = L"LocalAppData に SkeaClock のデータを準備できませんでした。ディスクの空き容量とフォルダーのアクセス許可を確認してください。";
    return text;
}();
