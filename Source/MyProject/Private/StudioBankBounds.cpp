#include "StudioBankBounds.h"
#include "Components/SceneComponent.h"
#include "Components/BoxComponent.h"
AStudioBankBounds::AStudioBankBounds()
{
    PrimaryActorTick.bCanEverTick=false;SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("SafetyRoot")));
    for(int32 I=0;I<4;++I){auto* Box=CreateDefaultSubobject<UBoxComponent>(*FString::Printf(TEXT("Perimeter%d"),I));Box->SetupAttachment(RootComponent);Walls.Add(Box);}
    Floor=CreateDefaultSubobject<UBoxComponent>(TEXT("ContinuousSafetyFloor"));Floor->SetupAttachment(RootComponent);
    auto Setup=[](UBoxComponent* Box){Box->SetCollisionProfileName(TEXT("BlockAll"));Box->SetCollisionResponseToChannel(ECC_Camera,ECR_Ignore);Box->SetCollisionResponseToChannel(ECC_Visibility,ECR_Ignore);Box->SetHiddenInGame(true);Box->SetCanEverAffectNavigation(false);};
    for(UBoxComponent* Box:Walls){Setup(Box);}Setup(Floor);
}
void AStudioBankBounds::OnConstruction(const FTransform& Transform)
{
    Super::OnConstruction(Transform);const double X=FMath::Max(500.,HalfSize.X),Y=FMath::Max(500.,HalfSize.Y);
    for(int32 I=0;I<4;++I){const bool Side=I<2;const double Sign=I%2?1:-1;Walls[I]->SetBoxExtent(Side?FVector(20,Y+20,700):FVector(X+20,20,700));Walls[I]->SetRelativeLocation(Side?FVector(Sign*(X+20),0,650):FVector(0,Sign*(Y+20),650));}
    Floor->SetBoxExtent(FVector(X+20,Y+20,30));Floor->SetRelativeLocation(FVector(0,0,-30));
}
bool AStudioBankBounds::NeedsRecovery(FVector Location)const
{
    const FVector P=GetTransform().InverseTransformPosition(Location);
    return Location.ContainsNaN() || FMath::Abs(P.X)>HalfSize.X+40 || FMath::Abs(P.Y)>HalfSize.Y+40 || P.Z < -RecoveryDepth;
}
