// Fill out your copyright notice in the Description page of Project Settings.


#include "OverheadWidget.h"
#include "Components/TextBlock.h"

void UOverheadWidget::SetDisplayText(FString TextToDisplay)
{
  if (DisplayText)
    DisplayText->SetText(FText::FromString(TextToDisplay));
}

void UOverheadWidget::ShowPlayerNetRole(APawn* InPawn)
{
  ENetRole RemoteRole = InPawn->GetRemoteRole();
  FString Role;

  switch (RemoteRole)
  {
  case ROLE_None:
    Role = FString("None");
    break;
  case ROLE_SimulatedProxy:
    Role = FString("Simulate Proxy");
    break;
  case ROLE_AutonomousProxy:
    Role = FString("Autonomous Proxy");
    break;
  case ROLE_Authority:
    Role = FString("Authority");
    break;
  default:
    Role = FString("None");
    break;
  }

  //FString RemoteRoleString = FString::Printf(TEXT("Local Role: %s"), *Role);
  SetDisplayText("");
}

void UOverheadWidget::NativeDestruct()
{
  RemoveFromParent();
  Super::NativeDestruct();
}