// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Interfaces/OnlineSessionInterface.h"

#include "MultiplayerSessionsSubsystem.generated.h"


//
// Declaring our own custom delegates for the Menu class to bind callbacks to
// 

DECLARE_MULTICAST_DELEGATE_OneParam(FMultiplayerOnJoinSessionComplete, EOnJoinSessionCompleteResult::Type Result);
DECLARE_MULTICAST_DELEGATE_TwoParams(FMultiplayerOnFindSessionsComplete, const TArray<FOnlineSessionSearchResult>& SessionResult, bool bWasSuccesfull);


DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FMultiplayerOnCreateSessionComplete, FName, NewSessionName, bool, bWasSuccesfull);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FMultiplayerOnStartSessionComplete,   bool, bWasSuccesfull);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FMultiplayerOnDestroySessionComplete, bool, bWasSuccesfull);


/**
 * 
 */
UCLASS()
class MULTIPLAYERSESSIONS_API UMultiplayerSessionsSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()
public:
	UMultiplayerSessionsSubsystem();

	//
	// To handle session functionality. The menu class will call this
	//

	void CreateSession(int32 NumPublicConnections, FString MatchType);
	void FindSessions(int32 MaxSearchResults);
	void JoinSession(const FOnlineSessionSearchResult& SessionResult);
	void DestroySession();
	void StartSession();

	//
	// Our own custom delegates for the menu class to bind callbacks to
	//

	FMultiplayerOnCreateSessionComplete  MultiplayerOnCreateSessionComplete;
	FMultiplayerOnJoinSessionComplete    MultiplayerOnJoinSessionComplete;
	FMultiplayerOnFindSessionsComplete   MultiplayerOnFindSessionsComplete;
	FMultiplayerOnStartSessionComplete   MultiplayerOnStartSessionComplete;
	FMultiplayerOnDestroySessionComplete MultiplayerOnDestroySessionComplete;


protected:

	//
	// Internal Callbacks for the delegates we'll add to the online session
	// This don't need to be called outside this class
	//

	void OnCreateSessionComplete(FName SessionName, bool bIsWasSuccesfull);
	void OnFindSessionsComplete(bool bIsWasSuccesfull);
	void OnJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result);
	void OnDestroySessionComplete(FName SessionName, bool bIsWasSuccesfull);
	void OnStartSessionComplete(FName SessionName, bool bIsWasSuccesfull);

private:
	IOnlineSessionPtr SessionInterface;

	TSharedPtr<FOnlineSessionSearch> SessionSearch;

	TSharedPtr<FOnlineSessionSettings> SessionSettings;
	
	//
	// To add the Online Session Interface Delegate list
	// We'll bind our MutliplayerSessionsSubsystem internal callbacks to this
	//
	FOnCreateSessionCompleteDelegate  CreateSessionCompleteDelegate;
	FDelegateHandle CreateSessionCompleteDelegateHandle;

	FOnFindSessionsCompleteDelegate   FindSessionsCompleteDelegate;
	FDelegateHandle FindSessionsCompleteDelegateHandle;

	FOnJoinSessionCompleteDelegate    JoinSessionCompleteDelegate;
	FDelegateHandle JoinSessionCompleteDelegateHandle;

	FOnDestroySessionCompleteDelegate DestroySessionCompleteDelegate;
	FDelegateHandle DestroySessionCompleteDelegateHandle;

	FOnStartSessionCompleteDelegate   StartSessionCompleteDelegate;
	FDelegateHandle StartSessionCompleteDelegateHandle;

	bool bCreateSessionOnDestroy{ false };

	int32 LastNumPublicConnections;
	FString LastMatchType;
};
