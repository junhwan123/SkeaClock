#pragma once

#include "function-skea.h"
#include "function-mute.h"
#include "function-kommy.h"
#include "function-coffe.h"
#include "../interaction/interaction-coffe.h"

bool& FirstDisplayOption() {
    if (IsKommyCharacter()) return Kommy::showTime;
    if (IsMuteCharacter()) return Mute::showBackground;
    if (IsCoffeCharacter()) return Coffe::showBackground;
    return showDial;
}

bool& TickDisplayOption() {
    if (IsKommyCharacter()) return Kommy::showTicks;
    if (IsMuteCharacter()) return Mute::hologramEnabled;
    return showTicks;
}

bool& NumberDisplayOption() {
    if (IsKommyCharacter()) return Kommy::showNumbers;
    if (IsMuteCharacter()) return Mute::use24Hour;
    return showNumbers;
}

std::wstring artworkDirectory;

ClockLoadResult LoadArtwork(const std::wstring& sourceDirectory) {
    clockDial = std::make_unique<ClockDial>();
    if (!clockDial->Initialize(clockCenter)) return ClockLoadResult::AssetError;
    if (IsCoffeCharacter()) return Coffe::Load(sourceDirectory + L"characters\\Coffe\\");
    bool loaded;
    if (IsKommyCharacter()) loaded = Kommy::Load(sourceDirectory + L"characters\\Kommy\\");
    else if (IsMuteCharacter()) loaded = Mute::Load(sourceDirectory + L"characters\\Mute\\");
    else loaded = Skea::Load(sourceDirectory + L"characters\\Skea\\");
    return loaded ? ClockLoadResult::Loaded : ClockLoadResult::AssetError;
}

void ReleaseArtwork() {
    Coffe::Release();
    Mute::Release();
    Kommy::Release();
    Skea::Release();
    ClearInteractionState();
    animationSheets.clear();
    prepareAlarmAnimation = false;
    blankPlaybackActive = false;
    clockDial.reset();
}

void DrawArtwork(Graphics& graphics, const SYSTEMTIME& now) {
    if (IsCoffeCharacter()) {
        Coffe::Draw(graphics);
    } else if (IsKommyCharacter()) {
        Kommy::Draw(graphics, true, Kommy::showTicks, Kommy::showNumbers);
    } else if (IsMuteCharacter()) {
        Mute::Draw(graphics, now, Mute::showBackground);
    } else {
        Skea::Draw(graphics, now, FirstDisplayOption(), TickDisplayOption(), NumberDisplayOption());
    }
}

HitEllipse HeadTouchArea() {
    if (IsCoffeCharacter()) return Coffe::HeadTouchArea();
    if (IsKommyCharacter()) return Kommy::HeadTouchArea();
    if (IsMuteCharacter()) return Mute::HeadTouchArea();
    return Skea::HeadTouchArea();
}

PointF CheekTouchCenter() {
    if (IsCoffeCharacter()) return Coffe::CheekTouchCenter();
    if (IsKommyCharacter()) return Kommy::CheekTouchCenter();
    if (IsMuteCharacter()) return Mute::Place(Mute::cheekSource);
    return Skea::Place(Skea::cheekSource);
}

float CheekTouchRadius() {
    if (IsCoffeCharacter()) return Coffe::CheekTouchRadius();
    if (IsKommyCharacter()) return Kommy::CheekRadius();
    if (IsMuteCharacter()) return Mute::cheekRadius * Mute::Scale();
    return Skea::cheekRadius * Skea::scale;
}

void BeginAlarm(const SYSTEMTIME& now) {
    if (IsMuteCharacter()) Mute::BeginAlarm();
    else Skea::BeginAlarm(now);
}

void DismissAlarmToIdle() {
    if (IsMuteCharacter()) Mute::Reset();
    else Skea::DismissAlarmToIdle();
}

void BeginBlankPlayback() {
    if (IsCoffeCharacter()) Coffe::BeginBlank();
    else if (IsKommyCharacter()) Kommy::BeginBlank();
    else if (IsMuteCharacter()) Mute::BeginBlank();
    else Skea::BeginBlankPlayback();
}

void EndBlankPlayback() {
    if (!blankPlaybackActive) return;
    if (IsCoffeCharacter()) Coffe::EndBlank();
    else if (IsKommyCharacter()) Kommy::EndBlank();
    else if (IsMuteCharacter()) Mute::EndBlank();
    else Skea::EndBlankPlayback();
}

void BeginReaction() {
    if (IsCoffeCharacter()) Coffe::BeginSmash();
    else if (IsKommyCharacter()) Kommy::BeginSmash();
    else if (IsMuteCharacter()) Mute::BeginSmash();
    else Skea::BeginReaction();
}

void BeginPat() {
    if (IsCoffeCharacter()) Coffe::BeginPat();
    else if (IsKommyCharacter()) Kommy::BeginPat();
    else if (IsMuteCharacter()) Mute::BeginPat();
    else Skea::BeginPat();
}

void EndPat() {
    if (!patActive) return;
    if (IsCoffeCharacter()) Coffe::EndPat();
    else if (IsKommyCharacter()) Kommy::EndPat();
    else if (IsMuteCharacter()) Mute::EndPat();
    else Skea::EndPat();
}

void BeginCheekPull() {
    if (IsCoffeCharacter()) Coffe::BeginTouch();
    else if (IsKommyCharacter()) Kommy::BeginTouch();
    else if (IsMuteCharacter()) Mute::BeginTouch();
    else Skea::BeginCheekPull();
}

void EndCheekPull() {
    if (!cheekDragging) return;
    touchReleasePullSource = cheekWarpBuffers.appliedPull;
    if (IsCoffeCharacter()) Coffe::EndTouch();
    else if (IsKommyCharacter()) Kommy::EndTouch();
    else if (IsMuteCharacter()) Mute::EndTouch();
    else Skea::EndCheekPull();
}
