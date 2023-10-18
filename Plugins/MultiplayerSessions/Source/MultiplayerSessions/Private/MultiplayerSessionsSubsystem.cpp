// Fill out your copyright notice in the Description page of Project Settings.


#include "MultiplayerSessionsSubsystem.h"
#include "OnlineSubsystem.h"
#include "OnlineSessionSettings.h"

UMultiplayerSessionsSubsystem::UMultiplayerSessionsSubsystem() :
  CreateSessionCompleteDelegate(FOnCreateSessionCompleteDelegate::CreateUObject(this, &ThisClass::OnCreateSessionComplete)),
  FindSessionsCompleteDelegate(FOnFindSessionsCompleteDelegate::CreateUObject(this, &ThisClass::OnFindSessionsComplete)),
  JoinSessionCompleteDelegate(FOnJoinSessionCompleteDelegate::CreateUObject(this, &ThisClass::OnJoinSessionComplete)),
  DestroySessionCompleteDelegate(FOnDestroySessionCompleteDelegate::CreateUObject(this, &ThisClass::OnDestroySessionComplete)),
  StartSessionCompleteDelegate(FOnStartSessionCompleteDelegate::CreateUObject(this, &ThisClass::OnStartSessionComplete))
{
  IOnlineSubsystem* Subsystem = IOnlineSubsystem::Get();
  if (Subsystem)
  {
    if (GEngine)
    {
      GEngine->AddOnScreenDebugMessage(
        -1,
        15.f,
        FColor::Blue,
        FString::Printf(TEXT("Subsystem %s"), *Subsystem->GetSubsystemName().ToString()));
    }

    SessionInterface = Subsystem->GetSessionInterface();
  }
}

void UMultiplayerSessionsSubsystem::CreateSession(int32 NumPublicConnections, FString MatchType)
{
  if (!SessionInterface.IsValid())
    return;

  const ULocalPlayer* LocalPlayer = GetWorld()->GetFirstLocalPlayerFromController();

  auto ExistingSession = SessionInterface->GetNamedSession(NAME_GameSession);

  if (ExistingSession)
  {
    bCreateSessionOnDestroy = true;
    LastNumPublicConnections = NumPublicConnections;
    LastMatchType = MatchType;

    DestroySession();
  }

  CreateSessionCompleteDelegateHandle = SessionInterface->AddOnCreateSessionCompleteDelegate_Handle(CreateSessionCompleteDelegate);

  SessionSettings = MakeShareable(new FOnlineSessionSettings());
  SessionSettings->bIsLANMatch = IOnlineSubsystem::Get()->GetSubsystemName() == "NULL";
  SessionSettings->NumPublicConnections = NumPublicConnections;
  SessionSettings->bAllowJoinInProgress = true;
  SessionSettings->bAllowJoinViaPresence = true;
  SessionSettings->bShouldAdvertise = true;
  SessionSettings->bUsesPresence = true;
  SessionSettings->Set(FName("MatchType"), MatchType, EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);
  SessionSettings->BuildUniqueId = 1;

  //If you cannot find sessions try this on session settings
  SessionSettings->bUseLobbiesIfAvailable = true;

  if (!SessionInterface->CreateSession(*LocalPlayer->GetPreferredUniqueNetId(), NAME_GameSession, *SessionSettings))
  {
    if (GEngine)
    {
      GEngine->AddOnScreenDebugMessage(
        -1,
        15.f,
        FColor::Red,
        FString::Printf(TEXT("Create Session Failed")));
    }

    SessionInterface->ClearOnCreateSessionCompleteDelegate_Handle(CreateSessionCompleteDelegateHandle);

    // Broadcast custom delegates
    MultiplayerOnCreateSessionComplete.Broadcast(NAME_GameSession, false);
  }
}

void UMultiplayerSessionsSubsystem::FindSessions(int32 MaxSearchResults)
{
  if (!SessionInterface.IsValid())
    return;

  FindSessionsCompleteDelegateHandle = SessionInterface->AddOnFindSessionsCompleteDelegate_Handle(FindSessionsCompleteDelegate);

  SessionSearch = MakeShareable(new FOnlineSessionSearch());
  SessionSearch->MaxSearchResults = MaxSearchResults;
  SessionSearch->QuerySettings.Set(SEARCH_PRESENCE, true, EOnlineComparisonOp::Equals);
  SessionSearch->bIsLanQuery = IOnlineSubsystem::Get()->GetSubsystemName() == "NULL";

  const ULocalPlayer* LocalPlayer = GetWorld()->GetFirstLocalPlayerFromController();
  if (!SessionInterface->FindSessions(*LocalPlayer->GetPreferredUniqueNetId(), SessionSearch.ToSharedRef()))
  {
    if (GEngine)
    {
      GEngine->AddOnScreenDebugMessage(
        -1,
        10.f,
        FColor::Red,
        FString::Printf(TEXT("Failed To Find Sessions"))
      );
    }

    SessionInterface->ClearOnFindSessionsCompleteDelegate_Handle(FindSessionsCompleteDelegateHandle);

    MultiplayerOnFindSessionsComplete.Broadcast(TArray<FOnlineSessionSearchResult>(), false);
  }
}

void UMultiplayerSessionsSubsystem::JoinSession(const FOnlineSessionSearchResult& SessionResult)
{
  if (!SessionInterface.IsValid())
  {
    MultiplayerOnJoinSessionComplete.Broadcast(EOnJoinSessionCompleteResult::UnknownError);
    return;
  }

  SessionInterface->AddOnJoinSessionCompleteDelegate_Handle(JoinSessionCompleteDelegate);

  const ULocalPlayer* LocalPlayer = GetWorld()->GetFirstLocalPlayerFromController();
  if (!SessionInterface->JoinSession(*LocalPlayer->GetPreferredUniqueNetId(), NAME_GameSession, SessionResult))
  {
    SessionInterface->ClearOnJoinSessionCompleteDelegate_Handle(JoinSessionCompleteDelegateHandle);

    MultiplayerOnJoinSessionComplete.Broadcast(EOnJoinSessionCompleteResult::UnknownError);
  }
}

void UMultiplayerSessionsSubsystem::DestroySession()
{
  if (!SessionInterface.IsValid())
  {
    MultiplayerOnDestroySessionComplete.Broadcast(false);
    return;
  }

  DestroySessionCompleteDelegateHandle = SessionInterface->AddOnDestroySessionCompleteDelegate_Handle(DestroySessionCompleteDelegate);

  if (GEngine)
  {
    GEngine->AddOnScreenDebugMessage(
      -1,
      15.f,
      FColor::Yellow,
      FString::Printf(TEXT("Session with the same name is exists, recreating...")));
  }

  if (!SessionInterface->DestroySession(NAME_GameSession))
  {
    SessionInterface->ClearOnDestroySessionCompleteDelegate_Handle(DestroySessionCompleteDelegateHandle);
    MultiplayerOnDestroySessionComplete.Broadcast(false);
  }
}

void UMultiplayerSessionsSubsystem::StartSession()
{

}

//
// Callbacks
//

void UMultiplayerSessionsSubsystem::OnCreateSessionComplete(FName SessionName, bool bIsWasSuccesfull)
{
  if (SessionInterface)
    SessionInterface->ClearOnCreateSessionCompleteDelegate_Handle(CreateSessionCompleteDelegateHandle);

  MultiplayerOnCreateSessionComplete.Broadcast(SessionName, bIsWasSuccesfull);
}

void UMultiplayerSessionsSubsystem::OnFindSessionsComplete(bool bIsWasSuccesfull)
{
  if (SessionInterface)
    SessionInterface->ClearOnFindSessionsCompleteDelegate_Handle(FindSessionsCompleteDelegateHandle);

  if (SessionSearch->SearchResults.IsEmpty())
  {
    if (GEngine)
    {
      GEngine->AddOnScreenDebugMessage(
        -1,
        10.f,
        FColor::Red,
        FString::Printf(TEXT("Search results is empty"))
      );
    }
    MultiplayerOnFindSessionsComplete.Broadcast(TArray<FOnlineSessionSearchResult>(), false);
    return;
  }

  MultiplayerOnFindSessionsComplete.Broadcast(SessionSearch->SearchResults, true);
}

void UMultiplayerSessionsSubsystem::OnJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result)
{
  if (SessionInterface)
  {
    SessionInterface->ClearOnJoinSessionCompleteDelegate_Handle(JoinSessionCompleteDelegateHandle);
  }

  MultiplayerOnJoinSessionComplete.Broadcast(Result);
}

void UMultiplayerSessionsSubsystem::OnDestroySessionComplete(FName SessionName, bool bIsWasSuccesfull)
{
  if (SessionInterface)
    SessionInterface->ClearOnDestroySessionCompleteDelegate_Handle(DestroySessionCompleteDelegateHandle);

  if (bIsWasSuccesfull && bCreateSessionOnDestroy)
  {
    bCreateSessionOnDestroy = false;

    const ULocalPlayer* LocalPlayer = GetWorld()->GetFirstLocalPlayerFromController();
    CreateSession(LastNumPublicConnections, LastMatchType);
  }

  MultiplayerOnDestroySessionComplete.Broadcast(bIsWasSuccesfull);
}

void UMultiplayerSessionsSubsystem::OnStartSessionComplete(FName SessionName, bool bIsWasSuccesfull)
{
}
