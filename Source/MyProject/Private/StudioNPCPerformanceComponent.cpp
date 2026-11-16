#include "StudioNPCPerformanceComponent.h"
#include "ActingComponent.h"
#include "WardrobeComponent.h"
#include "Animation/AnimSequence.h"
#include "Animation/AnimSingleNodeInstance.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "Engine/SkeletalMesh.h"
#include "GameFramework/Character.h"
#include "UObject/ConstructorHelpers.h"
UStudioNPCPerformanceComponent::UStudioNPCPerformanceComponent()
{
    PrimaryComponentTick.bCanEverTick=false;
    static ConstructorHelpers::FObjectFinder<UAnimSequence> Idle(TEXT("/Game/OneManStudio/Characters/Base3/Customization/Animations/B3_MM_Idle"));
    static ConstructorHelpers::FObjectFinder<UAnimSequence> Walk(TEXT("/Game/OneManStudio/Characters/Base3/Customization/Animations/B3_MF_Unarmed_Walk_Fwd"));
    IdleAnimation=Idle.Object;WalkAnimation=Walk.Object;
}
FCosmeticSelection UStudioNPCPerformanceComponent::VaryFace(const FCosmeticSelection& Base,const TArray<FCosmeticDefinition>& Catalog,int32 Seed)
{
    FCosmeticSelection Result=Base;FRandomStream Random(Seed);
    for(ECosmeticCategory Category:{ECosmeticCategory::Eyes,ECosmeticCategory::Mouth,ECosmeticCategory::Eyebrows})
    {
        TArray<FName> Options;for(const auto& Item:Catalog){if(Item.Category==Category && !Item.Id.IsNone() && !Item.Id.ToString().EndsWith(TEXT(".None"))){Options.Add(Item.Id);}}
        Options.Sort(FNameLexicalLess());if(!Options.IsEmpty()){Result.SetId(Category,Options[Random.RandRange(0,Options.Num()-1)]);}
    }
    return Result;
}
void UStudioNPCPerformanceComponent::Initialize(ACharacter* Actor,int32 Seed,bool bMayPace)
{
    Character=Actor;VariationSeed=Seed;bPacing=bMayPace;Performance=FActingRecording();CurrentAmbient=nullptr;
    NextSample=PendingHold=LastLiveTime=0;GestureIndex=0;FRandomStream Random(Seed);NextGesture=2.5+Random.FRandRange(0,3);
    if(!Character){return;}InitialTransform=Character->GetActorTransform();
    if(auto* Wardrobe=Character->FindComponentByClass<UWardrobeComponent>()){Performance.Appearance=Wardrobe->CaptureAppearance();}
    Character->GetMesh()->SetComponentTickEnabled(true);
    Character->GetMesh()->VisibilityBasedAnimTickOption=EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
    ObserveBoundary(0);PresentAnimation(0,false,false);
}
void UStudioNPCPerformanceComponent::ObserveBoundary(double Time)
{
    if(Character){FActingRecording::AppendPose(Performance.Samples,Time,Character->GetActorTransform(),PendingHold,true,true);}
}
void UStudioNPCPerformanceComponent::UpdateLive(double Time,bool bRunning,bool bCalm)
{
    if(!Character){return;}
    if(bRunning)
    {
        const FVector Before=Character->GetActorLocation();FTransform Pose=InitialTransform;
        const double Phase=(static_cast<uint32>(VariationSeed)%101)*.061;
        const float Look=12.f*(FMath::Sin(Time*.55+Phase)-FMath::Sin(Phase));
        Pose.SetRotation(InitialTransform.GetRotation()*FRotator(0,Look,0).Quaternion());
        if(bPacing && bCalm && Time>2)
        {
            const double Offset=PaceDistance*(1-FMath::Cos((Time-2)*.7))*.5;
            const FVector Desired=InitialTransform.GetLocation()+InitialTransform.GetUnitAxis(EAxis::Y)*Offset;
            FHitResult Hit;FCollisionQueryParams Query(SCENE_QUERY_STAT(NPCPace),false,Character);
            const auto* Capsule=Character->GetCapsuleComponent();
            if(!GetWorld()->SweepSingleByObjectType(Hit,InitialTransform.GetLocation(),Desired,FQuat::Identity,FCollisionObjectQueryParams(ECC_WorldStatic),FCollisionShape::MakeCapsule(Capsule->GetScaledCapsuleRadius(),Capsule->GetScaledCapsuleHalfHeight()-.5f),Query))
            {Pose.SetLocation(FMath::VInterpTo(Before,Desired,FMath::Clamp(Time-LastLiveTime,0.,.15),3.f));}
        }
        // Story reactions hold their current place; only calm witnesses shuffle.
        if(!bCalm){Pose.SetLocation(Before);}
        Character->SetActorTransform(Pose,false,nullptr,ETeleportType::TeleportPhysics);
        LastLiveTime=Time;
        if(Time>=NextSample){FActingRecording::AppendPose(Performance.Samples,Time,Pose,PendingHold,true);NextSample=Time+.05;}
    }
    FTransform Earlier;const bool Moving=Performance.Evaluate(FMath::Max(0.,Time-.08),Earlier) && FVector::DistSquared(Earlier.GetLocation(),Character->GetActorLocation())>.01;
    PresentAnimation(Time,bRunning,Moving);
}
void UStudioNPCPerformanceComponent::BeginPlayback()
{
    if(!Character){return;}CurrentAmbient=nullptr;
    if(auto* Wardrobe=Character->FindComponentByClass<UWardrobeComponent>();Wardrobe && Performance.Appearance.bHasAppearance)
    {Wardrobe->ApplySnapshot(Performance.Appearance,Character->GetMesh(),nullptr,true);}
    FTransform Pose;if(Performance.Evaluate(0,Pose)){Character->SetActorTransform(Pose,false,nullptr,ETeleportType::TeleportPhysics);}
    PresentAnimation(0,false,false);
}
void UStudioNPCPerformanceComponent::UpdatePlayback(double Time,bool bRunning)
{
    if(!Character){return;}FTransform Pose,Previous;
    if(Performance.Evaluate(Time,Pose)){Character->SetActorTransform(Pose,false,nullptr,ETeleportType::TeleportPhysics);}
    const bool Moving=Performance.Evaluate(FMath::Max(0.,Time-.08),Previous) && FVector::DistSquared(Previous.GetLocation(),Character->GetActorLocation())>.01;
    PresentAnimation(Time,bRunning,Moving);
}
void UStudioNPCPerformanceComponent::PresentAnimation(double Time,bool bRunning,bool bMoving)
{
    auto* Mesh=Character?Character->GetMesh():nullptr;if(!Mesh || !Mesh->GetSkeletalMeshAsset()){return;}
    if(auto* Acting=Character->FindComponentByClass<UActingComponent>();Acting && !Acting->ActiveGestureId.IsNone()){CurrentAmbient=nullptr;return;}
    UAnimSequence* Clip=bMoving && WalkAnimation?WalkAnimation.Get():IdleAnimation.Get();
    if(!Clip || Clip->GetSkeleton()!=Mesh->GetSkeletalMeshAsset()->GetSkeleton()){return;}
    if(CurrentAmbient!=Clip || Mesh->GetAnimationMode()!=EAnimationMode::AnimationSingleNode || !Mesh->GetSingleNodeInstance() || Mesh->GetSingleNodeInstance()->GetAnimationAsset()!=Clip)
    {Mesh->SetAnimationMode(EAnimationMode::AnimationSingleNode);Mesh->PlayAnimation(Clip,true);CurrentAmbient=Clip;}
    if(auto* Single=Mesh->GetSingleNodeInstance())
    {
        Single->SetRootMotionMode(ERootMotionMode::IgnoreRootMotion);
        const double Rate=bMoving?.14:1.;const double Offset=(static_cast<uint32>(VariationSeed)%17)*.13;
        Single->SetPosition(FMath::Fmod((Time+Offset)*Rate,static_cast<double>(FMath::Max(.01f,Clip->GetPlayLength()))),false);Single->SetPlaying(false);
    }
    Mesh->bPauseAnims=!bRunning;
}
FName UStudioNPCPerformanceComponent::ConsumeAmbientGesture(double Time,bool bCalm)
{
    if(Time<NextGesture){return NAME_None;}NextGesture=Time+7+((static_cast<uint32>(VariationSeed)+GestureIndex++)%5);
    if(!bCalm){return NAME_None;}
    return bPacing && (static_cast<uint32>(VariationSeed)+GestureIndex)%3==0?FName(TEXT("Dance")):FName(TEXT("Point"));
}
