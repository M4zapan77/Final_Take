#include "StudioCharacterAssetTools.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMesh.h"
#include "StudioAnimInstance.h"
#if WITH_EDITOR
#include "AnimGraphNode_BlendListByBool.h"
#include "AnimGraphNode_Root.h"
#include "AnimGraphNode_SequencePlayer.h"
#include "AnimGraphNode_Slot.h"
#include "Animation/AnimBlueprint.h"
#include "Animation/AnimData/IAnimationDataController.h"
#include "Animation/AnimSequence.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Dom/JsonObject.h"
#include "EdGraph/EdGraph.h"
#include "EdGraph/EdGraphSchema.h"
#include "Factories/AnimBlueprintFactory.h"
#include "K2Node_VariableGet.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "MaterialEditingLibrary.h"
#include "ObjectTools.h"
#include "Misc/ObjectThumbnail.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "ImageUtils.h"
#include "ShaderCompiler.h"
#include "AssetCompilingManager.h"
#include "ThumbnailRendering/SceneThumbnailInfo.h"
#include "Materials/Material.h"
#include "Materials/MaterialExpression.h"
#include "MeshDescription.h"
#include "Misc/PackageName.h"
#include "RetargetEditor/IKRetargetBatchOperation.h"
#include "Serialization/JsonSerializer.h"
#include "StaticMeshAttributes.h"
#include "UObject/Package.h"
#endif

bool UStudioCharacterAssetTools::ExportArtThumbnail(UObject* Asset,const FString& Filename,float OrbitYaw)
{
#if WITH_EDITOR
    FString Output=FPaths::ConvertRelativePathToFull(Filename),Allowed=FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir());
    FPaths::NormalizeFilename(Output);FPaths::NormalizeFilename(Allowed);
    if(!Asset||!Output.StartsWith(Allowed)||(!Asset->GetPathName().Contains(TEXT("/ProductionArt/"))&&Asset->GetOutermost()!=GetTransientPackage())){return false;}
    USceneThumbnailInfo* Info=NewObject<USceneThumbnailInfo>(Asset);Info->OrbitYaw=OrbitYaw;Info->OrbitPitch=-15;
    if(UStaticMesh* Mesh=Cast<UStaticMesh>(Asset)){Mesh->ThumbnailInfo=Info;}
    else if(USkeletalMesh* Skeletal=Cast<USkeletalMesh>(Asset)){Skeletal->SetThumbnailInfo(Info);}
    FAssetCompilingManager::Get().FinishAllCompilation();
    if(GShaderCompilingManager){GShaderCompilingManager->FinishAllCompilation();}
    FObjectThumbnail Thumbnail;ThumbnailTools::RenderThumbnail(Asset,512,512,ThumbnailTools::EThumbnailTextureFlushMode::AlwaysFlush,nullptr,&Thumbnail);
    const auto& Raw=Thumbnail.GetUncompressedImageData();const int32 Width=Thumbnail.GetImageWidth(),Height=Thumbnail.GetImageHeight();
    if(Width<=0||Height<=0||Raw.Num()!=Width*Height*4){return false;}
    TArray<FColor> Pixels;Pixels.SetNumUninitialized(Width*Height);FMemory::Memcpy(Pixels.GetData(),Raw.GetData(),Raw.Num());
    TArray64<uint8> PNG;FImageUtils::PNGCompressImageArray(Width,Height,Pixels,PNG);return FFileHelper::SaveArrayToFile(PNG,*Output);
#else
    return false;
#endif
}

bool UStudioCharacterAssetTools::NormalizeArtMesh(UStaticMesh* Mesh,float LongestDimension,int32 MaximumTriangles)
{
#if WITH_EDITOR
    if(!Mesh||!Mesh->GetPathName().Contains(TEXT("/ProductionArt/"))||!FMath::IsFinite(LongestDimension)||LongestDimension<=0){return false;}
    FMeshDescription* Description=Mesh->GetMeshDescription(0);if(!Description){return false;}
    FStaticMeshAttributes Attributes(*Description);auto Positions=Attributes.GetVertexPositions();FBox Box(ForceInit);
    for(FVertexID V:Description->Vertices().GetElementIDs()){Box+=FVector(Positions[V]);}
    if(!Box.IsValid||Box.GetSize().GetMax()<SMALL_NUMBER){return false;}
    Mesh->Modify();const float Scale=LongestDimension/Box.GetSize().GetMax();const FVector Center=Box.GetCenter();
    for(FVertexID V:Description->Vertices().GetElementIDs()){Positions[V]=FVector3f((FVector(Positions[V])-Center)*Scale);}
    Mesh->CommitMeshDescription(0);
    Mesh->GetSourceModel(0).ReductionSettings.PercentTriangles=FMath::Min(1.f,FMath::Max(1000,MaximumTriangles)/static_cast<float>(Description->Triangles().Num()));
    Mesh->Build(false);Mesh->PostEditChange();return true;
#else
    return false;
#endif
}

bool UStudioCharacterAssetTools::ExportOutfitThumbnail(USkeletalMesh* Mesh,UMaterialInterface* Material,const FString& Filename)
{
#if WITH_EDITOR
    if(!Mesh||!Material){return false;}
    USkeletalMesh* Preview=DuplicateObject<USkeletalMesh>(Mesh,GetTransientPackage());
    if(Preview->GetMaterials().IsEmpty()){Preview->GetMaterials().Add(FSkeletalMaterial(Material));}
    else{Preview->GetMaterials()[0].MaterialInterface=Material;}
    return ExportArtThumbnail(Preview,Filename,135.f);
#else
    return false;
#endif
}
FTransform UStudioCharacterAssetTools::ReferenceBoneTransform(USkeletalMesh* Mesh,FName Bone,const FTransform& MeshToActor)
{
    if(!Mesh){return FTransform::Identity;}const auto& Ref=Mesh->GetRefSkeleton();const int32 BoneIndex=Ref.FindBoneIndex(Bone);
    if(BoneIndex<0){return FTransform::Identity;}FTransform Pose=Ref.GetRefBonePose()[BoneIndex];
    for(int32 Parent=Ref.GetParentIndex(BoneIndex);Parent>=0;Parent=Ref.GetParentIndex(Parent)){Pose=Pose*Ref.GetRefBonePose()[Parent];}
    return Pose*MeshToActor;
}

bool UStudioCharacterAssetTools::ResetCustomizationMaterial(UMaterial* Material)
{
#if WITH_EDITOR
    if (!Material ||
        (!Material->GetPathName().StartsWith(TEXT("/Game/OneManStudio/Characters/Base3/Customization/Materials/")) &&
         !Material->GetPathName().StartsWith(TEXT("/Game/OneManStudio/Cosmetics/ProductionArt/Body/")) &&
         !Material->GetPathName().StartsWith(TEXT("/Game/OneManStudio/Cosmetics/ProductionArt/Corrections/Outfits/"))))
    {
        return false;
    }
    Material->Modify();
    for (UMaterialExpression* Expression : Material->GetExpressions())
    {
        if (Expression && Expression->IsRooted())
        {
            Expression->RemoveFromRoot();
        }
    }
    UMaterialEditingLibrary::DeleteAllMaterialExpressions(Material);
    return true;
#else
    return false;
#endif
}

FString UStudioCharacterAssetTools::DescribeMesh(USkeletalMesh* Mesh)
{
#if WITH_EDITOR
    if (!Mesh)
    {
        return TEXT("{}");
    }
    auto Root = MakeShared<FJsonObject>();
    const FReferenceSkeleton& Ref = Mesh->GetRefSkeleton();
    TArray<FTransform> Global;
    TArray<TSharedPtr<FJsonValue>> Bones;
    for (int32 I = 0; I < Ref.GetNum(); ++I)
    {
        const int32 Parent = Ref.GetParentIndex(I);
        Global.Add(Parent >= 0 ? Ref.GetRefBonePose()[I] * Global[Parent] : Ref.GetRefBonePose()[I]);
        auto B = MakeShared<FJsonObject>();
        B->SetStringField(TEXT("name"), Ref.GetBoneName(I).ToString());
        B->SetNumberField(TEXT("parent"), Parent);
        const FVector P = Global[I].GetTranslation();
        B->SetArrayField(TEXT("position"), {MakeShared<FJsonValueNumber>(P.X), MakeShared<FJsonValueNumber>(P.Y),
                                            MakeShared<FJsonValueNumber>(P.Z)});
        Bones.Add(MakeShared<FJsonValueObject>(B));
    }
    Root->SetArrayField(TEXT("bones"), Bones);
    if (const FMeshDescription* D = Mesh->GetMeshDescription(0))
    {
        FStaticMeshConstAttributes Attributes(*D);
        auto Positions = Attributes.GetVertexPositions();
        auto UVs = Attributes.GetVertexInstanceUVs();
        TArray<TSharedPtr<FJsonValue>> Vertices, Triangles, UV;
        for (const FVertexID V : D->Vertices().GetElementIDs())
        {
            const FVector3f P = Positions[V];
            Vertices.Add(MakeShared<FJsonValueArray>(TArray<TSharedPtr<FJsonValue>>{
                MakeShared<FJsonValueNumber>(V.GetValue()), MakeShared<FJsonValueNumber>(P.X),
                MakeShared<FJsonValueNumber>(P.Y), MakeShared<FJsonValueNumber>(P.Z)}));
        }
        for (const FTriangleID T : D->Triangles().GetElementIDs())
        {
            TArray<TSharedPtr<FJsonValue>> Indices, Tex;
            for (const FVertexInstanceID VI : D->GetTriangleVertexInstances(T))
            {
                Indices.Add(MakeShared<FJsonValueNumber>(D->GetVertexInstanceVertex(VI).GetValue()));
                const FVector2f U = UVs.Get(VI, 0);
                Tex.Add(MakeShared<FJsonValueArray>(TArray<TSharedPtr<FJsonValue>>{MakeShared<FJsonValueNumber>(U.X),
                                                                                   MakeShared<FJsonValueNumber>(U.Y)}));
            }
            Triangles.Add(MakeShared<FJsonValueArray>(Indices));
            UV.Add(MakeShared<FJsonValueArray>(Tex));
        }
        Root->SetArrayField(TEXT("vertices"), Vertices);
        Root->SetArrayField(TEXT("triangles"), Triangles);
        Root->SetArrayField(TEXT("triangle_uv0"), UV);
        Root->SetNumberField(TEXT("uv_channels"), UVs.GetNumChannels());
    }
    FString Json;
    FJsonSerializer::Serialize(Root, TJsonWriterFactory<>::Create(&Json));
    return Json;
#else
    return TEXT("{}");
#endif
}

UObject* UStudioCharacterAssetTools::CreateLocomotionBlueprint(USkeletalMesh* Mesh, UAnimSequence* Idle,
                                                               UAnimSequence* Walk, UAnimSequence* Run,
                                                               UAnimSequence* Jump, UAnimSequence* Fall,
                                                               const FString& PackagePath)
{
#if WITH_EDITOR
    if (!Mesh || !Idle || !Walk || !Run || !Jump || !Fall ||
        !PackagePath.StartsWith(TEXT("/Game/OneManStudio/Characters/Base3/Customization/")) ||
        FPackageName::DoesPackageExist(PackagePath))
    {
        return nullptr;
    }
    UPackage* Package = CreatePackage(*PackagePath);
    auto* Factory = NewObject<UAnimBlueprintFactory>();
    Factory->ParentClass = UStudioAnimInstance::StaticClass();
    Factory->TargetSkeleton = Mesh->GetSkeleton();
    Factory->PreviewSkeletalMesh = Mesh;
    UAnimBlueprint* Blueprint = Cast<UAnimBlueprint>(Factory->FactoryCreateNew(
        UAnimBlueprint::StaticClass(), Package, *FPackageName::GetLongPackageAssetName(PackagePath),
        RF_Public | RF_Standalone, nullptr, GWarn));
    if (!Blueprint)
    {
        return nullptr;
    }
    UEdGraph* Graph = nullptr;
    TArray<UEdGraph*> Graphs;
    Blueprint->GetAllGraphs(Graphs);
    for (UEdGraph* Candidate : Graphs)
    {
        if (Candidate->GetFName() == TEXT("AnimGraph"))
        {
            Graph = Candidate;
            break;
        }
    }
    if (!Graph)
    {
        return nullptr;
    }
    UAnimGraphNode_Root* Root = nullptr;
    for (UEdGraphNode* N : Graph->Nodes)
    {
        if (auto* R = Cast<UAnimGraphNode_Root>(N))
        {
            Root = R;
            break;
        }
    }
    if (!Root)
    {
        return nullptr;
    }
    const auto Add = [Graph](UEdGraphNode* Node)
    {
        Graph->AddNode(Node, false, false);
        Node->CreateNewGuid();
        Node->PostPlacedNewNode();
        Node->AllocateDefaultPins();
    };
    int32 Y = 0;
    const auto Player = [&](UAnimSequence* Sequence, bool Loop)
    {
        auto* Node = NewObject<UAnimGraphNode_SequencePlayer>(Graph);
        Node->Node.SetSequence(Sequence);
        Node->Node.SetLoopAnimation(Loop);
        Add(Node);
        Node->NodePosX = -1000;
        Node->NodePosY = Y;
        Y += 170;
        return Node;
    };
    const auto Connect = [Graph](UEdGraphNode* From, const TCHAR* Output, UEdGraphNode* To, const TCHAR* Input)
    {
        UEdGraphPin* A = From->FindPin(Output, EGPD_Output);
        UEdGraphPin* B = To->FindPin(Input, EGPD_Input);
        return A && B && Graph->GetSchema()->TryCreateConnection(A, B);
    };
    const auto Blend = [&](const TCHAR* Property, UEdGraphNode* TruePose, UEdGraphNode* FalsePose, int32 X)
    {
        auto* B = NewObject<UAnimGraphNode_BlendListByBool>(Graph);
        Add(B);
        B->NodePosX = X;
        B->NodePosY = Y;
        Y += 100;
        auto* Getter = NewObject<UK2Node_VariableGet>(Graph);
        Getter->VariableReference.SetSelfMember(Property);
        Add(Getter);
        Getter->NodePosX = X - 200;
        Getter->NodePosY = B->NodePosY + 100;
        if (!Connect(TruePose, TEXT("Pose"), B, TEXT("BlendPose_0")) ||
            !Connect(FalsePose, TEXT("Pose"), B, TEXT("BlendPose_1")) ||
            !Connect(Getter, Property, B, TEXT("bActiveValue")))
        {
            UE_LOG(LogTemp, Error, TEXT("Could not connect locomotion graph property %s"), Property);
        }
        return B;
    };
    auto* I = Player(Idle, true);
    auto* W = Player(Walk, true);
    auto* R = Player(Run, true);
    auto* J = Player(Jump, false);
    auto* F = Player(Fall, true);
    auto* Moving = Blend(TEXT("bIsMoving"), W, I, -700);
    auto* Running = Blend(TEXT("bIsRunning"), R, Moving, -450);
    auto* Descending = Blend(TEXT("bIsDescending"), F, J, -450);
    auto* Air = Blend(TEXT("bIsInAir"), Descending, Running, -200);
    auto* Slot = NewObject<UAnimGraphNode_Slot>(Graph);
    Slot->Node.SlotName = TEXT("DefaultSlot");
    Add(Slot);
    Slot->NodePosX = 100;
    Root->NodePosX = 400;
    Connect(Air, TEXT("Pose"), Slot, TEXT("Source"));
    Connect(Slot, TEXT("Pose"), Root, TEXT("Result"));
    FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);
    FKismetEditorUtilities::CompileBlueprint(Blueprint);
    if (Blueprint->Status == BS_Error)
    {
        return nullptr;
    }
    FAssetRegistryModule::AssetCreated(Blueprint);
    Package->MarkPackageDirty();
    return Blueprint;
#else
    return nullptr;
#endif
}

UAnimSequence* UStudioCharacterAssetTools::CreatePlaceholderGesture(USkeletalMesh* Mesh, UAnimSequence* Template,
                                                                    FName Gesture, const FString& PackagePath)
{
#if WITH_EDITOR
    if (!Mesh || !Template || !PackagePath.StartsWith(TEXT("/Game/OneManStudio/Characters/Base3/Customization/")) ||
        FPackageName::DoesPackageExist(PackagePath))
    {
        return nullptr;
    }
    UPackage* Package = CreatePackage(*PackagePath);
    auto* Sequence = NewObject<UAnimSequence>(Package, *FPackageName::GetLongPackageAssetName(PackagePath),
                                              RF_Public | RF_Standalone);
    Sequence->SetFlags(RF_Public | RF_Standalone);
    Sequence->SetSkeleton(Mesh->GetSkeleton());
    Sequence->Notifies.Reset();
    UE_LOG(LogTemp, Display, TEXT("Creating in-place gesture %s"), *Gesture.ToString());
    IAnimationDataController& C = Sequence->GetController();
    C.InitializeModel();
    C.OpenBracket(FText::FromString(TEXT("Create editable in-place placeholder acting clip")), false);
    C.SetFrameRate(FFrameRate(30, 1), false);
    C.SetNumberOfFrames(FFrameNumber(60), false);
    C.RemoveAllBoneTracks(false);
    const FReferenceSkeleton& Ref = Mesh->GetRefSkeleton();
    TArray<FTransform> Global;
    for (int32 I = 0; I < Ref.GetNum(); ++I)
    {
        const int32 Parent = Ref.GetParentIndex(I);
        Global.Add(Parent >= 0 ? Ref.GetRefBonePose()[I] * Global[Parent] : Ref.GetRefBonePose()[I]);
    }
    for (int32 I = 0; I < Ref.GetNum(); ++I)
    {
        const FName Name = Ref.GetBoneName(I);
        const FString Bone = Name.ToString();
        const FTransform Base = Ref.GetRefBonePose()[I];
        TArray<FVector3f> Positions, Scales;
        TArray<FQuat4f> Rotations;
        for (int32 Frame = 0; Frame <= 60; ++Frame)
        {
            const double Time = Frame / 30.0, Wave = FMath::Sin(Time * 2 * PI),
                         Blend = FMath::Clamp(Time / .25, 0.0, 1.0) * FMath::Clamp((2 - Time) / .25, 0.0, 1.0);
            FQuat Q = Base.GetRotation();
            const auto RotateWorld = [&](FVector Axis, double Angle)
            { Q = Q * FQuat(Global[I].GetRotation().UnrotateVector(Axis), Angle); };
            if (Gesture == TEXT("Dance"))
            {
                if (Bone == TEXT("LeftArm"))
                {
                    RotateWorld(FVector(0, -1, 0), .45 + Wave * .45);
                }
                else if (Bone == TEXT("RightArm"))
                {
                    RotateWorld(FVector(0, -1, 0), -.45 + Wave * .45);
                }
                else if (Bone == TEXT("Spine1"))
                {
                    RotateWorld(FVector(0, -1, 0), Wave * .12);
                }
                else if (Bone == TEXT("Head"))
                {
                    RotateWorld(FVector::UpVector, Wave * .16);
                }
                else if (Bone == TEXT("LeftUpLeg"))
                {
                    RotateWorld(FVector::ForwardVector, Wave * .10);
                }
                else if (Bone == TEXT("RightUpLeg"))
                {
                    RotateWorld(FVector::ForwardVector, -Wave * .10);
                }
            }
            else if ((Bone == TEXT("RightArm") &&
                      (Gesture == TEXT("Point") || Gesture == TEXT("Arrest") || Gesture == TEXT("Scared"))) ||
                     (Bone == TEXT("LeftArm") && (Gesture == TEXT("Scared") || Gesture == TEXT("Arrest"))))
            {
                const int32 Child =
                    Ref.FindBoneIndex(Bone == TEXT("RightArm") ? TEXT("RightForeArm") : TEXT("LeftForeArm"));
                const FVector Current = (Global[Child].GetLocation() - Global[I].GetLocation()).GetSafeNormal();
                const FVector Target = Gesture == TEXT("Scared")
                                           ? FVector(Bone == TEXT("LeftArm") ? .25 : -.25, .35, .9).GetSafeNormal()
                                           : FVector(Bone == TEXT("LeftArm") ? .2 : -.2, 1, .12).GetSafeNormal();
                const FQuat DesiredWorld = FQuat::FindBetweenNormals(Current, Target) * Global[I].GetRotation();
                const int32 Parent = Ref.GetParentIndex(I);
                const FQuat DesiredLocal =
                    Parent >= 0 ? Global[Parent].GetRotation().Inverse() * DesiredWorld : DesiredWorld;
                Q = FQuat::Slerp(Q, DesiredLocal, Blend);
            }
            else if (Gesture == TEXT("Scared") && Bone == TEXT("Head"))
            {
                RotateWorld(FVector(0, -1, 0), FMath::Sin(Time * 25) * .06 * Blend);
            }
            Q.Normalize();
            Positions.Add(FVector3f(Base.GetTranslation()));
            Scales.Add(FVector3f(Base.GetScale3D()));
            Rotations.Add(FQuat4f(Q));
        }
        C.AddBoneCurve(Name, false);
        C.SetBoneTrackKeys(Name, Positions, Rotations, Scales, false);
    }
    C.NotifyPopulated();
    C.CloseBracket(false);
    Sequence->bEnableRootMotion = false;
    Sequence->bForceRootLock = true;
    Sequence->PostEditChange();
    FAssetRegistryModule::AssetCreated(Sequence);
    Package->MarkPackageDirty();
    UE_LOG(LogTemp, Display, TEXT("Completed in-place gesture %s"), *Gesture.ToString());
    return Sequence;
#else
    return nullptr;
#endif
}

TArray<UObject*> UStudioCharacterAssetTools::RetargetAnimations(const TArray<UObject*>& Assets, USkeletalMesh* Source,
                                                                USkeletalMesh* Target, UObject* Retargeter,
                                                                const FString& Destination)
{
    TArray<UObject*> Result;
#if WITH_EDITOR
    if (!Source || !Target || !Cast<UIKRetargeter>(Retargeter) ||
        !Destination.StartsWith(TEXT("/Game/OneManStudio/Characters/Base3/Customization/")))
    {
        return Result;
    }
    FIKRetargetBatchOperationContext Context;
    Context.SourceMesh = Source;
    Context.TargetMesh = Target;
    Context.IKRetargetAsset = Cast<UIKRetargeter>(Retargeter);
    Context.bIncludeReferencedAssets = false;
    Context.NameRule.FolderPath = Destination;
    Context.NameRule.Prefix = TEXT("B3_");
    Context.bExportOnlyAnimatedBones = false;
    for (UObject* Asset : Assets)
    {
        if (!Asset || FPackageName::DoesPackageExist(Destination / TEXT("B3_") + Asset->GetName()))
        {
            return {};
        }
        Context.AssetsToRetarget.Add(Asset);
    }
    NewObject<UIKRetargetBatchOperation>()->RunRetarget(Context);
    for (UObject* Asset : Assets)
    {
        if (UObject* New = LoadObject<UObject>(nullptr, *(Destination / (TEXT("B3_") + Asset->GetName()))))
        {
            Result.Add(New);
        }
    }
#endif
    return Result;
}

UStaticMesh* UStudioCharacterAssetTools::BuildFaceSurface(USkeletalMesh* Mesh, const FString& PackagePath)
{
#if WITH_EDITOR
    if (!Mesh || !PackagePath.StartsWith(TEXT("/Game/OneManStudio/Characters/Base3/Customization/")) ||
        FPackageName::DoesPackageExist(PackagePath))
    {
        return nullptr;
    }
    const FMeshDescription* Source = Mesh->GetMeshDescription(0);
    if (!Source)
    {
        return nullptr;
    }
    const FReferenceSkeleton& Ref = Mesh->GetRefSkeleton();
    const int32 Head = Ref.FindBoneIndex(TEXT("Head")), Neck = Ref.FindBoneIndex(TEXT("Neck"));
    if (Head < 0 || Neck < 0)
    {
        return nullptr;
    }
    TArray<FTransform> Global;
    for (int32 I = 0; I < Ref.GetNum(); ++I)
    {
        const int32 P = Ref.GetParentIndex(I);
        Global.Add(P >= 0 ? Ref.GetRefBonePose()[I] * Global[P] : Ref.GetRefBonePose()[I]);
    }
    FStaticMeshConstAttributes SourceAttributes(*Source);
    auto Positions = SourceAttributes.GetVertexPositions();
    auto SourceNormals = SourceAttributes.GetVertexInstanceNormals();
    const double Bottom = Global[Neck].GetLocation().Z + 2;
    FBox HeadBounds(ForceInit);
    for (FVertexID V : Source->Vertices().GetElementIDs())
    {
        if (Positions[V].Z > Bottom)
        {
            HeadBounds += FVector(Positions[V]);
        }
    }
    if (!HeadBounds.IsValid)
    {
        return nullptr;
    }
    FMeshDescription Face;
    FStaticMeshAttributes Attributes(Face);
    Attributes.Register();
    auto P = Attributes.GetVertexPositions();
    auto N = Attributes.GetVertexInstanceNormals();
    auto Tangents = Attributes.GetVertexInstanceTangents();
    auto Signs = Attributes.GetVertexInstanceBinormalSigns();
    auto UV = Attributes.GetVertexInstanceUVs();
    UV.SetNumChannels(1);
    const FPolygonGroupID Group = Face.CreatePolygonGroup();
    int32 Count = 0;
    for (FTriangleID T : Source->Triangles().GetElementIDs())
    {
        const TArrayView<const FVertexInstanceID> Corners = Source->GetTriangleVertexInstances(T);
        FVector V[3];
        for (int32 I = 0; I < 3; ++I)
        {
            V[I] = FVector(Positions[Source->GetVertexInstanceVertex(Corners[I])]);
        }
        const FVector Normal = (FVector(SourceNormals[Corners[0]]) + FVector(SourceNormals[Corners[1]]) +
                                FVector(SourceNormals[Corners[2]]))
                                   .GetSafeNormal();
        if (V[0].Z < Bottom || V[1].Z < Bottom || V[2].Z < Bottom ||
            (V[0].Y + V[1].Y + V[2].Y) / 3 < HeadBounds.GetCenter().Y || Normal.Y < .05)
        {
            continue;
        }
        TArray<FVertexInstanceID> Output;
        for (int32 I = 0; I < 3; ++I)
        {
            const FVector VertexNormal = FVector(SourceNormals[Corners[I]]).GetSafeNormal();
            const FVertexID Vertex = Face.CreateVertex();
            P[Vertex] = FVector3f(Global[Head].InverseTransformPosition(V[I] + VertexNormal * .035));
            const FVertexInstanceID Instance = Face.CreateVertexInstance(Vertex);
            Output.Add(Instance);
            N[Instance] = FVector3f(Global[Head].InverseTransformVectorNoScale(Normal));
            Tangents[Instance] = FVector3f(Global[Head].InverseTransformVectorNoScale(FVector(1, 0, 0)));
            Signs[Instance] = 1;
            UV.Set(Instance, 0,
                   FVector2f((V[I].X - HeadBounds.Min.X) / HeadBounds.GetSize().X,
                             (HeadBounds.Max.Z - V[I].Z) / HeadBounds.GetSize().Z));
        }
        Face.CreateTriangle(Group, Output);
        ++Count;
    }
    if (Count < 10)
    {
        return nullptr;
    }
    UPackage* Package = CreatePackage(*PackagePath);
    UStaticMesh* Asset =
        NewObject<UStaticMesh>(Package, *FPackageName::GetLongPackageAssetName(PackagePath), RF_Public | RF_Standalone);
    Asset->GetStaticMaterials().Add(FStaticMaterial());
    UStaticMesh::FBuildMeshDescriptionsParams Params;
    Params.bBuildSimpleCollision = false;
    Params.bFastBuild = false;
    Asset->BuildFromMeshDescriptions({&Face}, Params);
    FAssetRegistryModule::AssetCreated(Asset);
    Package->MarkPackageDirty();
    return Asset;
#else
    return nullptr;
#endif
}
