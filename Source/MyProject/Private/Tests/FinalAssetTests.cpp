#if WITH_DEV_AUTOMATION_TESTS
#include "StudioAudioProfile.h"
#include "StudioCharacterProfile.h"
#include "StudioCharacterAssetTools.h"
#include "ProductionArtCatalog.h"
#include "ActingComponent.h"
#include "Misc/AutomationTest.h"
#include "Sound/SoundWave.h"
#include "Engine/Texture2D.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFinalAudioBindingsTest,"OneManStudio.FinalAssets.SuppliedAudioAndSnapshot",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FFinalAudioBindingsTest::RunTest(const FString& Parameters)
{
    const auto* Profile=LoadObject<UStudioAudioProfile>(nullptr,TEXT("/Game/OneManStudio/Audio/DA_StudioAudio"));
    if(!TestNotNull(TEXT("Editable supplied audio profile"),Profile)){return false;}
    TSet<USoundBase*> Unique;
    const auto Add=[&](USoundBase* Sound){TestNotNull(TEXT("Configured audio reference"),Sound);if(Sound){Unique.Add(Sound);}};
    TestEqual(TEXT("Six HUD sounds"),Profile->Soundboard.Num(),6);TestEqual(TEXT("Five recorded footstep variations"),Profile->Footsteps.Num(),5);
    for(const auto& S:Profile->Soundboard){Add(S.Sound);TestNotNull(TEXT("Actor spatial attenuation"),S.Attenuation.Get());}
    for(const auto& S:Profile->Footsteps){Add(S.Sound);}
    for(USoundBase* S:Profile->FaintImpacts){Add(S);}for(USoundBase* S:Profile->MoneyPickups){Add(S);}
    Add(Profile->DonutBite);Add(Profile->Applause);
    for(USoundBase* S:Profile->AlternateStings){Add(S);}
    const auto& Flow=Profile->FlowAudio;
    for(USoundBase* S:{Flow.MenuMusic.Get(),Flow.WardrobeMusic.Get(),Flow.RecordingMusic.Get(),Flow.ResultsMusic.Get(),Flow.BankAmbience.Get(),Flow.CinemaAmbience.Get()})
    {Add(S);const auto* W=Cast<USoundWave>(S);TestTrue(TEXT("Global track loops and stays global"),W && W->bLooping);}
    Add(Flow.ClapperSound);Add(Flow.ButtonSound);for(const auto& Cue:Flow.Announcements){Add(Cue.Sound);}
    Add(Profile->PremiereAudio.PremiereSting);Add(Profile->PremiereAudio.ResultsSting);
    TestEqual(TEXT("All 34 supplied recordings represented"),Unique.Num(),34);
    auto* Acting=NewObject<UActingComponent>();Profile->ConfigureActing(Acting);
    const auto Snapshot=Acting->CapturePlaybackSettings();
    Acting->Sounds[0].Sound=nullptr;Acting->Footsteps.Reset();
    TestNotNull(TEXT("Recorded sound references are value copies"),Snapshot.Sounds[0].Sound.Get());
    TestEqual(TEXT("Recorded footsteps survive later live edits"),Snapshot.Footsteps.Num(),5);
    for(const auto& S:Snapshot.Sounds){TestFalse(TEXT("Music never enters an actor recording"),S.Sound==Flow.RecordingMusic || S.Sound==Flow.WardrobeMusic);}
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFinalAttachmentConfigTest,"OneManStudio.FinalAssets.ReferenceAttachmentsAndSkinMasks",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FFinalAttachmentConfigTest::RunTest(const FString& Parameters)
{
    const auto* Catalog=LoadObject<UProductionArtCatalog>(nullptr,TEXT("/Game/OneManStudio/Cosmetics/ProductionArt/DA_ProductionCosmetics"));
    const auto* Profile=LoadObject<UStudioCharacterProfile>(nullptr,TEXT("/Game/OneManStudio/Characters/Base3/Customization/DA_Base3Character"));
    if(!TestNotNull(TEXT("Production catalog"),Catalog)||!TestNotNull(TEXT("Existing Base3 rig"),Profile)){return false;}
    TSet<UTexture2D*> Masks;bool MaskAlias=false;
    for(const auto& Item:Catalog->Cosmetics)
    {
        if(Item.Category==ECosmeticCategory::Body){TestNotNull(TEXT("Individual UV0 coverage mask"),Item.ClothingMask.Get());Masks.Add(Item.ClothingMask);}
        else if(Item.StaticMesh)
        {
            TestFalse(TEXT("Reference fit never resolves from a live animated pose"),Item.bFitInActorSpace);
            TestTrue(TEXT("Imported bone scale already resolved once"),Item.bSocketScaleResolved);
            const FTransform Parent=UStudioCharacterAssetTools::ReferenceBoneTransform(Profile->BodyMesh,Item.SocketName,Profile->MeshTransform);
            TestTrue(TEXT("Preview/live/replay share the exact reference-space fit"),(Item.RelativeTransform*Parent).Equals(Item.ActorRelativeTransform,.02f));
        }
        if(Item.Id==TEXT("Face.RobberMask")){MaskAlias=Item.SocketName==TEXT("Head") && Item.StaticMesh!=nullptr;}
    }
    TestEqual(TEXT("Six distinct outfit masks"),Masks.Num(),6);TestTrue(TEXT("Mask can coexist with a hat"),MaskAlias);
    return true;
}
#endif
