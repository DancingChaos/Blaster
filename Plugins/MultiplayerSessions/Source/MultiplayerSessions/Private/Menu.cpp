// Fill out your copyright notice in the Description page of Project Settings.


#include "Menu.h"
#include "Components/Button.h"
#include "Components/ListView.h"
#include "MultiplayerSessionsSubsystem.h"
#include "OnlineSubsystem.h"
#include "OnlineSessionSettings.h"

void UMenu::MenuSetup(int32 NumOfPublicConnections, FString TypeOfMatch, FString LobbyPath)
{
  PathToLobby = FString::Printf(TEXT("%s?listen"), *LobbyPath);

  NumPublicConnections = NumOfPublicConnections;
  MatchType = TypeOfMatch;

  AddToViewport();
  SetVisibility(ESlateVisibility::Visible);
  bIsFocusable = true;

  UWorld* World = GetWorld();

  if (World)
  {
    APlayerController* PlayerController = World->GetFirstPlayerController();

    if (PlayerController)
    {
      FInputModeUIOnly InputModeData;

      InputModeData.SetWidgetToFocus(TakeWidget());
      InputModeData.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);

      PlayerController->SetInputMode(InputModeData);
      PlayerController->SetShowMouseCursor(true);
    }
  }

  UGameInstance* GameInstance = GetGameInstance();
  if (GameInstance)
  {
    SessionsSubsystem = GameInstance->GetSubsystem<UMultiplayerSessionsSubsystem>();
  }

  if (SessionsSubsystem)
  {
    SessionsSubsystem->MultiplayerOnCreateSessionComplete.AddDynamic(this, &ThisClass::OnCreateSession);
    SessionsSubsystem->MultiplayerOnStartSessionComplete.AddDynamic(this, &ThisClass::OnStartSession);
    SessionsSubsystem->MultiplayerOnDestroySessionComplete.AddDynamic(this, &ThisClass::OnDestroySession);

    SessionsSubsystem->MultiplayerOnFindSessionsComplete.AddUObject(this, &ThisClass::OnFindSessions);
    SessionsSubsystem->MultiplayerOnJoinSessionComplete.AddUObject(this, &ThisClass::OnJoinSession);
  }
}

bool UMenu::Initialize()
{
  if (!Super::Initialize())
    return false;

  if (HostBtn)
    HostBtn->OnClicked.AddDynamic(this, &UMenu::HostButtonClicked);

  if (JoinBtn)
    JoinBtn->OnClicked.AddDynamic(this, &UMenu::JoinButtonClicked);

  return true;
}

void UMenu::NativeDestruct()
{
  MenuTearDown();
  Super::NativeDestruct();
}

void UMenu::OnCreateSession(FName SessionName, bool bWasSuccesfull)
{
  if (bWasSuccesfull)
  {
    if (GEngine)
    {
      GEngine->AddOnScreenDebugMessage(
        -1,
        15.f,
        FColor::Green,
        FString::Printf(TEXT("Session \"%s\" Created"), *SessionName.ToString())
      );
    }

    UWorld* World = GetWorld();
    if (World)
    {
      if (GEngine)
      {
        GEngine->AddOnScreenDebugMessage(
          -1,
          15.f,
          FColor::Green,
          FString::Printf(TEXT("Server Travel To Lobby")));
      }
      World->ServerTravel(FString(PathToLobby));
    }
  }
  else
  {
    if (GEngine)
    {
      GEngine->AddOnScreenDebugMessage(
        -1,
        15.f,
        FColor::Red,
        FString::Printf(TEXT("Failed to create session"))
      );
    }

    HostBtn->SetIsEnabled(true);
  }
}

void UMenu::OnFindSessions(const TArray<FOnlineSessionSearchResult>& SessionResults, bool bWasSuccesfull)
{
  if (!SessionsSubsystem)
    return;

  for (auto Result : SessionResults)
  {
    FString SettingsValue;
    Result.Session.SessionSettings.Get(FName("MatchType"), SettingsValue);
    
    if (GEngine)
    {
      GEngine->AddOnScreenDebugMessage(
        -1,
        120.f,
        FColor::Red,
        FString::Printf(TEXT("MatchType: %s | Server (ID:%s) by user: %s"), *SettingsValue, *Result.Session.SessionInfo->GetSessionId().ToString(), *Result.Session.OwningUserName)
      );
    }

    if (SettingsValue == MatchType)
    {
      SessionsSubsystem->JoinSession(Result);
      return;
    }
  }

  if (bWasSuccesfull || SessionResults.IsEmpty())
    JoinBtn->SetIsEnabled(true);
}

void UMenu::OnJoinSession(EOnJoinSessionCompleteResult::Type Result)
{
  IOnlineSubsystem* Subsystem = IOnlineSubsystem::Get();
  if (Subsystem)
  {
    IOnlineSessionPtr SessionInterface = Subsystem->GetSessionInterface();

    if (SessionInterface.IsValid())
    {
      FString Address;
      SessionInterface->GetResolvedConnectString(NAME_GameSession, Address);

      APlayerController* PlayerController = GetGameInstance()->GetFirstLocalPlayerController();
      if (PlayerController)
      {
        PlayerController->ClientTravel(Address, ETravelType::TRAVEL_Absolute);
      }
    }
  }

  if (Result != EOnJoinSessionCompleteResult::Success)
  {
    JoinBtn->SetIsEnabled(true);
  }
  else
  {
    if (GEngine)
    {
      GEngine->AddOnScreenDebugMessage(
        -1,
        15.f,
        FColor::Red,
        FString::Printf(TEXT("Connection Failed | %d"), Result)
      );
    }
  }
}

void UMenu::OnStartSession(bool bWasSuccesfull)
{
}

void UMenu::OnDestroySession(bool bWasSuccesfull)
{
}

void UMenu::HostButtonClicked()
{
  HostBtn->SetIsEnabled(false);
  if (SessionsSubsystem)
    SessionsSubsystem->CreateSession(NumPublicConnections, MatchType);
}

void UMenu::JoinButtonClicked()
{
  JoinBtn->SetIsEnabled(false);

  if (SessionsSubsystem)
  {
    if (GEngine)
    {
      GEngine->AddOnScreenDebugMessage(
        -1,
        10.f,
        FColor::Green,
        FString::Printf(TEXT("Searching For Sessions"))
      );
    }
    SessionsSubsystem->FindSessions(10000);
  }
  else
  {
    if (GEngine)
    {
      GEngine->AddOnScreenDebugMessage(
        -1,
        10.f,
        FColor::Green,
        FString::Printf(TEXT("Failed to search"))
      );
    }
  }
}

void UMenu::MenuTearDown()
{
  RemoveFromParent();
  UWorld* World = GetWorld();

  if (World)
  {
    APlayerController* PlayerController = World->GetFirstPlayerController();
    if (PlayerController)
    {
      FInputModeGameOnly GameOnlyInputModeData;
      PlayerController->SetInputMode(GameOnlyInputModeData);
      PlayerController->SetShowMouseCursor(false);
    }
  }
}
