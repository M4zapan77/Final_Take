#include "StudioAssetCorrectionTools.h"
#include "Components/PrimitiveComponent.h"

FVector UStudioAssetCorrectionTools::TraceSurface(UPrimitiveComponent* Surface,FVector Start,FVector End)
{
    FHitResult Hit; FCollisionQueryParams Params(SCENE_QUERY_STAT(StudioCorrectionFloor),true);
    return Surface && Surface->LineTraceComponent(Hit,Start,End,Params) && Hit.ImpactNormal.Z>.75f?Hit.ImpactPoint:FVector(0,0,-100000);
}
#include "Engine/StaticMesh.h"
#if WITH_EDITOR
#include "AssetCompilingManager.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Dom/JsonObject.h"
#include "MeshDescription.h"
#include "Misc/PackageName.h"
#include "Serialization/JsonSerializer.h"
#include "StaticMeshAttributes.h"
#include "StaticMeshResources.h"
#include "UObject/Package.h"
#endif

FString UStudioAssetCorrectionTools::DescribeStaticMesh(UStaticMesh* Mesh)
{
#if WITH_EDITOR
    if(!Mesh){return TEXT("{}");}
    FAssetCompilingManager::Get().FinishAllCompilation();
    if(!Mesh->GetRenderData() || Mesh->GetRenderData()->LODResources.IsEmpty()){return TEXT("{}");}
    const auto& LOD=Mesh->GetRenderData()->LODResources[0];
    auto Root=MakeShared<FJsonObject>();
    TArray<TSharedPtr<FJsonValue>> Vertices,Normals,UVs,Triangles;
    for(int32 I=0;I<LOD.GetNumVertices();++I)
    {
        const FVector3f P=LOD.VertexBuffers.PositionVertexBuffer.VertexPosition(I);
        const FVector3f N=LOD.VertexBuffers.StaticMeshVertexBuffer.VertexTangentZ(I);
        const FVector2f UV=LOD.VertexBuffers.StaticMeshVertexBuffer.GetVertexUV(I,0);
        Vertices.Add(MakeShared<FJsonValueArray>(TArray<TSharedPtr<FJsonValue>>{MakeShared<FJsonValueNumber>(P.X),MakeShared<FJsonValueNumber>(P.Y),MakeShared<FJsonValueNumber>(P.Z)}));
        Normals.Add(MakeShared<FJsonValueArray>(TArray<TSharedPtr<FJsonValue>>{MakeShared<FJsonValueNumber>(N.X),MakeShared<FJsonValueNumber>(N.Y),MakeShared<FJsonValueNumber>(N.Z)}));
        UVs.Add(MakeShared<FJsonValueArray>(TArray<TSharedPtr<FJsonValue>>{MakeShared<FJsonValueNumber>(UV.X),MakeShared<FJsonValueNumber>(UV.Y)}));
    }
    for(int32 I=0;I<LOD.IndexBuffer.GetNumIndices();I+=3)
    {
        Triangles.Add(MakeShared<FJsonValueArray>(TArray<TSharedPtr<FJsonValue>>{MakeShared<FJsonValueNumber>(LOD.IndexBuffer.GetIndex(I)),MakeShared<FJsonValueNumber>(LOD.IndexBuffer.GetIndex(I+1)),MakeShared<FJsonValueNumber>(LOD.IndexBuffer.GetIndex(I+2))}));
    }
    Root->SetArrayField(TEXT("vertices"),Vertices);Root->SetArrayField(TEXT("normals"),Normals);
    Root->SetArrayField(TEXT("uv"),UVs);Root->SetArrayField(TEXT("triangles"),Triangles);
    FString Result;FJsonSerializer::Serialize(Root,TJsonWriterFactory<>::Create(&Result));return Result;
#else
    return TEXT("{}");
#endif
}

UStaticMesh* UStudioAssetCorrectionTools::CreateCorrectedMesh(UStaticMesh* Source,const TArray<FVector>& Positions,const FString& PackagePath)
{
#if WITH_EDITOR
    if(!Source || !PackagePath.StartsWith(TEXT("/Game/OneManStudio/Cosmetics/ProductionArt/Corrections/")) || FPackageName::DoesPackageExist(PackagePath)){return nullptr;}
    FAssetCompilingManager::Get().FinishAllCompilation();
    if(!Source->GetRenderData() || Source->GetRenderData()->LODResources.IsEmpty()){return nullptr;}
    const auto& LOD=Source->GetRenderData()->LODResources[0];
    if(Positions.Num()!=LOD.GetNumVertices()){return nullptr;}
    FMeshDescription D;FStaticMeshAttributes A(D);A.Register();
    auto P=A.GetVertexPositions();auto N=A.GetVertexInstanceNormals();auto T=A.GetVertexInstanceTangents();
    auto S=A.GetVertexInstanceBinormalSigns();auto UV=A.GetVertexInstanceUVs();UV.SetNumChannels(1);
    const auto Group=D.CreatePolygonGroup();TArray<FVertexID> V;
    for(const FVector& Position:Positions)
    {
        if(Position.ContainsNaN()){return nullptr;}
        const auto Id=D.CreateVertex();P[Id]=FVector3f(Position);V.Add(Id);
    }
    for(int32 I=0;I<LOD.IndexBuffer.GetNumIndices();I+=3)
    {
        TArray<FVertexInstanceID> Corners;
        for(int32 C=0;C<3;++C)
        {
            const int32 Index=LOD.IndexBuffer.GetIndex(I+C);const auto VI=D.CreateVertexInstance(V[Index]);
            N[VI]=LOD.VertexBuffers.StaticMeshVertexBuffer.VertexTangentZ(Index);
            T[VI]=LOD.VertexBuffers.StaticMeshVertexBuffer.VertexTangentX(Index);S[VI]=1;
            UV.Set(VI,0,LOD.VertexBuffers.StaticMeshVertexBuffer.GetVertexUV(Index,0));Corners.Add(VI);
        }
        D.CreateTriangle(Group,Corners);
    }
    auto* Package=CreatePackage(*PackagePath);
    auto* Result=NewObject<UStaticMesh>(Package,*FPackageName::GetLongPackageAssetName(PackagePath),RF_Public|RF_Standalone);
    Result->GetStaticMaterials()=Source->GetStaticMaterials();
    UStaticMesh::FBuildMeshDescriptionsParams Params;Params.bBuildSimpleCollision=false;Params.bFastBuild=false;
    Result->BuildFromMeshDescriptions({&D},Params);
    if(Result->GetNumSourceModels()>0)
    {
        auto& Settings=Result->GetSourceModel(0).BuildSettings;Settings.bRecomputeNormals=true;Settings.bRecomputeTangents=true;
        Result->Build(false);
    }
    FAssetRegistryModule::AssetCreated(Result);Package->MarkPackageDirty();return Result;
#else
    return nullptr;
#endif
}
