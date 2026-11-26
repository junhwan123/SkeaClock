#pragma once

const UiStrings EnglishStrings = [] {
    UiStrings text = KoreanStrings;
    text.languageCode = L"en";
    text.nativeName = L"English";
    text.languageFileChoose = L"Choose language file";
    text.languageFileTitle = L"Choose language file";
    text.languageFiles = L"Language files";
    text.allFiles = L"All files";
    text.background = L"Show background";
    text.timeDisplay = L"Show time";
    text.ticks = L"Show tick marks";
    text.numbers = L"Show numbers";
    text.hologram = L"Hologram effect";
    text.format24Hour = L"24 hour format";
    text.alarmAdd = L"Add alarm";
    text.alarmEdit = L"Edit alarm";
    text.alarmDelete = L"Delete alarm";
    text.effectVolume = L"Sound effect volume";
    text.alarmFileChoose = L"Choose alarm file";
    text.alarmDefault = L"Default alarm";
    text.volume = L"Volume";
    text.alarmSound = L"Alarm sound";
    text.musicSound = L"Music settings";
    text.musicFileChoose = L"Choose music file";
    text.musicAutoPlay = L"Play music at startup";
    text.tableDisplay = L"Show table";
    text.playerDisplay = L"Show player";
    text.alwaysOnTop = L"Always on top";
    text.topmostExceptions = L"Exceptions to Always on Top";
    text.noPrograms = L"No programs";
    text.programChoose = L"Choose program";
    text.runAtStartup = L"Run at startup";
    text.taskbar = L"Show on taskbar";
    text.layout = L"Adjust size and position";
    text.displaySelect = L"Display";
    text.displayPrefix = L"Display";
    text.displayPrimary = L"Primary";
    text.exit = L"Exit";
    text.autoUpdateCheck = L"Automatically check for updates";
    text.githubVisit = L"Visit GitHub";
    text.alarmDays = {
        L"Sun",
        L"Mon",
        L"Tue",
        L"Wed",
        L"Thu",
        L"Fri",
        L"Sat",
    };
    text.alarmNoDays = L"No days selected";
    text.mediaFiles = L"Supported media";
    text.audioFiles = L"Audio files";
    text.videoFiles = L"Video files";
    text.alarmFileTitle = L"Choose alarm sound file";
    text.programFiles = L"Programs";
    text.programFileTitle = L"Choose a program to exclude from Always on Top";

    // Notices and error dialogs
    text.updateAvailable = L"An update is available";
    text.updateDownloadQuestion = L"Would you like to download it";
    text.updateDownload = L"Open download page";
    text.updateCancel = L"Cancel";
    text.settingsDowngradeWarning = L"This version of Skea Clock is older than the version you used previously";
    text.settingsDowngradeContinue = L"Run anyway";
    text.standaloneDataError = L"Could not create the assets folder";
    text.standaloneDataHelp = L"Please check available disk space and access permissions";
    text.artworkLoadError = L"An asset error occurred";
    text.artworkRecoveryHelp = L"Please restart the program";
    text.runtimeError = L"An error occurred while running the program";
    text.settingsVersionError = L"Could not read the settings version or update Skea Clock settings";
    text.languageFileError = L"Could not load the language file";
    text.languageFileHelp = L"Check that it exists and is in the correct format";
    text.startupError = L"Could not change the startup setting";
    text.startupHelp = L"Please check the executable path length and registry access permissions";
    text.alarmFileError = L"An error occurred while playing the selected alarm sound file";
    text.mediaValidationStartError = L"Could not start the audio file check";
    text.mediaValidationIncomplete = L"Could not complete the audio file check";
    text.mediaPackMissing = L"The Windows media components needed to check the audio file are unavailable";
    text.musicFileError = L"Could not play the selected music file";
    return text;
}();
