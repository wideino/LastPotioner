// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Items/LPBaseItem.h"
#include "LPInventoryComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSlotChangedSignature, int, SlotIndex);

class ALPBaseItem;
class ULPInventorySlot;

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class LASTPOTIONER_API ULPInventoryComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	ULPInventoryComponent();

	UPROPERTY(BlueprintAssignable)
	FOnSlotChangedSignature OnSlotChanged;

	UFUNCTION(BlueprintCallable)
	int AddItemByClassRef(const ALPBaseItem* Item); // to resolve blueprints conflict
	
	UFUNCTION(BlueprintCallable)
	int AddItem(const FItemSignature& Item);

	UFUNCTION(BlueprintCallable)
	int AddItemInSlot(const FItemSignature& Item, int SlotIndex);

	UFUNCTION(BlueprintCallable)
	int GetSlotsNum() const { return InventoryContainer.Num(); }

	UFUNCTION(BlueprintCallable)
	const FItemSignature& GetSlotByIndex(int Index) const;

	UFUNCTION(BlueprintCallable)
	void SwapSlots(int SelfSlotIndex, int SourceSlotIndex, ULPInventoryComponent* SourceInventory);

	UFUNCTION(BlueprintCallable)
	void SubtractSlotValue(int SlotIndex, int Value);

	UFUNCTION(BlueprintCallable)
	int GetSlotsPerRow() const { return SlotsPerRow; }

	void SetSlotByIndex(int Index, const FItemSignature& NewData);

	UFUNCTION(BlueprintCallable)
	TArray<FItemSignature> GetSlotsByObjectiveID(FName ObjectiveID);


private:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	TArray<FItemSignature> InventoryContainer;

	int FindSuitableSlot(const FItemSignature& Item);
	void SetSlotEmpty(int SlotIndex);
	void InitializeSlot(int SlotIndex, const FItemSignature& Item);

	UPROPERTY(EditAnywhere, Category = "UI")
	int SlotsPerRow = 4;
};
