#pragma once

// Simplified Chinese strings; Korean strings cover untranslated entries.
const UiStrings SimplifiedChineseStrings = [] {
    UiStrings text = KoreanStrings;
    text.languageCode = L"zh-CN";
    text.nativeName = L"简体中文";
    text.languageFileChoose = L"选择语言文件...";
    text.languageFileTitle = L"选择语言文件";
    text.languageFileFilter = L"语言文件\0*.h\0所有文件\0*.*\0\0";
    text.languageFileError = L"无法加载语言文件。请确认文件存在且语言文件格式正确。";
    text.background = L"背景显示";
    text.ticks = L"刻度显示";
    text.numbers = L"数字显示";
    text.alarmAdd = L"添加闹钟";
    text.alarmEdit = L"编辑闹钟";
    text.alarmDelete = L"删除闹钟";
    text.effectVolume = L"音效音量调节";
    text.alarmFileChoose = L"选择闹钟文件...";
    text.alarmDefault = L"默认闹钟";
    text.volume = L"音量调节";
    text.alarmSound = L"闹钟声音调节";
    text.alwaysOnTop = L"始终置顶显示";
    text.topmostExceptions = L"置顶显示例外";
    text.noPrograms = L"(无程序)";
    text.programChoose = L"选择程序...";
    text.runAtStartup = L"开机自动运行";
    text.startupError = L"无法更改启动设置。请检查可执行文件路径长度和注册表访问权限。";
    text.taskbar = L"在任务栏显示";
    text.layout = L"调整大小·位置";
    text.displaySelect = L"显示屏幕";
    text.displayPrefix = L"显示器 ";
    text.displayPrimary = L" · 主";
    text.exit = L"退出";
    text.autoUpdateCheck = L"自动检查更新";
    text.githubVisit = L"访问 Github";
    text.updateAvailable = L"有最新更新。要下载吗？";
    text.updateVisitGithub = L"访问 Github 页面";
    text.updateIgnore = L"忽略本次更新";
    text.updateCancel = L"取消";
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
    text.alarmFileFilter =
        L"支持的媒体\0*.wav;*.mp3;*.m4a;*.aac;*.flac;*.wma;*.mp4;*.m4v;*.mov;*.wmv\0"
        L"音频文件\0*.wav;*.mp3;*.m4a;*.aac;*.flac;*.wma\0"
        L"视频文件\0*.mp4;*.m4v;*.mov;*.wmv\0\0";
    text.alarmFileTitle = L"选择闹钟声音文件";
    text.programFileFilter = L"程序\0*.exe\0所有文件\0*.*\0\0";
    text.programFileTitle = L"选择置顶例外程序";
    text.mediaPackMissing = L"缺少 Windows 媒体包，可能无法正常运行。";
    text.alarmFileError = L"播放所选闹钟文件时发生错误";
    text.artworkLoadError = L"无法加载 PNG 动画、指针图像或 locate data 中的坐标设置。";
    text.artworkLoadHelp = L"请重新运行 build.cmd，并将 bin 文件夹与 sources 文件夹放在一起。";
    text.standaloneArtworkLoadHelp = L"请重新下载分发版 EXE，或删除用户数据文件夹中的 assets 文件夹后重新运行。";
    text.clockWindowError = L"无法创建时钟窗口。";
    text.standaloneDataError = L"无法在 LocalAppData 中准备 SkeaClock 数据。请检查磁盘剩余空间和文件夹权限。";
    return text;
}();
