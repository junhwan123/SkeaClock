#pragma once

// Keep menu terms from README.ja.md; Korean strings cover untranslated entries.
const UiStrings JapaneseStrings = [] {
    UiStrings text = KoreanStrings;
    text.languageCode = L"ja";
    text.nativeName = L"日本語";
    text.languageFileChoose = L"言語ファイルを選択";
    text.languageFileTitle = L"言語ファイルを選択";
    text.languageFiles = L"言語ファイル";
    text.allFiles = L"すべてのファイル";
    text.background = L"背景表示";
    text.timeDisplay = L"時間表示";
    text.ticks = L"目盛り表示";
    text.numbers = L"数字表示";
    text.hologram = L"ホログラム効果";
    text.format24Hour = L"24時間表示";
    text.alarmAdd = L"アラームを追加";
    text.alarmEdit = L"アラームを編集";
    text.alarmDelete = L"アラームを削除";
    text.effectVolume = L"効果音の音量調整";
    text.alarmFileChoose = L"アラームファイルの選択";
    text.alarmDefault = L"既定のアラーム音";
    text.volume = L"音量調整";
    text.alarmSound = L"アラーム音の調整";
    text.musicSound = L"音楽の設定";
    text.musicFileChoose = L"音楽ファイルを選択";
    text.musicAutoPlay = L"時計の起動時に音楽を自動再生";
    text.tableDisplay = L"テーブルを表示";
    text.playerDisplay = L"プレーヤーを表示";
    text.alwaysOnTop = L"常に手前に表示";
    text.topmostExceptions = L"常に手前に表示する設定の例外";
    text.noPrograms = L"プログラムなし";
    text.programChoose = L"プログラムを選択";
    text.runAtStartup = L"Windows 起動時に自動実行";
    text.taskbar = L"タスクバーに表示";
    text.layout = L"サイズ・位置の調整";
    text.displaySelect = L"表示先ディスプレイ";
    text.displayPrefix = L"ディスプレイ";
    text.displayPrimary = L"メイン";
    text.exit = L"終了";
    text.autoUpdateCheck = L"更新を自動チェック";
    text.githubVisit = L"GitHub を開く";
    text.alarmDays = {
        L"日",
        L"月",
        L"火",
        L"水",
        L"木",
        L"金",
        L"土",
    };
    text.alarmNoDays = L"曜日の指定なし";
    text.mediaFiles = L"対応するメディア";
    text.audioFiles = L"オーディオファイル";
    text.videoFiles = L"動画ファイル";
    text.alarmFileTitle = L"アラーム音のファイルを選択";
    text.programFiles = L"プログラム";
    text.programFileTitle = L"常に手前に表示する設定の例外にするプログラムを選択";

    // 通知とエラーダイアログ
    text.updateAvailable = L"アップデートがあります";
    text.updateDownloadQuestion = L"ダウンロードしますか";
    text.updateDownload = L"ダウンロードページを開く";
    text.updateCancel = L"キャンセル";
    text.settingsDowngradeWarning = L"この Skea Clock は前回使用したものより古いバージョンです";
    text.settingsDowngradeContinue = L"そのまま起動";
    text.standaloneDataError = L"アセットフォルダーを作成できませんでした";
    text.standaloneDataHelp = L"ディスクの空き容量とアクセス許可を確認してください";
    text.artworkLoadError = L"アセットでエラーが発生しました";
    text.artworkRecoveryHelp = L"プログラムを再起動してください";
    text.runtimeError = L"プログラムの実行中にエラーが発生しました";
    text.settingsVersionError = L"設定のバージョンの読み込みまたは設定データの更新に失敗しました";
    text.languageFileError = L"言語ファイルを読み込めませんでした";
    text.languageFileHelp = L"ファイルが存在し正しい言語ファイル形式であることを確認してください";
    text.startupError = L"スタートアップの設定を変更できませんでした";
    text.startupHelp = L"実行ファイルのパスの長さとレジストリへのアクセス権限を確認してください";
    text.alarmFileError = L"選択したアラームファイルの再生中にエラーが発生しました";
    text.mediaValidationStartError = L"音声ファイルの検証を開始できませんでした";
    text.mediaValidationIncomplete = L"音声ファイルの検証を完了できませんでした";
    text.mediaPackMissing = L"音声ファイルの検証に必要な Windows のメディア機能を利用できません";
    text.musicFileError = L"選択した音楽ファイルを再生できませんでした";
    return text;
}();
