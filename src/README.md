# Skea Clock 소스 구조

[앱 진입점](../skea-clock.cpp)은 Windows 메시지와 타이머를 받아 각 기능에 전달합니다. 프로그램은 이 C++ 파일을 중심으로 컴파일하므로 헤더에 함수 구현과 실행 상태가 함께 들어 있습니다. 공용 입력 상태는 `interaction.h`에서 먼저 정의되고, 아래에서 포함하는 렌더링과 캐릭터 코드가 그 상태를 사용합니다.

## `app` · 시작 준비와 업데이트

| 파일 | 담당 기능 |
|---|---|
| [preload.h](app/preload.h) | 내장 언어와 AppData 경로를 준비하고 설정 버전 확인을 호출합니다. INI의 누락된 키에 기본값을 채웁니다. 자료 로딩을 마치면 `[updates]`의 `currentVersion`에 EXE에서 읽은 릴리스 버전 세 숫자를 기록합니다. GDI+ 초기화와 앱 아이콘 로딩도 담당합니다. |
| [embedded-assets.h](app/embedded-assets.h) | `%LOCALAPPDATA%\SkeaClock` 경로를 구하고, 빌드에서 생성한 `assets.h` 목록을 읽어 EXE의 내장 자료를 해상도별 폴더에 풉니다. |
| [settings-migration.h](app/settings-migration.h) | 저장된 릴리스 버전에 따라 INI 키와 모니터 배치를 이전합니다. 버전 기록이 없는 AppData의 초기화와 하위 버전 실행 경고를 처리합니다. |
| [update.h](app/update.h) | 7일 간격으로 GitHub 최신 릴리스를 백그라운드에서 확인하고 실행 중인 EXE 파일 버전의 앞 세 숫자와 비교합니다. 새 버전 알림과 `[updates]`의 마지막 확인 시각을 관리합니다. |

## `menu` · 우클릭 메뉴, 설정 저장, 알람 편집 창

| 파일 | 담당 기능 |
|---|---|
| [menu.h](menu/menu.h) | 우클릭 메뉴를 열고 명령을 각 기능에 연결합니다. 캐릭터·표시·모니터·언어 변경을 처리하며, 일반 설정·소리·코페 음악·알람 목록·항상 위 예외 목록의 INI 저장 함수를 둡니다. |
| [menu-sound.h](menu/menu-sound.h) | 공용 효과음 볼륨, 알람음 선택, 코페 음악 파일·볼륨·자동 재생 메뉴를 구성합니다. 소리 관련 명령 번호와 지원 미디어 파일을 고르는 창도 관리합니다. |
| [alarm-editor.h](menu/alarm-editor.h) | 시·분·요일을 입력하는 알람 편집 창의 배치와 입력 검증을 담당합니다. 등록된 값은 알람 목록에 추가하고 `menu.h`의 저장 함수를 호출합니다. |
| [menu-exceptions.h](menu/menu-exceptions.h) | 항상 위 예외 하위 메뉴를 만듭니다. 실행 중인 앱 경로와 등록된 예외 경로를 합치고, 아이콘을 백그라운드에서 불러옵니다. EXE 직접 선택도 여기서 처리합니다. |
| [menu-icons.h](menu/menu-icons.h) | 이름으로 메뉴 아이콘 리소스를 불러오고, Windows 메뉴가 사용할 비트맵으로 변환합니다. DPI와 시스템 메뉴 색을 적용하고 메뉴가 닫힌 뒤 비트맵을 해제합니다. |

## `language` · 언어 자료 읽기와 적용

| 파일 | 담당 기능 |
|---|---|
| [language.h](language/language.h) | UI 문자열 구조와 언어 선택 상태를 정의합니다. EXE에 포함된 기본 언어와 사용자 지정 UTF-8 언어 파일을 같은 파서로 읽고, 누락된 번역을 한국어로 보충합니다. 사용자 파일을 다시 읽어 적용하거나 오류를 표시합니다. |

내장 문구는 [`languages`](../languages)의 언어별 헤더에서 관리합니다. [리소스 스크립트](../skea-clock.rc)가 이 헤더를 텍스트 자료로 EXE에 포함합니다. [언어 편집기](../language-editor)는 별도로 사용합니다.

## `window` · Windows 창 표시와 배치

| 파일 | 담당 기능 |
|---|---|
| [window-render.h](window/window-render.h) | 캐릭터, 시계 표시, 상호작용 효과, 코미 시간 패널, 배치 편집 화면을 그리기 버퍼에 합성해 투명 창에 표시합니다. 캐릭터별 애니메이션과 코미 카운트다운을 갱신하고 다음 프레임 타이머를 예약합니다. |
| [window-layout.h](window/window-layout.h) | 모니터 선택, DPI에 따른 창 크기와 작업 영역 내 위치, 배치 편집 손잡이와 드래그를 처리합니다. 코미 시간 패널에 필요한 창 여백도 계산합니다. |
| [window-layout-settings.h](window/window-layout-settings.h) | 모니터별 위치와 크기를 `[displaydata]`에 `DISPLAY1=x,y,size` 형식으로 읽고 씁니다. 기존 모니터별 섹션을 읽는 함수는 `settings-migration.h`에서도 사용합니다. |
| [window-behavior.h](window/window-behavior.h) | 항상 위, 작업 표시줄 표시, 포그라운드 전환을 Windows 창에 적용합니다. 예외 프로그램이 앞에 오면 그 창 뒤에 시계를 놓는 동작과 예외 목록 상태를 관리합니다. |
| [clock-startup.h](window/clock-startup.h) | Windows의 현재 사용자 시작 프로그램에 현재 EXE를 등록하거나 등록을 삭제합니다. 메뉴의 체크 상태에는 INI의 `[clock] runAtStartup`을 사용합니다. |

## `clock` · 캐릭터 기능과 알람 예약

| 파일 | 담당 기능 |
|---|---|
| [character.h](clock/character.h) | 캐릭터의 번호·표시 이름·배경색·테두리색을 정의하고 현재 캐릭터를 판별합니다. |
| [function.h](clock/function.h) | 화면 표시 옵션, 자료 로딩·해제, 그리기, 입력 범위, 알람·상호작용 호출을 선택된 캐릭터의 구현으로 전달합니다. |
| [function-skea.h](clock/function-skea.h) | 스키아의 아날로그 시계판과 시침·분침을 그립니다. 실제 시각을 바늘 각도로 바꾸고, 상호작용이나 알람 중의 바늘 변형과 복귀를 처리합니다. |
| [function-mute.h](clock/function-mute.h) | 뮤트의 디지털 시각, 12·24시간제, 홀로그램, 알람 문구와 글리치를 그립니다. |
| [function-kommy.h](clock/function-kommy.h) | 코미의 남은 시간과 혀 모양 초침을 조작합니다. 카운트다운, 종료음 호출, 코미의 눈금·테두리·남은 시간 패널을 담당합니다. |
| [function-coffe.h](clock/function-coffe.h) | 코페의 레코드 원호, 재생 아이콘, 테이블과 소품, 연기와 잔의 페이드를 그립니다. `CoffeFiles`에서 검증된 원본의 임시 음악 파일을 백그라운드로 복사하고 정리합니다. |
| [alarm.h](clock/alarm.h) | 스키아·뮤트 알람의 시각·요일과 발동 상태를 관리합니다. 알람 시각을 대조하고, 시작 전에 애니메이션 준비를 요청하며, 알람을 시작하거나 종료합니다. |

## `animation` · PNG 청크와 캐릭터별 재생 순서

| 파일 | 담당 기능 |
|---|---|
| [animation.h](animation/animation.h) | `locate data`의 프레임 위치·오프셋·좌표를 읽고, 30fps 시간 계산과 레이어의 동시 진행을 제공합니다. 다음 청크의 준비와 배율을 반영한 프레임 그리기에 쓰입니다. 일반판과 축소판의 이미지 기준 배율도 여기 있습니다. |
| [animation-sheet.h](animation/animation-sheet.h) | 여러 프레임이 담긴 PNG 청크를 픽셀 버퍼로 풀고 각 프레임이 같은 버퍼의 지정 영역을 참조하게 합니다. 경로별 시트를 공유하고, 사전 로딩과 더 이상 필요한 표시가 없는 시트의 해제를 담당합니다. |
| [animation-skea.h](animation/animation-skea.h) | 스키아의 대기·알람·기본 상호작용·배치 편집용 클립 전환과 사전 로딩을 정합니다. 프레임별 볼 변형 중심과 바늘이 표시될 시점·중심을 관리합니다. |
| [animation-mute.h](animation/animation-mute.h) | 뮤트의 클립과 사전 로딩을 관리합니다. 쓰다듬기 복귀 중 다시 누르는 동작, 알람 중 반응, 꿀밤에 따른 글리치 시작 신호를 처리합니다. |
| [animation-kommy.h](animation/animation-kommy.h) | 코미의 기본 동작과 타이머 종료 반응을 재생합니다. 대기 장면에 자연스럽게 복귀할 위치를 맞추고, 혀 초침의 표시 시점과 볼 변형 좌표를 정합니다. |
| [animation-coffe.h](animation/animation-coffe.h) | 코페의 커피 단계, 기본 상호작용, 배치 편집용 클립의 전환과 사전 로딩을 정합니다. 고양이 잔 선택과 연기·잔 프레임 진행, 커피 효과음이 이어지는 시점을 관리합니다. |

## `interaction` · 입력 판정과 반응 효과

| 파일 | 담당 기능 |
|---|---|
| [interaction.h](interaction/interaction.h) | 공용 상호작용 상태와 머리·볼 범위를 정의하고, 마우스 누름·이동·해제·취소를 처리합니다. 꿀밤과 쓰다듬기 구분, 볼 당기기 방향, 창 배치·코미 초침·코페 소품으로의 입력 전달을 맡습니다. |
| [interaction-coffe.h](interaction/interaction-coffe.h) | 코페 플레이어 버튼과 소품 입력을 처리합니다. 공용 흐름에서 받은 드래그 좌표로 소품을 건네는 범위를 판정하고, 커피 단계 전환과 커피 만들기·마시기 전용 효과음을 연결합니다. |
| [interaction-effects.h](interaction/interaction-effects.h) | 꿀밤 별과 쓰다듬기 하트의 발생 시점, 이동, 투명도 변화와 그리기를 관리합니다. |

## `graphics` · 도형 자료와 픽셀 변형

| 파일 | 담당 기능 |
|---|---|
| [clock-shape.h](graphics/clock-shape.h) | 240×240 시계 기준 좌표, 원형 클리핑, 공용 테두리, 둥근 우하단 모서리와 원형 버튼의 도형을 정의합니다. 스키아·코미가 사용하는 눈금과 숫자 경로의 초기화 함수도 둡니다. |
| [skea-dial.h](graphics/skea-dial.h) | 스키아 아날로그 시계의 눈금과 숫자 1~12 외곽선을 고정 좌표로 담습니다. |
| [kommy-dial.h](graphics/kommy-dial.h) | 코미의 초 눈금과 5초 단위 숫자 외곽선, 테두리 폭과 남은 시간 패널의 도형 기준을 담습니다. |
| [digital-segments.h](graphics/digital-segments.h) | 뮤트·코미의 디지털 표시에서 사용하는 숫자 조각의 좌표와 0~9 조합을 정의합니다. |
| [warp.h](graphics/warp.h) | 마우스 변형량을 기준으로 쓰다듬기·볼 당기기 픽셀을 변형하고 뒤·머리·앞 레이어를 합성합니다. 같은 프레임의 변형과 합성 결과를 재사용합니다. |

## `audio` · 공용 재생과 사용자 음원 검증

| 파일 | 담당 기능 |
|---|---|
| [sound.h](audio/sound.h) | 공용 효과음과 캐릭터 음성을 재생하는 WAV 채널, 볼륨 보정, 지원 파일 확장자와 Media Foundation 재생기 초기화·해제를 제공합니다. |
| [media-validation.h](audio/media-validation.h) | 사용자 지정 알람음과 코페 음원의 원본을 백그라운드에서 검사합니다. 무음 재생으로 오디오 트랙과 재생 시작을 확인하고, 전체 오디오를 디코딩합니다. 검증한 원본의 파일 식별 정보와 수정 여부를 비교하는 함수도 제공합니다. |
| [alarm-sound.h](audio/alarm-sound.h) | 스키아·뮤트 공유 알람음과 코미 전용 알람음의 선택 상태를 관리합니다. 검증이 성공하면 사용자 지정음 사용을 확정하고, 실제 알람에서 원본이 사라졌거나 바뀌었으면 선택을 해제합니다. 스키아·뮤트의 알람음 재생과 공용 알람 엔진도 담당합니다. |
| [timer-sound.h](audio/timer-sound.h) | 코미의 초침 소리 채널과 타이머 종료음을 담당합니다. 종료음은 유효한 사용자 지정음을 한 번 재생하거나 코미의 기본 WAV를 사용합니다. |
| [music.h](audio/music.h) | 코페 곡 선택 검증의 결과, 재생·일시정지·반복, 볼륨과 자동 재생을 관리합니다. 원본이 검증되면 선택을 확정하고 `CoffeFiles`에 복사를 요청합니다. 복사 완료 후 재생하며, 반복하거나 재개할 때 원본의 변경·삭제를 확인합니다. |

## 새 기능을 연결할 때 찾을 곳

- **설정이 하나 늘어날 때**는 값을 사용하는 기능에 실행 상태를 두고, 시작 시 읽을 값과 기본값은 `preload.h`에, 메뉴 동작과 저장은 `menu.h`에 연결합니다. 공용 값은 `[clock]`, 캐릭터별 값은 `[skea]`·`[mute]`·`[kommy]`·`[coffe]`에 둡니다. 모니터 배치와 업데이트 실행 기록은 각 담당 파일에서 저장합니다.
- **설정 형식이나 기존 값의 의미를 바꿀 때**는 [빌드 스크립트](../distribution/build.ps1)의 `$productVersion`을 올립니다. 릴리스 버전을 올릴 때마다 이전 버전의 INI와 비교하고, 이전이 필요한 변경은 `settings-migration.h`의 `ApplySettingsMigrations`에 추가합니다. 기존 값의 이동·변환·삭제·유지 방식과 대상 키에 이미 값이 있는 경우의 우선순위를 코드에 명시합니다. 이전 규칙은 저장된 버전보다 높고 실행 버전 이하인 것부터 차례대로 적용합니다. 새 키의 기본값은 `preload.h`에서 채웁니다.
- **캐릭터 그림을 바꿀 때**는 재생 순서·진입 프레임·사전 로딩을 `animation-*.h`에서, 화면 표시나 시계 기능을 `function-*.h`에서 찾습니다. 고정된 시계판 좌표는 `graphics`의 해당 캐릭터 자료에 넣습니다.
- **마우스 동작을 추가할 때**는 `interaction.h`의 공용 누름·이동·해제·취소 흐름에 연결합니다. 코페 소품의 놓는 범위와 단계 판정은 `interaction-coffe.h`에서 처리합니다.
- **소리를 추가할 때**는 공용 WAV 재생은 `sound.h`, 코미 초침·타이머 종료음은 `timer-sound.h`, 커피 단계 효과음은 `interaction-coffe.h`를 사용합니다. 사용자 음원을 활성화하는 시점은 `media-validation.h`의 완료 결과에 맞춥니다.

[루트 빌드 명령](../build.cmd)은 [`sources`](../sources)의 자료를 사용해 `distribution`에 일반·축소 통합판을 만듭니다. 실행하면 EXE가 해상도별로 내장 자료를 풀고, 두 통합판은 `%LOCALAPPDATA%\SkeaClock\skea-clock.ini`를 공유합니다. 원본 제작 자료는 개인 `resources` 폴더에서 관리하며, `sources` 갱신은 사용자 요청에 따라 수동으로 진행합니다.
