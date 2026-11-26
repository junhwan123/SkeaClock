#pragma once

const UiStrings KoreanStrings = [] {
    UiStrings text = {};
    text.languageCode = L"ko";
    text.nativeName = L"한국어";
    text.languageFileChoose = L"언어 파일 선택";
    text.languageFileTitle = L"언어 파일 선택";
    text.languageFiles = L"언어 파일";
    text.allFiles = L"모든 파일";
    text.background = L"배경 표시";
    text.timeDisplay = L"시간 표시";
    text.ticks = L"눈금 표시";
    text.numbers = L"숫자 표시";
    text.hologram = L"홀로그램 효과";
    text.format24Hour = L"24시간 표시";
    text.alarmAdd = L"알람 추가";
    text.alarmEdit = L"알람 편집";
    text.alarmDelete = L"알람 삭제";
    text.effectVolume = L"효과음 크기 조절";
    text.alarmFileChoose = L"알람 파일 선택";
    text.alarmDefault = L"기본 알람";
    text.volume = L"크기 조절";
    text.alarmSound = L"알람 소리 조절";
    text.musicSound = L"노래 소리 조절";
    text.musicFileChoose = L"노래 파일 선택";
    text.musicAutoPlay = L"시계 실행 시 노래 자동 재생";
    text.tableDisplay = L"테이블 표시";
    text.playerDisplay = L"플레이어 표시";
    text.alwaysOnTop = L"항상 위에 표시";
    text.topmostExceptions = L"최상단 고정 예외";
    text.noPrograms = L"프로그램 없음";
    text.programChoose = L"프로그램 선택";
    text.runAtStartup = L"시작 시 자동 실행";
    text.taskbar = L"작업 표시줄 표시";
    text.layout = L"크기·위치 이동";
    text.displaySelect = L"표시 디스플레이";
    text.displayPrefix = L"디스플레이";
    text.displayPrimary = L"기본";
    text.exit = L"종료";
    text.autoUpdateCheck = L"업데이트 자동 체크";
    text.githubVisit = L"Github 방문";
    text.alarmDays = {
        L"일",
        L"월",
        L"화",
        L"수",
        L"목",
        L"금",
        L"토",
    };
    text.alarmNoDays = L"요일 없음";
    text.mediaFiles = L"지원되는 미디어";
    text.audioFiles = L"오디오 파일";
    text.videoFiles = L"동영상 파일";
    text.alarmFileTitle = L"알람 소리 파일 선택";
    text.programFiles = L"프로그램";
    text.programFileTitle = L"최상단 고정 예외 프로그램 선택";

    // 안내 및 오류 창
    text.updateAvailable = L"최신 업데이트가 있습니다";
    text.updateDownloadQuestion = L"다운받으시겠습니까";
    text.updateDownload = L"다운받으러 이동";
    text.updateCancel = L"취소";
    text.settingsDowngradeWarning = L"실행한 버전이 이전에 사용한 버전보다 낮습니다";
    text.settingsDowngradeContinue = L"무시하고 실행";
    text.standaloneDataError = L"에셋 폴더를 생성하지 못했습니다";
    text.standaloneDataHelp = L"저장공간 혹은 사용 권한을 체크해주세요";
    text.artworkLoadError = L"에셋에 오류가 발생했습니다";
    text.artworkRecoveryHelp = L"프로그램을 다시 실행해 주세요";
    text.runtimeError = L"실행 도중 오류가 발생했습니다";
    text.settingsVersionError = L"설정 버전을 읽거나 설정 데이터를 갱신하지 못했습니다";
    text.languageFileError = L"언어 파일을 불러오지 못했습니다";
    text.languageFileHelp = L"파일이 존재하는지와 올바른 언어 파일 형식인지 확인해 주세요";
    text.startupError = L"시작 프로그램 설정을 변경하지 못했습니다";
    text.startupHelp = L"실행 파일 경로 길이와 레지스트리 접근 권한을 확인해 주세요";
    text.alarmFileError = L"선택한 알람 파일 실행 도중 오류가 발생했습니다";
    text.mediaValidationStartError = L"오디오 파일 검사를 시작하지 못했습니다";
    text.mediaValidationIncomplete = L"오디오 파일 검사를 완료하지 못했습니다";
    text.mediaPackMissing = L"오디오 파일 검사에 필요한 Windows 미디어 기능을 사용할 수 없습니다";
    text.musicFileError = L"선택한 노래 파일을 재생하지 못했습니다";
    return text;
}();
