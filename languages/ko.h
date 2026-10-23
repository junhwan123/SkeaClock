#pragma once

const UiStrings KoreanStrings = [] {
    UiStrings text = {};
    text.languageCode = L"ko";
    text.nativeName = L"한국어";
    text.languageFileChoose = L"언어 파일 선택...";
    text.languageFileTitle = L"언어 파일 선택";
    text.languageFileFilter = L"언어 파일\0*.h\0모든 파일\0*.*\0\0";
    text.languageFileError = L"언어 파일을 불러오지 못했습니다. 파일이 존재하는지와 올바른 언어 파일 형식인지 확인해 주세요.";
    text.background = L"배경 표시";
    text.ticks = L"눈금 표시";
    text.numbers = L"숫자 표시";
    text.alarmAdd = L"알람 추가";
    text.alarmEdit = L"알람 편집";
    text.alarmDelete = L"알람 삭제";
    text.effectVolume = L"효과음 크기 조절";
    text.alarmFileChoose = L"알람 파일 선택...";
    text.alarmDefault = L"기본 알람";
    text.volume = L"크기 조절";
    text.alarmSound = L"알람 소리 조절";
    text.alwaysOnTop = L"항상 위에 표시";
    text.topmostExceptions = L"최상단 고정 예외";
    text.noPrograms = L"(프로그램 없음)";
    text.programChoose = L"프로그램 선택...";
    text.runAtStartup = L"시작 시 자동 실행";
    text.startupError = L"시작 프로그램 설정을 변경하지 못했습니다. 실행 파일 경로 길이와 레지스트리 접근 권한을 확인해 주세요.";
    text.taskbar = L"작업 표시줄 표시";
    text.layout = L"크기·위치 이동";
    text.displaySelect = L"표시 디스플레이";
    text.displayPrefix = L"디스플레이 ";
    text.displayPrimary = L" · 기본";
    text.exit = L"종료";
    text.autoUpdateCheck = L"업데이트 자동 체크";
    text.githubVisit = L"Github 방문";
    text.updateAvailable = L"최신 업데이트가 있습니다. 다운받으시겠습니까?";
    text.updateVisitGithub = L"Github 페이지 방문";
    text.updateIgnore = L"이번 업데이트 무시";
    text.updateCancel = L"취소";
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
    text.alarmFileFilter =
        L"지원되는 미디어\0*.wav;*.mp3;*.m4a;*.aac;*.flac;*.wma;*.mp4;*.m4v;*.mov;*.wmv\0"
        L"오디오 파일\0*.wav;*.mp3;*.m4a;*.aac;*.flac;*.wma\0"
        L"동영상 파일\0*.mp4;*.m4v;*.mov;*.wmv\0\0";
    text.alarmFileTitle = L"알람 소리 파일 선택";
    text.programFileFilter = L"프로그램\0*.exe\0모든 파일\0*.*\0\0";
    text.programFileTitle = L"최상단 고정 예외 프로그램 선택";
    text.mediaPackMissing = L"윈도우 미디어 팩이 없습니다. 정상적으로 동작하지 않을 수 있습니다.";
    text.alarmFileError = L"선택한 알람 파일 실행 도중 오류가 발생했습니다";
    text.artworkLoadError = L"PNG 애니메이션, 바늘 이미지 또는 locate data의 좌표 설정을 불러오지 못했습니다.";
    text.artworkLoadHelp = L"build.cmd를 다시 실행하고, bin 폴더와 sources 폴더를 함께 보관해 주세요.";
    text.standaloneArtworkLoadHelp = L"배포용 EXE를 다시 받거나 사용자 데이터 폴더의 assets 폴더를 지운 뒤 다시 실행해 주세요.";
    text.clockWindowError = L"시계 창을 만들지 못했습니다.";
    text.standaloneDataError = L"LocalAppData에 SkeaClock 데이터를 준비하지 못했습니다. 디스크 여유 공간과 폴더 권한을 확인해 주세요.";
    return text;
}();
