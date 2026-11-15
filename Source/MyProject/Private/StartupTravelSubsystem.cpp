#include "StartupTravelSubsystem.h"
#include "StudioFlowManager.h"
#include "StudioGameMode.h"
#include "WardrobeWidget.h"
#include "Components/Button.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/GameViewportClient.h"
#include "Engine/World.h"
#include "Framework/Application/SlateApplication.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/PackageName.h"
#include "Styling/CoreStyle.h"
#include "UObject/UObjectGlobals.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Layout/SBorder.h"

namespace
{
    class SStartupBlackShield final:public SCompoundWidget
    {
    public:
        SLATE_BEGIN_ARGS(SStartupBlackShield) {} SLATE_END_ARGS()
        void Construct(const FArguments& Args)
        { ChildSlot[SNew(SBorder).BorderImage(FCoreStyle::Get().GetBrush(TEXT("WhiteBrush"))).BorderBackgroundColor(FLinearColor::Black)]; }
        virtual bool SupportsKeyboardFocus() const override { return true; }
        virtual FReply OnKeyDown(const FGeometry&,const FKeyEvent&) override { return FReply::Handled(); }
        virtual FReply OnKeyUp(const FGeometry&,const FKeyEvent&) override { return FReply::Handled(); }
        virtual FReply OnMouseButtonDown(const FGeometry&,const FPointerEvent&) override { return FReply::Handled(); }
        virtual FReply OnMouseButtonUp(const FGeometry&,const FPointerEvent&) override { return FReply::Handled(); }
        virtual FReply OnAnalogValueChanged(const FGeometry&,const FAnalogInputEvent&) override { return FReply::Handled(); }
    };
}
UStartupTravelSubsystem* UStartupTravelSubsystem::Find(const UObject* Context)
{
    UWorld* World=Context?Context->GetWorld():nullptr; auto* Instance=World?World->GetGameInstance():nullptr;
    return Instance?Instance->GetSubsystem<UStartupTravelSubsystem>():nullptr;
}
void UStartupTravelSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    MapHandle=FCoreUObjectDelegates::PostLoadMapWithWorld.AddUObject(this,&UStartupTravelSubsystem::OnMapLoaded);
    if (GEngine) { FailureHandle=GEngine->OnTravelFailure().AddUObject(this,&UStartupTravelSubsystem::OnTravelFailed); }
}
void UStartupTravelSubsystem::Deinitialize()
{
    FCoreUObjectDelegates::PostLoadMapWithWorld.Remove(MapHandle);
    if (GEngine) { GEngine->OnTravelFailure().Remove(FailureHandle); }
    RemoveBlack(false); Super::Deinitialize();
}
bool UStartupTravelSubsystem::BeginGameplayTravel(const TSoftObjectPtr<UWorld>& Level,double FadeSeconds)
{
    const FString Package=Level.ToSoftObjectPath().GetLongPackageName();
    if (bTravelPending || !GetWorld() || !FPackageName::DoesPackageExist(Package) || !GEngine || !GEngine->GameViewport) { return false; }
    bTravelPending=true; bIntentConsumed=bMapLoaded=bLevelReady=false; FadeDuration=FMath::Max(0.0,FadeSeconds); FadeElapsed=0; ReadyFrames=0; BlackOpacity=1;
    BlackShield=SNew(SStartupBlackShield); GEngine->GameViewport->AddViewportWidgetContent(BlackShield.ToSharedRef(),10000);
    FSlateApplication::Get().SetKeyboardFocus(BlackShield,EFocusCause::SetDirectly);
    ++GameplayLoads; UE_LOG(LogTemp,Display,TEXT("StartupVideo: black shield active; loading %s"),*Package);
    UGameplayStatics::OpenLevelBySoftObjectPtr(GetWorld(),Level,true); return true;
}
bool UStartupTravelSubsystem::ConsumeGameplayIntent()
{
    if (!bTravelPending || bIntentConsumed) { return false; } bIntentConsumed=true; return true;
}
void UStartupTravelSubsystem::MarkGameplayReady() { bLevelReady=true; ReadyFrames=0; FadeElapsed=0; }
void UStartupTravelSubsystem::OnMapLoaded(UWorld* World)
{
    if (!bTravelPending || !World || World->GetGameInstance()!=GetGameInstance()) { return; }
    bMapLoaded=true;
}
void UStartupTravelSubsystem::OnTravelFailed(UWorld* World,ETravelFailure::Type Type,const FString& Error)
{
    if (!bTravelPending || !World || World->GetGameInstance()!=GetGameInstance()) { return; }
    UE_LOG(LogTemp,Warning,TEXT("StartupVideo: level travel failed: %s"),*Error);
    bTravelPending=false; bIntentConsumed=false;
    if (auto* Flow=AStudioFlowManager::Find(World)) { Flow->ReturnToMainMenu(); }
    RemoveBlack(false);
}
bool UStartupTravelSubsystem::IsTickable() const { return !IsTemplate() && bTravelPending; }
TStatId UStartupTravelSubsystem::GetStatId() const { RETURN_QUICK_DECLARE_CYCLE_STAT(UStartupTravelSubsystem,STATGROUP_Tickables); }
UWorld* UStartupTravelSubsystem::GetTickableGameObjectWorld() const { return GetWorld(); }
void UStartupTravelSubsystem::Tick(float DeltaTime)
{
    if (!bTravelPending) { return; }
    UWorld* World=GetWorld(); APlayerController* Player=World?UGameplayStatics::GetPlayerController(World,0):nullptr;
    if (bMapLoaded && !bLevelReady && Player && Player->GetPawn() && !World->GetAuthGameMode<AStudioGameMode>()) { MarkGameplayReady(); }
    if (BlackShield.IsValid() && FSlateApplication::IsInitialized()) { FSlateApplication::Get().SetKeyboardFocus(BlackShield,EFocusCause::SetDirectly); }
    if (!bLevelReady || !bMapLoaded || ++ReadyFrames<3) { return; }
    FadeElapsed+=DeltaTime; BlackOpacity=FadeDuration>0?static_cast<float>(1-FMath::Clamp(FadeElapsed/FadeDuration,0.0,1.0)):0.f;
    if (BlackShield.IsValid()) { BlackShield->SetRenderOpacity(BlackOpacity); }
    if (BlackOpacity<=0)
    {
        bTravelPending=false; bIntentConsumed=false;
        UE_LOG(LogTemp,Display,TEXT("StartupVideo: gameplay ready; removing black input shield"));
        RemoveBlack(true);
    }
}
void UStartupTravelSubsystem::RemoveBlack(bool bRestoreFocus)
{
    if (BlackShield.IsValid() && GEngine && GEngine->GameViewport) { GEngine->GameViewport->RemoveViewportWidgetContent(BlackShield.ToSharedRef()); }
    BlackShield.Reset(); BlackOpacity=0;
    if (!bRestoreFocus) { return; }
    APlayerController* Player=GetWorld()?UGameplayStatics::GetPlayerController(GetWorld(),0):nullptr;
    if (!Player) { return; }
    auto* Flow=AStudioFlowManager::Find(GetWorld());
    if (Flow && Flow->Movie && Flow->Movie->IsWardrobeOpen() && Flow->Movie->ActiveWardrobeWidget)
    {
        UWardrobeWidget* Wardrobe=Flow->Movie->ActiveWardrobeWidget;
        FInputModeGameAndUI Input; Input.SetWidgetToFocus(Wardrobe->TakeWidget()); Input.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock); Player->SetInputMode(Input);
        if (Wardrobe->ReadyButton) { Wardrobe->ReadyButton->SetKeyboardFocus(); } else { Wardrobe->SetKeyboardFocus(); }
    }
    else { Player->SetInputMode(FInputModeGameOnly()); Player->bShowMouseCursor=false; }
}
