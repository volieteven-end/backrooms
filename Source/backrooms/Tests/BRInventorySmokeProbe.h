#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BRInventorySmokeProbe.generated.h"
class ABRPlayerCharacter;class ABRMenuPlayerController;class UBRInventoryComponent;class USkeletalMeshComponent;class AStaticMeshActor;
UCLASS()
class BACKROOMS_API ABRInventorySmokeProbe : public AActor
{
    GENERATED_BODY()
public:ABRInventorySmokeProbe();virtual void Tick(float DT) override;
private:
    void TickArmsViews();
    void TickArmsMotion(float DT);
    void Check(bool OK,const TCHAR* Name);void Capture(const TCHAR* Name);void Stand(FVector Location,float Yaw=0);void StepFixture(float Height);
    UPROPERTY() TObjectPtr<ABRPlayerCharacter> P;
    UPROPERTY() TObjectPtr<ABRMenuPlayerController> PC;
    UPROPERTY() TObjectPtr<UBRInventoryComponent> Bag;
    UPROPERTY() TObjectPtr<USkeletalMeshComponent> Arms;
    UPROPERTY() TObjectPtr<AStaticMeshActor> Curb;
    double Started=0;int32 Stage=0,Checks=0,Failures=0;
    FVector HandBefore=FVector::ZeroVector;float SanityBefore=100;FGuid UsedId;int32 WaterSlot=INDEX_NONE;
    float MotionTime=0,NextMotionCapture=0;int32 MotionFrame=0;
    FVector MotionLeft=FVector::ZeroVector,MotionRight=FVector::ZeroVector;
    FVector CaptureLeft=FVector::ZeroVector,CaptureRight=FVector::ZeroVector;
    float MaxHandStep=0;int32 MeasuredMotionFrames=0;bool bWalkingPoseMoved=false,bRunningPoseMoved=false;
};
