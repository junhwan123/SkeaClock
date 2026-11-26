#pragma once

// Simplified Chinese strings; Korean strings cover untranslated entries.
const UiStrings SimplifiedChineseStrings = [] {
    UiStrings text = KoreanStrings;
    text.languageCode = L"zh-CN";
    text.nativeName = L"简体中文";
    text.languageFileChoose = L"选择语言文件";
    text.languageFileTitle = L"选择语言文件";
    text.languageFiles = L"语言文件";
    text.allFiles = L"所有文件";
    text.background = L"背景显示";
    text.timeDisplay = L"时间显示";
    text.ticks = L"刻度显示";
    text.numbers = L"数字显示";
    text.hologram = L"全息效果";
    text.format24Hour = L"24小时制";
    text.alarmAdd = L"添加闹钟";
    text.alarmEdit = L"编辑闹钟";
    text.alarmDelete = L"删除闹钟";
    text.effectVolume = L"音效音量调节";
    text.alarmFileChoose = L"选择闹钟文件";
    text.alarmDefault = L"默认闹钟";
    text.volume = L"音量调节";
    text.alarmSound = L"闹钟声音调节";
    text.musicSound = L"音乐设置";
    text.musicFileChoose = L"选择音乐文件";
    text.musicAutoPlay = L"启动时自动播放音乐";
    text.tableDisplay = L"显示桌子";
    text.playerDisplay = L"显示播放器";
    text.alwaysOnTop = L"始终置顶显示";
    text.topmostExceptions = L"置顶显示例外";
    text.noPrograms = L"无程序";
    text.programChoose = L"选择程序";
    text.runAtStartup = L"开机自动运行";
    text.taskbar = L"在任务栏显示";
    text.layout = L"调整大小·位置";
    text.displaySelect = L"显示屏幕";
    text.displayPrefix = L"显示器";
    text.displayPrimary = L"主";
    text.exit = L"退出";
    text.autoUpdateCheck = L"自动检查更新";
    text.githubVisit = L"访问 Github";
    text.alarmDays = {
        L"日",
        L"一",
        L"二",
        L"三",
        L"四",
        L"五",
        L"六",
    };
    text.alarmNoDays = L"无星期";
    text.mediaFiles = L"支持的媒体";
    text.audioFiles = L"音频文件";
    text.videoFiles = L"视频文件";
    text.alarmFileTitle = L"选择闹钟声音文件";
    text.programFiles = L"程序";
    text.programFileTitle = L"选择置顶例外程序";

    // 提示和错误对话框
    text.updateAvailable = L"有最新更新";
    text.updateDownloadQuestion = L"要下载吗";
    text.updateDownload = L"前往下载";
    text.updateCancel = L"取消";
    text.settingsDowngradeWarning = L"当前运行的 Skea Clock 版本低于上次使用的版本";
    text.settingsDowngradeContinue = L"仍然运行";
    text.standaloneDataError = L"无法创建资源文件夹";
    text.standaloneDataHelp = L"请检查磁盘剩余空间和文件夹权限";
    text.artworkLoadError = L"资源发生错误";
    text.artworkRecoveryHelp = L"请重新启动程序";
    text.runtimeError = L"程序运行过程中发生错误";
    text.settingsVersionError = L"无法读取设置版本或更新设置数据";
    text.languageFileError = L"无法加载语言文件";
    text.languageFileHelp = L"请确认文件存在且语言文件格式正确";
    text.startupError = L"无法更改启动设置";
    text.startupHelp = L"请检查可执行文件路径长度和注册表访问权限";
    text.alarmFileError = L"播放所选闹钟文件时发生错误";
    text.mediaValidationStartError = L"无法开始检查音频文件";
    text.mediaValidationIncomplete = L"无法完成音频文件检查";
    text.mediaPackMissing = L"无法使用检查音频文件所需的 Windows 媒体功能";
    text.musicFileError = L"无法播放所选的音乐文件";
    return text;
}();
