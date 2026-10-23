#pragma once

const UiStrings EnglishStrings = [] {
    UiStrings text = KoreanStrings;
    text.languageCode = L"en";
    text.nativeName = L"English";
    text.languageFileChoose = L"Choose language file...";
    text.languageFileTitle = L"Choose language file";
    text.languageFileFilter = L"Language files\0*.h\0All files\0*.*\0\0";
    text.languageFileError = L"Could not load the language file. Check that the file exists and has a valid language file format.";
    text.background = L"Show background";
    text.ticks = L"Show tick marks";
    text.numbers = L"Show numbers";
    text.alarmAdd = L"Add alarm";
    text.alarmEdit = L"Edit alarm";
    text.alarmDelete = L"Delete alarm";
    text.effectVolume = L"Sound effect volume";
    text.alarmFileChoose = L"Choose alarm file...";
    text.alarmDefault = L"Default alarm";
    text.volume = L"Volume";
    text.alarmSound = L"Alarm sound";
    text.alwaysOnTop = L"Always on top";
    text.topmostExceptions = L"Always on top exceptions";
    text.noPrograms = L"(No programs)";
    text.programChoose = L"Choose program...";
    text.runAtStartup = L"Run at startup";
    text.startupError = L"Could not change the startup setting. Please check the executable path length and registry access permissions.";
    text.taskbar = L"Show on taskbar";
    text.layout = L"Adjust size and position";
    text.displaySelect = L"Display";
    text.displayPrefix = L"Display ";
    text.displayPrimary = L" · Primary";
    text.exit = L"Exit";
    text.autoUpdateCheck = L"Automatically check for updates";
    text.githubVisit = L"Visit Github";
    text.updateAvailable = L"An update is available. Would you like to download it?";
    text.updateVisitGithub = L"Visit Github page";
    text.updateIgnore = L"Skip this update";
    text.updateCancel = L"Cancel";
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
    text.alarmFileFilter =
        L"Supported media\0*.wav;*.mp3;*.m4a;*.aac;*.flac;*.wma;*.mp4;*.m4v;*.mov;*.wmv\0"
        L"Audio files\0*.wav;*.mp3;*.m4a;*.aac;*.flac;*.wma\0"
        L"Video files\0*.mp4;*.m4v;*.mov;*.wmv\0\0";
    text.alarmFileTitle = L"Choose alarm sound file";
    text.programFileFilter = L"Programs\0*.exe\0All files\0*.*\0\0";
    text.programFileTitle = L"Choose program for always on top exceptions";
    text.mediaPackMissing = L"The Windows Media Pack is missing. The app may not work correctly.";
    text.alarmFileError = L"An error occurred while playing the selected alarm file";
    text.artworkLoadError = L"Could not load the PNG animation, clock hand images, or coordinate settings in locate data.";
    text.artworkLoadHelp = L"Run build.cmd again and keep the bin and sources folders together.";
    text.standaloneArtworkLoadHelp = L"Download the standalone EXE again, or delete the assets folder in the user data folder and restart the app.";
    text.clockWindowError = L"Could not create the clock window.";
    text.standaloneDataError = L"Could not prepare SkeaClock data in LocalAppData. Please check available disk space and folder permissions.";
    return text;
}();
