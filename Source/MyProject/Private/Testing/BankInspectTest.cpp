// Runtime-only survey of the supplied level in a separate opt-in process; no project assets are saved.
#if !UE_BUILD_SHIPPING
#include "MovieManager.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Animation/SkeletalMeshActor.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "HAL/FileManager.h"
#include "HAL/IConsoleManager.h"
#include "HAL/PlatformMisc.h"
#include "HAL/PlatformTime.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/CommandLine.h"
#include "Misc/FileHelper.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "TimerManager.h"
#include "UnrealClient.h"

namespace BankInspectTest
{
    class FRun
    {
    public:
        TWeakObjectPtr<UWorld> World;
        TWeakObjectPtr<ACameraActor> Camera;
        TWeakObjectPtr<AStaticMeshActor> Bank;
        FTimerHandle Timer;
        FVector Center,Extent;
        double Started=FPlatformTime::Seconds(),ChangedAt=0;
        int32 Shot=0;
        bool bConfigured=false,bRequested=false,bDone=false;
        FString Directory;
        bool bCharacter=false;
        void Finish(bool bPass)
        {
            if (bDone) { return; } bDone=true;
            IFileManager::Get().MakeDirectory(*Directory,true);
            FFileHelper::SaveStringToFile(FString::Printf(TEXT("{\"passed\":%s,\"shots\":%d,\"center\":[%.3f,%.3f,%.3f],\"extent\":[%.3f,%.3f,%.3f]}"),bPass?TEXT("true"):TEXT("false"),Shot,Center.X,Center.Y,Center.Z,Extent.X,Extent.Y,Extent.Z),*(Directory/TEXT("result.json")));
            if (World.IsValid()) { World->GetTimerManager().ClearTimer(Timer); }
            FPlatformMisc::RequestExitWithStatus(false,bPass?0:1);
        }
        void Pose()
        {
            const FVector Directions[]={FVector(-1,-1,.8),FVector(-1,1,.8),FVector(1,1,.8),FVector(1,-1,.8)};
            const double Distance=bCharacter?FMath::Max3(Extent.X,Extent.Y,Extent.Z)*3.0:FMath::Max(Extent.X,Extent.Y)*2.2;
            const FVector Location=Center+Directions[Shot]*Distance;
            Camera->SetActorLocationAndRotation(Location,(Center-Location).Rotation());
            ChangedAt=World->GetTimeSeconds(); bRequested=false;
        }
        void Tick()
        {
            if (bDone || !World.IsValid()) { return; }
            if (FPlatformTime::Seconds()-Started>120) { Finish(false); return; }
            APlayerController* Player=UGameplayStatics::GetPlayerController(World.Get(),0);
            if (!Player || !Player->GetPawn()) { return; }
            if (!bConfigured)
            {
                for (TActorIterator<AMovieManager> It(World.Get());It;++It) { It->StopMovie(); }
                FString CharacterPath;
                if (FParse::Value(FCommandLine::Get(),TEXT("InspectCharacter="),CharacterPath))
                {
                    USkeletalMesh* Mesh=LoadObject<USkeletalMesh>(nullptr,*CharacterPath);
                    if (!Mesh) { Finish(false); return; }
                    ASkeletalMeshActor* Preview=World->SpawnActor<ASkeletalMeshActor>(FVector(0,-2400,0),FRotator::ZeroRotator);
                    Preview->GetSkeletalMeshComponent()->SetSkeletalMeshAsset(Mesh); Preview->SetActorEnableCollision(false);
                    Preview->GetActorBounds(false,Center,Extent); bCharacter=true;
                }
                else
                {
                    for (TActorIterator<AStaticMeshActor> It(World.Get());It;++It) { if (It->ActorHasTag(TEXT("BankShell"))) { Bank=*It; break; } }
                    if (!Bank.IsValid()) { Finish(false); return; }
                    Bank->GetActorBounds(false,Center,Extent);
                }
                Camera=World->SpawnActor<ACameraActor>(); Camera->GetCameraComponent()->SetFieldOfView(60);
                Player->SetViewTarget(Camera.Get()); Player->GetPawn()->SetActorHiddenInGame(true);
                bConfigured=true; Pose(); return;
            }
            if (!bRequested && World->GetTimeSeconds()-ChangedAt>2.0)
            {
                IFileManager::Get().MakeDirectory(*Directory,true);
                FScreenshotRequest::RequestScreenshot(Directory/FString::Printf(TEXT("%sView%d.png"),bCharacter?TEXT("Character"):TEXT("Bank"),Shot+1),true,false); bRequested=true; return;
            }
            if (bRequested && !FScreenshotRequest::IsScreenshotRequested())
            {
                if (++Shot>=4) { Finish(true); } else { Pose(); }
            }
        }
    };
    void Start(UWorld* World)
    {
        if (!World || !World->IsGameWorld() || !FParse::Param(FCommandLine::Get(),TEXT("BankInspect"))) { return; }
        auto Run=MakeShared<FRun>(); Run->World=World; Run->Directory=FPaths::ProjectSavedDir()/TEXT("BankIntegration/Survey");
        FString CharacterPath;
        if (FParse::Value(FCommandLine::Get(),TEXT("InspectCharacter="),CharacterPath)) { Run->Directory=FPaths::ProjectSavedDir()/TEXT("CharacterImport/Preview"); }
        World->GetTimerManager().SetTimer(Run->Timer,FTimerDelegate::CreateLambda([Run]() { Run->Tick(); }),.1f,true);
    }
    FAutoConsoleCommandWithWorld Command(TEXT("Bank.Inspect"),TEXT("Separate -BankInspect process only; saves four bank survey views and exits."),FConsoleCommandWithWorldDelegate::CreateStatic(&Start));
}
#endif
